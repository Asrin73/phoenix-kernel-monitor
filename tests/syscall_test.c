#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(void)
{
    int fd;
    char buffer[100];
    const char *message = "Hello from Phoenix test!\n";

    /* Open a file */
    fd = open("phoenix_test.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);

    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("File opened successfully.\n");

    /* Write to the file */
    if (write(fd, message, strlen(message)) < 0) {
        perror("write");
        close(fd);
        return 1;
    }

    printf("Data written successfully.\n");

    /* Move back to the beginning of the file */
    lseek(fd, 0, SEEK_SET);

    /* Read from the file */
    ssize_t n = read(fd, buffer, sizeof(buffer) - 1);

    if (n < 0) {
        perror("read");
        close(fd);
        return 1;
    }

    buffer[n] = '\0';

    printf("Data read from file: %s", buffer);

    close(fd);

    return 0;
}
