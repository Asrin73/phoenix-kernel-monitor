#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

static const char *normalize_syscall(const char *event)
{
    if (strcmp(event, "__x64_sys_openat") == 0)
        return "openat";

    if (strcmp(event, "__x64_sys_read") == 0)
        return "read";

    if (strcmp(event, "__x64_sys_write") == 0)
        return "write";

    return NULL;
}

int main(void)
{
    int fd;
    char buffer[128];
    ssize_t bytes_read;
    const char *syscall_name;

    fd = open("/dev/phoenix", O_RDONLY);

    if (fd < 0) {
        perror("Failed to open /dev/phoenix");
        return 1;
    }

    bytes_read = read(fd, buffer, sizeof(buffer) - 1);

    if (bytes_read < 0) {
        perror("Failed to read from /dev/phoenix");
        close(fd);
        return 1;
    }

    buffer[bytes_read] = '\0';

    printf("Raw event from kernel: %s\n", buffer);

    syscall_name = normalize_syscall(buffer);

    if (syscall_name == NULL) {
        printf("Unknown syscall event.\n");
    } else {
        printf("Normalized syscall: %s\n", syscall_name);
    }

    close(fd);

    return 0;
}

