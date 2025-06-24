// 文件名：mycat3.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

// 返回系统内存页大小
long io_blocksize() {
    long pagesize = sysconf(_SC_PAGESIZE);
    return (pagesize == -1) ? 4096 : pagesize;
}

// 对齐分配函数：返回页对齐的内存地址
char* align_alloc(size_t size) {
    void* ptr = NULL;
    long alignment = sysconf(_SC_PAGESIZE);
    if (alignment == -1) alignment = 4096;

    int ret = posix_memalign(&ptr, alignment, size);
    if (ret != 0) {
        return NULL;
    }
    return (char*)ptr;
}

// 对齐释放函数
void align_free(void* ptr) {
    free(ptr);  // posix_memalign 对应使用 free 即可
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
    char *buffer = align_alloc(bufsize);
    if (buffer == NULL) {
        fprintf(stderr, "缓冲区分配失败\n");
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
                align_free(buffer);
                close(fd);
                return EXIT_FAILURE;
            }
            nwritten += result;
        }
    }

    if (nread == -1) {
        fprintf(stderr, "读取失败: %s\n", strerror(errno));
    }

    align_free(buffer);
    close(fd);
    return (nread == -1) ? EXIT_FAILURE : EXIT_SUCCESS;
}
