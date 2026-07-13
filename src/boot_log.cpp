#include "boot_log.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <orbis/libkernel.h>

namespace {
const char *kBootLogPath = "/user/app/KYLN00002/kxboot.log";

void AppendLine(const char *line) {
    const int fd = sceKernelOpen(kBootLogPath, O_WRONLY | O_CREAT | O_APPEND, 0777);
    if (fd < 0) return;
    sceKernelWrite(fd, line, strlen(line));
    sceKernelFsync(fd);
    sceKernelClose(fd);
}
}

void BootLog(const char *message) {
    char line[768];
    snprintf(line, sizeof(line), "[KXBOOT] %s\n", message);
#ifdef KYLIN_DEBUG
    sceKernelDebugOutText(0, line);
    printf("%s", line);
#else
    AppendLine(line);
#endif
}

void BootLogResult(const char *message, int result) {
    char line[768];
    snprintf(line, sizeof(line), "[KXBOOT] %s: 0x%08x\n", message, result);
#ifdef KYLIN_DEBUG
    sceKernelDebugOutText(0, line);
    printf("%s", line);
#else
    AppendLine(line);
#endif
}

void BootLogPtr(const char *message, const void *ptr) {
    char line[768];
    snprintf(line, sizeof(line), "[KXBOOT] %s: %p\n", message, ptr);
#ifdef KYLIN_DEBUG
    sceKernelDebugOutText(0, line);
    printf("%s", line);
#else
    AppendLine(line);
#endif
}

void BootLogReset() {
#ifdef KYLIN_DEBUG
    (void)0;
#else
    const int fd = sceKernelOpen(kBootLogPath, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (fd < 0) return;
    sceKernelFsync(fd);
    sceKernelClose(fd);
#endif
}
