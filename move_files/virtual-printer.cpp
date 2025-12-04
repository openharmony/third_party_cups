/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

const size_t BUFFER_SIZE = 4096;
const std::string DEFAULT_TMP_DIR = "/data/service/el1/public/print_service/cups/spool/tmp";
const size_t OUTPUT_FILE_BUF_SIZE = 256;

static bool validate_args(int argc, char* argv[]) {
    if (argc != 6 && argc != 7) {
        fprintf(stderr, "ERROR: invalid argc %d, only 6 or 7 allowed\n", argc);
        fprintf(stderr, "Usage: %s job-id user title copies options [file]\n", argv[0]);
        return false;
    }
    return true;
}

static std::string get_temp_dir() {
    const char* tmp_dir_c = getenv("TMPDIR");
    std::string tmp_dir;

    if (tmp_dir_c == nullptr) {
        tmp_dir = DEFAULT_TMP_DIR;
    } else {
        tmp_dir = tmp_dir_c;
    }
    return tmp_dir;
}

static int build_output_path(const std::string& tmp_dir, const std::string& job_id, std::string& output_file) {
output_file = tmp_dir + "/" + job_id + ".pdf";

if (output_file.length() >= OUTPUT_FILE_BUF_SIZE) {
fprintf(stderr, "ERROR: path too long (need %zu bytes, max %zu)\n",
output_file.length(), OUTPUT_FILE_BUF_SIZE - 1);
return 1;
}

return 0;
}

static int open_input_source(int argc, char* argv[]) {
    int input_fd = -1;

    if (argc == 7) {
        input_fd = open(argv[6], O_RDONLY);
        if (input_fd < 0) {
            fprintf(stderr, "ERROR: open file %s failed: %s\n", argv[6], strerror(errno));
            return -1;
        }
    } else {
        input_fd = 0;
    }
    return input_fd;
}

static int write_output_file(int input_fd, const std::string& output_file) {
FILE* fp_dest = fopen(output_file.c_str(), "wb");
if (fp_dest == nullptr) {
fprintf(stderr, "ERROR: open output %s failed: %s\n", output_file.c_str(), strerror(errno));
return 1;
}

char buffer[BUFFER_SIZE];
ssize_t bytes_read;
while ((bytes_read = read(input_fd, buffer, BUFFER_SIZE)) > 0) {
size_t bytes_written = fwrite(buffer, 1, static_cast<size_t>(bytes_read), fp_dest);
if (bytes_written != static_cast<size_t>(bytes_read)) {
fprintf(stderr, "ERROR: write failed (wrote %zu of %zd bytes): %s\n",
bytes_written, bytes_read, strerror(errno));
fclose(fp_dest);
return 1;
}
}

if (bytes_read < 0) {
fprintf(stderr, "ERROR: read failed: %s\n", strerror(errno));
fclose(fp_dest);
return 1;
}

fclose(fp_dest);
return 0;
}

static void verify_output_file(const std::string& output_file) {
struct stat file_stat;
if (stat(output_file.c_str(), &file_stat) == 0) {
fprintf(stderr, "SUCCESS: File saved to %s, size: %ld bytes\n", output_file.c_str(), file_stat.st_size);
} else {
fprintf(stderr, "WARNING: stat %s failed: %s\n", output_file.c_str(), strerror(errno));
}
}

int main(int argc, char* argv[]) {
    if (!validate_args(argc, argv)) {
        return 1;
    }

    std::string tmp_dir = get_temp_dir();

    if (argc < 2 || !argv[1] || strlen(argv[1]) == 0) {
        fprintf(stderr, "ERROR: job id is missing\n");
        return 1;
    }
    std::string job_id = argv[1];

    std::string output_file;
    if (build_output_path(tmp_dir, job_id, output_file) != 0) {
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