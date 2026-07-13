#include "utils.h"

#include <cstdio>
#include <cinttypes>
#include <ctime>
#include <sys/stat.h>

void FormatSize(char *output, size_t size, uint64_t bytes) {
    const double kKB = 1024.0;
    const double kMB = 1024.0 * 1024;
    const double kGB = 1024.0 * 1024 * 1024;

    if (bytes >= (uint64_t)kGB) {
        snprintf(output, size, "%.2f GB", (double)bytes / kGB);
    } else if (bytes >= (uint64_t)kMB) {
        snprintf(output, size, "%.2f MB", (double)bytes / kMB);
    } else if (bytes >= (uint64_t)kKB) {
        snprintf(output, size, "%.2f KB", (double)bytes / kKB);
    } else {
        snprintf(output, size, "%" PRIu64 " B", bytes);
    }
}

void FormatModifiedTime(char *output, size_t size, uint64_t timestamp) {
    if (timestamp == 0) {
        snprintf(output, size, "--");
        return;
    }
    const time_t value = static_cast<time_t>(timestamp);
    struct tm parsed;
    if (localtime_r(&value, &parsed) == NULL) {
        snprintf(output, size, "--");
        return;
    }
    strftime(output, size, "%Y-%m-%d %H:%M", &parsed);
}

bool IsMounted(const char *path) {
    struct stat mountStat;
    if (stat(path, &mountStat) != 0) return false;
    if (!S_ISDIR(mountStat.st_mode)) return false;

    struct stat parentStat;
    if (stat("/mnt", &parentStat) != 0) return false;

    return mountStat.st_dev != parentStat.st_dev;
}

std::string JoinPath(const std::string &directory, const std::string &name) {
    return directory == "/" ? directory + name : directory + "/" + name;
}

std::string ParentPath(const std::string &path) {
    if (path.empty() || path == "/") {
        return "/";
    }
    const size_t separator = path.find_last_of('/');
    return separator == 0 ? "/" : path.substr(0, separator);
}

bool IsPathWithin(const std::string &path, const std::string &root) {
    return root == "/" || path == root ||
           (path.size() > root.size() && path.find(root + "/") == 0);
}

uint64_t GetFileSize(const std::string &path) {
    FILE *fp = fopen(path.c_str(), "rb");
    if (fp == NULL) return 0;
    fseek(fp, 0, SEEK_END);
    long tellSize = ftell(fp);
    fclose(fp);
    return (tellSize > 0) ? static_cast<uint64_t>(tellSize) : 0;
}
