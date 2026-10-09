#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

#include "../src/phoenix_ioctl.h"

int main(void)
{
    int fd;
    int mode = 1;

    const char *syscall_name = "read";
    const char *process_name = "syscall_test";

    printf("Opening /dev/phoenix...\n");

    fd = open("/dev/phoenix", O_RDWR);

    if (fd < 0) {
        printf("Failed to open /dev/phoenix: %s\n",
               strerror(errno));
        return 1;
    }

    printf("/dev/phoenix opened successfully.\n");


    /*
     * Set Phoenix mode.
     */
    printf("Sending SET_MODE ioctl with mode=%d...\n",
           mode);

    if (ioctl(fd,
              PHOENIX_IOCTL_SET_MODE,
              &mode) < 0) {

        printf("SET_MODE ioctl failed: %s\n",
               strerror(errno));

        close(fd);
        return 1;
    }

    printf("SET_MODE ioctl completed successfully.\n");


    /*
     * Select syscall.
     */
    printf("Sending SET_SYSCALL ioctl with syscall=%s...\n",
           syscall_name);

    if (ioctl(fd,
              PHOENIX_IOCTL_SET_SYSCALL,
              syscall_name) < 0) {

        printf("SET_SYSCALL ioctl failed: %s\n",
               strerror(errno));

        close(fd);
        return 1;
    }

    printf("SET_SYSCALL ioctl completed successfully.\n");


    /*
     * Select process.
     */
    printf("Sending SET_PROCESS ioctl with process=%s...\n",
           process_name);

    if (ioctl(fd,
              PHOENIX_IOCTL_SET_PROCESS,
              process_name) < 0) {

        printf("SET_PROCESS ioctl failed: %s\n",
               strerror(errno));

        close(fd);
        return 1;
    }

    printf("SET_PROCESS ioctl completed successfully.\n");


    close(fd);

    printf("/dev/phoenix closed.\n");

    return 0;
}
