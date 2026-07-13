#include "logger.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <mutex>

#ifdef KYLIN_DEBUG
#ifdef __ORBIS__
#include <orbis/libkernel.h>
#endif
#endif

namespace {
const char *kLogPath = "/data/kylinexplorer/kylin-explorer.log";
const size_t kMaxLogBytes = 256 * 1024;
const unsigned kRotateCount = 3;
std::mutex g_loggerMutex;
bool g_initialized = false;

void EnsureDirectory(const char *path) {
    if (path == nullptr || path[0] == '\0') return;
    if (mkdir(path, 0777) != 0 && errno != EEXIST) return;
}

void EnsureParentDirectory(const char *file_path) {
    char path[512];
    snprintf(path, sizeof(path), "%s", file_path);
    char *slash = strrchr(path, '/');
    if (slash == nullptr || slash == path) return;

    *slash = '\0';
    slash = strchr(path + 1, '/');
    while (slash != nullptr) {
        *slash = '\0';
        EnsureDirectory(path);
        *slash = '/';
        slash = strchr(slash + 1, '/');
    }
    EnsureDirectory(path);
}

const char *LevelName(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warn: return "WARN";
        case LogLevel::Error: return "ERROR";
        default: return "UNKNOWN";
    }
}

void FormatTimestamp(char *out, size_t out_size) {
    struct timeval tv;
    struct tm tm_value;
    if (out_size == 0) return;
    memset(&tm_value, 0, sizeof(tm_value));
    gettimeofday(&tv, nullptr);
    localtime_r(&tv.tv_sec, &tm_value);
    snprintf(out, out_size, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             tm_value.tm_year + 1900, tm_value.tm_mon + 1, tm_value.tm_mday,
             tm_value.tm_hour, tm_value.tm_min, tm_value.tm_sec,
             static_cast<int>(tv.tv_usec / 1000));
}

void BuildRotatedPath(const char *base_path, unsigned index, char *out, size_t out_size) {
    snprintf(out, out_size, "%s.%u", base_path, index);
}

void RotateLocked(const char *path) {
    if (path == nullptr || path[0] == '\0') return;
    if (kRotateCount == 0) {
        remove(path);
        return;
    }

    char src[512];
    char dst[512];
    for (unsigned index = kRotateCount; index > 0; --index) {
        BuildRotatedPath(path, index, dst, sizeof(dst));
        if (index == 1) {
            snprintf(src, sizeof(src), "%s", path);
        } else {
            BuildRotatedPath(path, index - 1, src, sizeof(src));
        }
        if (index == kRotateCount) {
            remove(dst);
        }
        rename(src, dst);
    }
}

void MaybeRotateLocked(const char *path, size_t incoming_bytes) {
    struct stat st;
    if (path == nullptr || path[0] == '\0') return;
    if (stat(path, &st) != 0) return;
    if (static_cast<uint64_t>(st.st_size) + incoming_bytes < kMaxLogBytes) return;
    RotateLocked(path);
}

} // namespace

void LoggerInit() {
    std::lock_guard<std::mutex> lock(g_loggerMutex);
    if (!g_initialized) {
#ifndef KYLIN_DEBUG
        EnsureParentDirectory(kLogPath);
#endif
        g_initialized = true;
    }
}

void LogMessage(LogLevel level, const char *fmt, ...) {
    char timestamp[32];
    char message[3072];
    char line[4096];
    va_list args;

    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    FormatTimestamp(timestamp, sizeof(timestamp));
    snprintf(line, sizeof(line), "%s %-5s %s", timestamp, LevelName(level), message);

#ifdef KYLIN_DEBUG
#ifdef __ORBIS__
    static char out[4096];
    snprintf(out, sizeof(out), "%s\n", line);
    sceKernelDebugOutText(0, out);
#else
    fprintf(stderr, "%s\n", line);
#endif
#else
    std::lock_guard<std::mutex> lock(g_loggerMutex);
    if (!g_initialized) {
        EnsureParentDirectory(kLogPath);
        g_initialized = true;
    }

    size_t line_len = strlen(line) + 1;
    MaybeRotateLocked(kLogPath, line_len);

    FILE *fp = fopen(kLogPath, "ab");
    if (fp != nullptr) {
        fprintf(fp, "%s\n", line);
        fclose(fp);
    }
#endif
}
