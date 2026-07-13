#ifndef KYLIN_EXPLORER_BOOT_LOG_H
#define KYLIN_EXPLORER_BOOT_LOG_H

void BootLog(const char *message);
void BootLogResult(const char *message, int result);
void BootLogPtr(const char *message, const void *ptr);
void BootLogReset();

#endif
