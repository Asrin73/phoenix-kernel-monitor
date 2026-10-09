#ifndef PHOENIX_IOCTL_H
#define PHOENIX_IOCTL_H

#include <linux/ioctl.h>

/*
 * Phoenix operating modes
 */
#define PHOENIX_MODE_OFF   0
#define PHOENIX_MODE_LOG   1
#define PHOENIX_MODE_BLOCK 2

/*
 * IOCTL definitions
 */
#define PHOENIX_IOCTL_MAGIC 'P'

#define PHOENIX_IOCTL_SET_MODE \
    _IOW(PHOENIX_IOCTL_MAGIC, 1, int)

#define PHOENIX_IOCTL_SET_SYSCALL \
    _IOW(PHOENIX_IOCTL_MAGIC, 2, char[32])

#define PHOENIX_IOCTL_SET_PROCESS \
    _IOW(PHOENIX_IOCTL_MAGIC, 3, char[32])

#endif

