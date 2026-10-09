#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kprobes.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#include "phoenix_ioctl.h"

#define PHOENIX_DEVICE_NAME "phoenix"

static int phoenix_mode = PHOENIX_MODE_OFF;

static dev_t phoenix_dev;
static struct class *phoenix_class;
static struct cdev phoenix_cdev;


/* -------------------- Configuration -------------------- */

/*
 * Selected syscall.
 *
 * "none" means that no syscall filter is selected.
 */
static char phoenix_syscall[32] = "none";

/*
 * Selected process.
 *
 * "none" means that no process filter is selected.
 */
static char phoenix_process[32] = "none";


/* -------------------- Event Buffer -------------------- */

/*
 * Stores the latest syscall event.
 */
static char phoenix_event[32] = "";

/*
 * 0 = no new event
 * 1 = a new event is available
 */
static int phoenix_event_available = 0;


/* -------------------- Syscall Name Normalization -------------------- */

/*
 * Convert the kernel Kprobe symbol name
 * into the normal syscall name used by user space.
 *
 * Example:
 *
 * __x64_sys_read  -> read
 * __x64_sys_write -> write
 * __x64_sys_openat -> openat
 */
static const char *normalize_syscall(const char *symbol)
{
    if (strcmp(symbol, "__x64_sys_openat") == 0)
        return "openat";

    if (strcmp(symbol, "__x64_sys_read") == 0)
        return "read";

    if (strcmp(symbol, "__x64_sys_write") == 0)
        return "write";

    return NULL;
}


/* -------------------- Kprobe Handler -------------------- */

static int syscall_handler(struct kprobe *p, struct pt_regs *regs)
{
    const char *syscall_name;

    /*
     * Only generate events in LOG mode.
     */
    if (READ_ONCE(phoenix_mode) != PHOENIX_MODE_LOG)
        return 0;

    /*
     * Check the selected process.
     *
     * If a process filter is configured,
     * ignore all other processes.
     */
    if (strcmp(phoenix_process, "none") != 0) {

        if (strcmp(current->comm, phoenix_process) != 0)
            return 0;
    }

    /*
     * Convert the kernel symbol name
     * into the normal syscall name.
     */
    syscall_name = normalize_syscall(p->symbol_name);

    if (syscall_name == NULL)
        return 0;

    /*
     * Check the selected syscall.
     *
     * If a syscall filter is configured,
     * ignore all other syscalls.
     */
    if (strcmp(phoenix_syscall, "none") != 0) {

        if (strcmp(syscall_name, phoenix_syscall) != 0)
            return 0;
    }

    /*
     * Print the matching syscall.
     */
    pr_info("Phoenix: syscall=%s process=%s pid=%d\n",
            syscall_name,
            current->comm,
            current->pid);

    /*
     * Store the syscall event.
     */
    strncpy(phoenix_event,
            p->symbol_name,
            sizeof(phoenix_event) - 1);

    phoenix_event[sizeof(phoenix_event) - 1] = '\0';

    /*
     * Tell read() that a new event is available.
     */
    phoenix_event_available = 1;

    return 0;
}


/* -------------------- IOCTL Handler -------------------- */

static long phoenix_ioctl(struct file *file,
                          unsigned int cmd,
                          unsigned long arg)
{
    int mode;

    switch (cmd) {

    /*
     * Change Phoenix operating mode.
     */
    case PHOENIX_IOCTL_SET_MODE:

        if (copy_from_user(&mode,
                           (int __user *)arg,
                           sizeof(mode)))
            return -EFAULT;

        if (mode != PHOENIX_MODE_OFF &&
            mode != PHOENIX_MODE_LOG &&
            mode != PHOENIX_MODE_BLOCK)
            return -EINVAL;

        WRITE_ONCE(phoenix_mode, mode);

        pr_info("Phoenix: mode changed to %d\n",
                mode);

        return 0;


    /*
     * Select the syscall to monitor.
     */
    case PHOENIX_IOCTL_SET_SYSCALL:

        if (copy_from_user(phoenix_syscall,
                           (char __user *)arg,
                           sizeof(phoenix_syscall) - 1))
            return -EFAULT;

        phoenix_syscall[sizeof(phoenix_syscall) - 1] = '\0';

        /*
         * Validate the selected syscall.
         */
        if (strcmp(phoenix_syscall, "openat") != 0 &&
            strcmp(phoenix_syscall, "read") != 0 &&
            strcmp(phoenix_syscall, "write") != 0 &&
            strcmp(phoenix_syscall, "none") != 0)
            return -EINVAL;

        pr_info("Phoenix: syscall filter changed to %s\n",
                phoenix_syscall);

        return 0;


    /*
     * Select the process to monitor.
     */
    case PHOENIX_IOCTL_SET_PROCESS:

        if (copy_from_user(phoenix_process,
                           (char __user *)arg,
                           sizeof(phoenix_process) - 1))
            return -EFAULT;

        phoenix_process[sizeof(phoenix_process) - 1] = '\0';

        pr_info("Phoenix: process filter changed to %s\n",
                phoenix_process);

        return 0;


    default:

        return -ENOTTY;
    }
}


/* -------------------- Device Read -------------------- */

static ssize_t phoenix_read(struct file *file,
                            char __user *buffer,
                            size_t length,
                            loff_t *offset)
{
    size_t event_length;

    /*
     * If there is no new event,
     * there is nothing to send.
     */
    if (!phoenix_event_available)
        return 0;

    /*
     * Calculate the size of the event.
     */
    event_length = strlen(phoenix_event);

    /*
     * Do not copy more than the user buffer can hold.
     */
    if (length > event_length)
        length = event_length;

    /*
     * Copy the event from kernel space
     * to user space.
     */
    if (copy_to_user(buffer,
                     phoenix_event,
                     length))
        return -EFAULT;

    /*
     * The event has now been consumed.
     */
    phoenix_event_available = 0;

    return length;
}


/* -------------------- File Operations -------------------- */

static const struct file_operations phoenix_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = phoenix_ioctl,
    .read = phoenix_read,
};


/* -------------------- Kprobes -------------------- */

static struct kprobe kp_openat = {
    .symbol_name = "__x64_sys_openat",
    .pre_handler = syscall_handler,
};

static struct kprobe kp_read = {
    .symbol_name = "__x64_sys_read",
    .pre_handler = syscall_handler,
};

static struct kprobe kp_write = {
    .symbol_name = "__x64_sys_write",
    .pre_handler = syscall_handler,
};


/* -------------------- Module Initialization -------------------- */

static int __init phoenix_init(void)
{
    int ret;

    /*
     * Allocate a device number.
     */
    ret = alloc_chrdev_region(&phoenix_dev,
                              0,
                              1,
                              PHOENIX_DEVICE_NAME);

    if (ret < 0) {
        pr_err("Phoenix: failed to allocate device number\n");
        return ret;
    }

    /*
     * Initialize the character device.
     */
    cdev_init(&phoenix_cdev,
              &phoenix_fops);

    phoenix_cdev.owner = THIS_MODULE;

    /*
     * Add the character device to the kernel.
     */
    ret = cdev_add(&phoenix_cdev,
                   phoenix_dev,
                   1);

    if (ret < 0) {
        pr_err("Phoenix: failed to add character device\n");

        unregister_chrdev_region(phoenix_dev, 1);

        return ret;
    }

    /*
     * Create the device class.
     */
    phoenix_class = class_create(PHOENIX_DEVICE_NAME);

    if (IS_ERR(phoenix_class)) {

        pr_err("Phoenix: failed to create device class\n");

        cdev_del(&phoenix_cdev);

        unregister_chrdev_region(phoenix_dev, 1);

        return PTR_ERR(phoenix_class);
    }

    /*
     * Create /dev/phoenix.
     */
    if (IS_ERR(device_create(phoenix_class,
                             NULL,
                             phoenix_dev,
                             NULL,
                             PHOENIX_DEVICE_NAME))) {

        pr_err("Phoenix: failed to create device\n");

        class_destroy(phoenix_class);

        cdev_del(&phoenix_cdev);

        unregister_chrdev_region(phoenix_dev, 1);

        return -EINVAL;
    }

    /*
     * Register the openat Kprobe.
     */
    ret = register_kprobe(&kp_openat);

    if (ret < 0) {

        pr_err("Phoenix: failed to register openat kprobe, error=%d\n",
               ret);

        device_destroy(phoenix_class,
                       phoenix_dev);

        class_destroy(phoenix_class);

        cdev_del(&phoenix_cdev);

        unregister_chrdev_region(phoenix_dev, 1);

        return ret;
    }

    /*
     * Register the read Kprobe.
     */
    ret = register_kprobe(&kp_read);

    if (ret < 0) {

        pr_err("Phoenix: failed to register read kprobe, error=%d\n",
               ret);

        unregister_kprobe(&kp_openat);

        device_destroy(phoenix_class,
                       phoenix_dev);

        class_destroy(phoenix_class);

        cdev_del(&phoenix_cdev);

        unregister_chrdev_region(phoenix_dev, 1);

        return ret;
    }

    /*
     * Register the write Kprobe.
     */
    ret = register_kprobe(&kp_write);

    if (ret < 0) {

        pr_err("Phoenix: failed to register write kprobe, error=%d\n",
               ret);

        unregister_kprobe(&kp_read);

        unregister_kprobe(&kp_openat);

        device_destroy(phoenix_class,
                       phoenix_dev);

        class_destroy(phoenix_class);

        cdev_del(&phoenix_cdev);

        unregister_chrdev_region(phoenix_dev, 1);

        return ret;
    }

    pr_info("Phoenix: module loaded. Mode=off.\n");

    return 0;
}


/* -------------------- Module Cleanup -------------------- */

static void __exit phoenix_exit(void)
{
    /*
     * Remove Kprobes.
     */
    unregister_kprobe(&kp_write);

    unregister_kprobe(&kp_read);

    unregister_kprobe(&kp_openat);

    /*
     * Remove /dev/phoenix.
     */
    device_destroy(phoenix_class,
                   phoenix_dev);

    class_destroy(phoenix_class);

    cdev_del(&phoenix_cdev);

    unregister_chrdev_region(phoenix_dev,
                             1);

    pr_info("Phoenix: module unloaded.\n");
}


module_init(phoenix_init);
module_exit(phoenix_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Asrin Namdar");
MODULE_DESCRIPTION("Phoenix Kprobe module with IOCTL, filtering and kernel-to-user event notification");
