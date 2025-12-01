/*
 * Backend support definitions for CUPS.
 *
 * Copyright © 2021 by OpenPrinting
 * Copyright © 2007-2021 by Apple Inc.
 * Copyright © 1997-2007 by Easy Software Products, all rights reserved.
 *
 * Licensed under Apache License v2.0.  See the file "LICENSE" for more
 * information.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdint.h>

#define BUFFER_SIZE 4096
#define DEFAULT_TMP_DIR "/data/service/el1/public/print_service/cups/spool/tmp"
#define MAX_JOB_ID_LEN 64
#define OUTPUT_PATH_BUF_SIZE 256

static int validate_args(int argc, char *argv[]) {
    fprintf(stderr, "CUPS backend start, argc = %d\n", argc);

    if (argc != 6 && argc != 7) {
        fprintf(stderr, "ERROR: invalid argc %d, only support 6 or 7\n", argc);
        fprintf(stderr, "Usage: %s job-id user title copies options [file]\n", argv[0]);
        return 1;
    }

    if (argc < 2 || strlen(argv[1]) > MAX_JOB_ID_LEN) {
        if (argc < 2) {
            fprintf(stderr, "ERROR: job id is missing\n");
        } else {
            fprintf(stderr, "ERROR: job id too long (max %d bytes)\n", MAX_JOB_ID_LEN);
        }
        return 1;
    }

    fprintf(stderr, "receive print job (argc = %d)\n", argc);
    return 0;
}

static const char *get_temp_dir(void) {
    const char *tmp_dir = getenv("TMPDIR");
    if (!tmp_dir) {
        fprintf(stderr, "get temp dir failed, use default dir\n");
        tmp_dir = DEFAULT_TMP_DIR;
        fprintf(stderr, "Use default temp dir: %s\n", tmp_dir);
    }
    return tmp_dir;
}

static int build_output_path(const char *tmp_dir, const char *job_id, char *output_file) {
    if (tmp_dir == NULL || job_id == NULL || output_file == NULL) {
        fprintf(stderr, "ERROR: invalid input parameters for path building (NULL pointer)\n");
        return 1;
    }

    // 跨平台：移除_TRUNCATE，手动校验长度
    int ret = snprintf_s(output_file,
                         OUTPUT_PATH_BUF_SIZE,
                         "%s/%s.pdf",
                         tmp_dir, job_id);

    // snprintf_s返回值：成功返回写入的字符数（不含\0），失败返回非0
    if (ret < 0 || ret >= OUTPUT_PATH_BUF_SIZE) {
        if (ret >= OUTPUT_PATH_BUF_SIZE) {
            fprintf(stderr, "ERROR: output path too long, max length: %d\n", OUTPUT_PATH_BUF_SIZE - 1);
        } else {
            fprintf(stderr, "ERROR: snprintf_s failed to build output path, err code: %d\n", ret);
        }
        return 1;
    }

    fprintf(stderr, "Output file path: %s\n", output_file);
    return 0;
}

static int open_input_source(int argc, char *argv[]) {
    int input_fd = -1;
    if (argc == 7) {
        input_fd = open(argv[6], O_RDONLY);
        if (input_fd < 0) {
            fprintf(stderr, "ERROR: open input file %s failed, err: %s\n", argv[6], strerror(errno));
            return -1;
        }
        fprintf(stderr, "Open input file success: %s\n", argv[6]);
    } else {
        input_fd = 0;
        fprintf(stderr, "Use stdin as input\n");
    }
    return input_fd;
}

static int write_output_file(int input_fd, const char *output_file) {
    FILE *fp_dest = fopen(output_file, "wb");
    if (fp_dest == NULL) {
        fprintf(stderr, "ERROR: open output file %s failed, err: %s\n", output_file, strerror(errno));
        return 1;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(input_fd, buffer, BUFFER_SIZE)) > 0) {
        if (fwrite(buffer, 1, (size_t)bytes_read, fp_dest) != (size_t)bytes_read) {
            fprintf(stderr, "ERROR: write to %s failed, err: %s\n", output_file, strerror(errno));
            fclose(fp_dest);
            return 1;
        }
    }

    if (bytes_read < 0) {
        fprintf(stderr, "ERROR: read input data failed, err: %s\n", strerror(errno));
        fclose(fp_dest);
        return 1;
    }

    fclose(fp_dest);
    return 0;
}

static void verify_output_file(const char *output_file) {
    struct stat file_stat;
    if (stat(output_file, &file_stat) == 0) {
        fprintf(stderr, "SUCCESS: File saved to %s, size: %ld bytes\n", output_file, file_stat.st_size);
    } else {
        fprintf(stderr, "WARNING: output file %s created but stat failed, err: %s\n", output_file, strerror(errno));
    }
}

int main(int argc, char *argv[]) {
    if (validate_args(argc, argv) != 0) {
        return 1;
    }

    const char *tmp_dir = get_temp_dir();

    char output_file[OUTPUT_PATH_BUF_SIZE];
    if (build_output_path(tmp_dir, argv[1], output_file) != 0) {
        return 1;
    }

    int input_fd = open_input_source(argc, argv);
    if (input_fd < 0) {
        return 1;
    }

    if (write_output_file(input_fd, output_file) != 0) {
        if (input_fd != 0) {
            close(input_fd);
        }
        return 1;
    }

    if (input_fd != 0) {
        close(input_fd);
    }

    verify_output_file(output_file);

    return 0;
}