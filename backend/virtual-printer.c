#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFFER_SIZE 4096  // 4KB 缓冲区

int main(int argc, char *argv[]) {
    // 1. 打印参数调试（改为 stderr）
    fprintf(stderr, "CUPS backend start, argc = %d\n", argc);
    for (int i = 0; i < argc; i++) {
        fprintf(stderr, "argv[%d] = %s\n", i, argv[i]);
    }

    // 2. 校验参数合法性
    if (argc != 6 && argc != 7) {
        fprintf(stderr, "ERROR: invalid argc %d, only support 6 or 7\n", argc);
        fprintf(stderr, "Usage: %s job-id user title copies options [file]\n", argv[0]);
        return 1;
    }

    fprintf(stderr, "receive print job (argc = %d)\n", argc);

    // 3. 基础校验：job ID 必传
    if (argc < 2) {
        fprintf(stderr, "ERROR: job id is missing\n");
        return 1;
    }

    // 4. 获取临时目录
    const char *tmp_dir = getenv("TMPDIR");
    if (!tmp_dir) {
        tmp_dir = "/data/service/el1/public/print_service/cups/spool/tmp";
        fprintf(stderr, "Use default temp dir: %s\n", tmp_dir);
    }

    // 5. 构造输出文件路径
    char output_file[256];
    snprintf(output_file, sizeof(output_file), "%s/%s.pdf", tmp_dir, argv[1]);
    fprintf(stderr, "Output file path: %s\n", output_file);

    // 6. 核心：区分数据来源（移除 copies 相关逻辑）
    int input_fd = -1;
    if (argc == 7) {
        // 场景1：argc=7 → 打开 argv[6] 指向的文件
        input_fd = open(argv[6], O_RDONLY);
        if (input_fd < 0) {
            fprintf(stderr, "ERROR: open input file %s failed, err: %s\n", argv[6], strerror(errno));
            return 1;
        }
        fprintf(stderr, "Open input file success: %s\n", argv[6]);
    } else {
        // 场景2：argc=6 → 从 stdin 读取
        input_fd = 0;
        fprintf(stderr, "Use stdin as input\n");
    }

    // 7. 打开输出文件（二进制写入）
    FILE *fp_dest = fopen(output_file, "wb");
    if (!fp_dest) {
        fprintf(stderr, "ERROR: open output file %s failed, err: %s\n", output_file, strerror(errno));
        if (input_fd != 0) close(input_fd);
        return 1;
    }

    // 8. 读取输入数据并写入输出文件
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(input_fd, buffer, BUFFER_SIZE)) > 0) {
        if (fwrite(buffer, 1, (size_t)bytes_read, fp_dest) != (size_t)bytes_read) {
            fprintf(stderr, "ERROR: write to %s failed, err: %s\n", output_file, strerror(errno));
            fclose(fp_dest);
            if (input_fd != 0) close(input_fd);
            return 1;
        }
    }

    // 9. 校验读取错误
    if (bytes_read < 0) {
        fprintf(stderr, "ERROR: read input data failed, err: %s\n", strerror(errno));
        fclose(fp_dest);
        if (input_fd != 0) close(input_fd);
        return 1;
    }

    // 10. 关闭文件句柄
    fclose(fp_dest);
    if (input_fd != 0) {
        close(input_fd);
    }

    // 11. 验证输出文件
    struct stat file_stat;
    if (stat(output_file, &file_stat) == 0) {
        fprintf(stderr, "SUCCESS: File saved to %s, size: %ld bytes\n", output_file, file_stat.st_size);
    } else {
        fprintf(stderr, "WARNING: output file %s created but stat failed, err: %s\n", output_file, strerror(errno));
    }

    return 0;
}