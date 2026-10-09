#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int main(void)
{
    int fd;

    char buffer[128];

    ssize_t bytes_read;

    /*
     * Open the Phoenix device.
     */
    fd = open("/dev/phoenix", O_RDONLY);

    if (fd < 0) {
        perror("Failed to open /dev/phoenix");
        return 1;
    }

    /*
     * Read data from the kernel device.
     */
    bytes_read = read(fd,
                      buffer,
                      sizeof(buffer) - 1);

    if (bytes_read < 0) {
        perror("Failed to read from /dev/phoenix");

        close(fd);

        return 1;
    }

    /*
     * Add the string terminator.
     */
    buffer[bytes_read] = '\0';

    /*
     * Print the event received from the kernel.
     */
    printf("Received from kernel: %s",
           buffer);

    /*
     * Close the device.
     */
    close(fd);

    return 0;
}
