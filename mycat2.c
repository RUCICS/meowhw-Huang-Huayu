// 文件名：mycat2.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

long io_blocksize() {
    long pagesize = sysconf(_SC_PAGESIZE);
    if (pagesize == -1) {
        // fallback
        return 4096;
    }
    return pagesize;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "用法: %s <文件名>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "无法打开文件 '%s': %s\n", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }

    long bufsize = io_blocksize();
    char *buffer = malloc(bufsize);
    if (buffer == NULL) {
        fprintf(stderr, "分配缓冲区失败\n");
        close(fd);
        return EXIT_FAILURE;
    }

    ssize_t nread;
    while ((nread = read(fd, buffer, bufsize)) > 0) {
        ssize_t nwritten = 0;
        while (nwritten < nread) {
            ssize_t result = write(STDOUT_FILENO, buffer + nwritten, nread - nwritten);
            if (result == -1) {
                fprintf(stderr, "写入失败: %s\n", strerror(errno));
                free(buffer);
                close(fd);
                return EXIT_FAILURE;
            }
            nwritten += result;
        }
    }

    if (nread == -1) {
        fprintf(stderr, "读取失败: %s\n", strerror(errno));
    }

    free(buffer);
    close(fd);
    return (nread == -1) ? EXIT_FAILURE : EXIT_SUCCESS;
}
