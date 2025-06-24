// 文件名：mycat6.c
#define _POSIX_C_SOURCE 200112L  // 开启 posix_fadvise 支持

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>      // for posix_fadvise
#include <sys/types.h>

#define MULTIPLIER 64  // 从任务5实验确定的最优倍率

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

    size_t fs_blocksize = pagesize;
    if (stat(filepath, &st) == 0 && (st.st_blksize & (st.st_blksize - 1)) == 0) {
        fs_blocksize = st.st_blksize;
    }

    return fs_blocksize * MULTIPLIER;
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

    // ⭐ 设置 fadvise：告知操作系统我们是顺序读取
    if (posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL) != 0) {
        fprintf(stderr, "posix_fadvise 设置失败（非致命）\n");
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
