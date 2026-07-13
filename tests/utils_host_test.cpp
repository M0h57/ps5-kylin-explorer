#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>

#include "../src/utils.h"

namespace {
void WriteFile(const std::string &path, const char *content) {
    FILE *file = fopen(path.c_str(), "wb");
    assert(file != NULL);
    fputs(content, file);
    fclose(file);
}
}

int main() {
    char size[32];
    FormatSize(size, sizeof(size), 0);
    assert(std::string(size) == "0 B");
    FormatSize(size, sizeof(size), 1023);
    assert(std::string(size) == "1023 B");
    FormatSize(size, sizeof(size), 1024);
    assert(std::string(size) == "1.00 KB");
    FormatSize(size, sizeof(size), 1024 * 1024);
    assert(std::string(size) == "1.00 MB");
    FormatSize(size, sizeof(size), 1024ULL * 1024 * 1024);
    assert(std::string(size) == "1.00 GB");

    char modified[32];
    FormatModifiedTime(modified, sizeof(modified), 0);
    assert(std::string(modified) == "--");
    FormatModifiedTime(modified, sizeof(modified), 1);
    assert(std::string(modified) != "--");

    assert(JoinPath("/", "data") == "/data");
    assert(JoinPath("/data", "file.txt") == "/data/file.txt");
    assert(ParentPath("/") == "/");
    assert(ParentPath("/data") == "/");
    assert(ParentPath("/data/file.txt") == "/data");
    assert(IsPathWithin("/data/file.txt", "/data"));
    assert(IsPathWithin("/data", "/data"));
    assert(IsPathWithin("/anything", "/"));
    assert(!IsPathWithin("/database", "/data"));

    char rootTemplate[] = "/tmp/kylin-utils-XXXXXX";
    const std::string root = mkdtemp(rootTemplate);
    const std::string filePath = JoinPath(root, "bytes.bin");
    WriteFile(filePath, "12345");
    assert(GetFileSize(filePath) == 5);
    assert(GetFileSize(JoinPath(root, "missing.bin")) == 0);

    printf("utils-host-tests-ok\n");
    return 0;
}
