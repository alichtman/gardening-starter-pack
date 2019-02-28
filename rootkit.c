/**
 * @file	rootkit.c
 * @author	Aaron Lichtman, Arch Gupta  and Brandon Weidner
 * @brief	A rootkit. TODO: Expand description
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
//#include <linux/mutex.h> // Will likely need this for handling interrupts.

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aaron Lichtman");
MODULE_AUTHOR("Arch Gupta");
MODULE_AUTHOR("Brandon Weidner");
MODULE_DESCRIPTION("Linux rootkit.");
MODULE_VERSION("0.1");

// Look into module_param() and MODULE_PARM_DESC for adding parameters

/**
 * Rootkit module initialization.
 */
static int rootkit_init(void) {
   printk(KERN_INFO "Initializing rootkit.\n");
   return 0;
}

/**
 * Called at exit. All cleanup should be done here.
 */
static int rootkit_exit(void) {
	printk(KERN_INFO "Cleaning up rootkit.\n");
}

module_init(rootkit_init);
module_init(rootkit_exit);
