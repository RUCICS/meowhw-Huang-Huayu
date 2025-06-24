// 文件名：mycat1.c

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "用法: %s <文件名>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "无法打开文件 '%s': %s\n", argv[1], strerror(errno));
        exit(EXIT_FAILURE);
    }

    char ch;
    ssize_t n;
    while ((n = read(fd, &ch, 1)) > 0) {
        if (write(STDOUT_FILENO, &ch, 1) != 1) {
            fprintf(stderr, "写入标准输出失败: %s\n", strerror(errno));
            close(fd);
            exit(EXIT_FAILURE);
        }
    }

    if (n == -1) {
        fprintf(stderr, "读取文件失败: %s\n", strerror(errno));
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
    return 0;
}
