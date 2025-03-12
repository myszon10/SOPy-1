#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int main() {
    int fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    struct iovec iov[2];
    char part1[] = "Hello, ";
    char part2[] = "world!\n";

    iov[0].iov_base = part1;
    iov[0].iov_len = sizeof(part1) - 1;  // bez końcowego '\0'
    iov[1].iov_base = part2;
    iov[1].iov_len = sizeof(part2) - 1;

    ssize_t bytes_written = writev(fd, iov, 2);
    if (bytes_written == -1) {
        perror("writev");
    } else {
        printf("Zapisano %zd bajtów\n", bytes_written);
    }

    close(fd);
    return 0;
}
