#include "boot_log.h"
#include <stdio.h>

void BootLog(const char *message) {
    fprintf(stderr, "[KXBOOT] %s\n", message);
}

void BootLogResult(const char *message, int result) {
    fprintf(stderr, "[KXBOOT] %s: 0x%08x\n", message, result);
}

void BootLogPtr(const char *message, const void *ptr) {
    fprintf(stderr, "[KXBOOT] %s: %p\n", message, ptr);
}

void BootLogReset() {
}
