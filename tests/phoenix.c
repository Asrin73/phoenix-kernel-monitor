#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "../src/phoenix_ioctl.h"

int main(int argc, char *argv[])
{
    int fd;
    int mode;

    /* Check command-line arguments */
    if (argc < 2) {
        printf("Usage:\n");
        printf("  %s --log\n", argv[0]);
        printf("  %s --off\n", argv[0]);
        printf("  %s --block\n", argv[0]);
        printf("  %s --syscall <name>\n", argv[0]);
        return 1;
    }

    /* Handle --log */
    if (strcmp(argv[1], "--log") == 0) {

        mode = 1;

        fd = open("/dev/phoenix", O_RDWR);

        if (fd < 0) {
            printf("Failed to open /dev/phoenix: %s\n",
                   strerror(errno));
            return 1;
        }

        if (ioctl(fd,
                  PHOENIX_IOCTL_SET_MODE,
                  &mode) < 0) {
            printf("SET_MODE ioctl failed: %s\n",
                   strerror(errno));
            close(fd);
            return 1;
        }

        printf("Phoenix mode changed to LOG.\n");

        close(fd);

        return 0;
    }

    /* Handle --off */
    if (strcmp(argv[1], "--off") == 0) {

        mode = 0;

        fd = open("/dev/phoenix", O_RDWR);

        if (fd < 0) {
            printf("Failed to open /dev/phoenix: %s\n",
                   strerror(errno));
            return 1;
        }

        if (ioctl(fd,
                  PHOENIX_IOCTL_SET_MODE,
                  &mode) < 0) {
            printf("SET_MODE ioctl failed: %s\n",
                   strerror(errno));
            close(fd);
            return 1;
        }

        printf("Phoenix mode changed to OFF.\n");

        close(fd);

        return 0;
    }

    /* Handle --block */
    if (strcmp(argv[1], "--block") == 0) {

        mode = 2;

        fd = open("/dev/phoenix", O_RDWR);

        if (fd < 0) {
            printf("Failed to open /dev/phoenix: %s\n",
                   strerror(errno));
            return 1;
        }

        if (ioctl(fd,
                  PHOENIX_IOCTL_SET_MODE,
                  &mode) < 0) {
            printf("SET_MODE ioctl failed: %s\n",
                   strerror(errno));
            close(fd);
            return 1;
        }

        printf("Phoenix mode changed to BLOCK.\n");

        close(fd);

        return 0;
    }

    /* Handle --syscall <name> */
    if (strcmp(argv[1], "--syscall") == 0) {

        if (argc != 3) {
            printf("Usage: %s --syscall <name>\n",
                   argv[0]);
            return 1;
        }

        fd = open("/dev/phoenix", O_RDWR);

        if (fd < 0) {
            printf("Failed to open /dev/phoenix: %s\n",
                   strerror(errno));
            return 1;
        }

        if (ioctl(fd,
                  PHOENIX_IOCTL_SET_SYSCALL,
                  argv[2]) < 0) {
            printf("SET_SYSCALL ioctl failed: %s\n",
                   strerror(errno));
            close(fd);
            return 1;
        }

        printf("Phoenix syscall changed to %s.\n",
               argv[2]);

        close(fd);

        return 0;
    }

    /* Unknown option */
    printf("Unknown option: %s\n", argv[1]);

    printf("Usage:\n");
    printf("  %s --log\n", argv[0]);
    printf("  %s --off\n", argv[0]);
    printf("  %s --block\n", argv[0]);
    printf("  %s --syscall <name>\n", argv[0]);

    return 1;
}
