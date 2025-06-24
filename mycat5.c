// 文件名：mycat5.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#define MULTIPLIER 64  // A 倍率

char* align_alloc(size_t size) {
    void* ptr = NULL;
    long alignment = sysconf(_SC_PAGESIZE);
    if (alignment == -1) alignment = 4096;
    int ret = posix_memalign(&ptr, alignment, size);
    return (ret == 0) ? (char*)ptr : NULL;
}

void align_free(void* ptr) {
    free(ptr);
}

size_t io_blocksize(const char* filepath) {
    struct stat st;
    long pagesize = sysconf(_SC_PAGESIZE);
    if (pagesize <= 0) pagesize = 4096;

    size_t fs_blocksize = pagesize;  // 默认使用页大小
    if (stat(filepath, &st) == 0 && (st.st_blksize & (st.st_blksize - 1)) == 0) {
        fs_blocksize = st.st_blksize;
    }

    return fs_blocksize * MULTIPLIER;  // 实验确定的最优倍率
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "用法: %s <文件名>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* filepath = argv[1];
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "无法打开文件 '%s': %s\n", filepath, strerror(errno));
        return EXIT_FAILURE;
    }

    size_t bufsize = io_blocksize(filepath);
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
