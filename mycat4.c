// 文件名：mycat4.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

// 分配页对齐的缓冲区
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

// 同时考虑页大小和文件系统块大小，返回两者的 LCM 或 max
size_t io_blocksize(const char* filepath) {
    struct stat st;
    long pagesize = sysconf(_SC_PAGESIZE);
    if (pagesize <= 0) pagesize = 4096;

    if (stat(filepath, &st) == -1) {
        perror("stat失败，使用默认页大小");
        return (size_t)pagesize;
    }

    size_t fs_blocksize = (size_t)st.st_blksize;

    // 如果文件系统块大小不是 2 的整数次幂，则 fallback
    if ((fs_blocksize & (fs_blocksize - 1)) != 0) {
        return (size_t)pagesize;  // fallback: 非2次幂块大小视为不可信
    }

    // 使用两个值中的较大者（或 LCM，可选）
    return (fs_blocksize > (size_t)pagesize) ? fs_blocksize : (size_t)pagesize;
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
