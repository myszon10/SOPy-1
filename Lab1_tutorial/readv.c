#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int main() {
    int fd = open("input.txt", O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    struct iovec iov[2];
    char buf1[5];
    char buf2[10];

    iov[0].iov_base = buf1;
    iov[0].iov_len = sizeof(buf1) - 1;
    iov[1].iov_base = buf2;
    iov[1].iov_len = sizeof(buf2) - 1;

    ssize_t bytes_read = readv(fd, iov, 2);
    if (bytes_read == -1) {
        perror("readv");
    } else {
        buf1[4] = '\0';  // końcowy znak null dla bufora 1
        buf2[9] = '\0';  // końcowy znak null dla bufora 2
        printf("Odczytano %zd bajtów: \"%s\" i \"%s\"\n", bytes_read, buf1, buf2);
    }

    close(fd);
    return 0;
}
