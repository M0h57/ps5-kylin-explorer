#include "boot_log.h"
#include <errno.h>
#include <fcntl.h>
#include <orbis/libkernel.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef KYLIN_MINIMAL_BOOT_TEST
#include "app.h"
#include "logger.h"
#include <sstream>

std::stringstream debugLogStream;
#endif


namespace {
constexpr int kJailbreakWaitAttempts = 50;
constexpr int kJailbreakWaitUs = 100000;
constexpr const char *kJailbreakTriggerPath = "/download0/etahen_jailbreak";

void IdleForever() {
    for (;;) {
        sceKernelSleep(1);
    }
}

int WriteAll(int fd, const char *buf, size_t len) {
    size_t offset = 0;
    while (offset < len) {
        const ssize_t written = write(fd, buf + offset, len - offset);
        if (written <= 0) {
            return -1;
        }
        offset += static_cast<size_t>(written);
    }
    return 0;
}

bool RequestKylinJailbreak() {
    char json[64];
    const int pid = static_cast<int>(getpid());
    const int len = snprintf(json, sizeof(json), "{\"PID\":%d}\n", pid);
    if (len <= 0 || static_cast<size_t>(len) >= sizeof(json)) {
        BootLog("jailbreak trigger json failed");
        return false;
    }

    BootLogResult("jailbreak pid", pid);
    unlink(kJailbreakTriggerPath);

    BootLog("jailbreak download0 mkdir begin");
    if (mkdir("/download0", 0777) != 0 && errno != EEXIST) {
        BootLogResult("jailbreak download0 mkdir errno", errno);
    } else {
        BootLog("jailbreak download0 mkdir ok");
    }

    BootLog("jailbreak trigger open begin");
    const int fd = open(kJailbreakTriggerPath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        BootLogResult("jailbreak trigger open failed", fd);
        BootLogResult("jailbreak trigger open errno", errno);
        return false;
    }

    if (WriteAll(fd, json, static_cast<size_t>(len)) < 0) {
        BootLogResult("jailbreak trigger write errno", errno);
        close(fd);
        return false;
    }
    close(fd);
    BootLog("jailbreak trigger write ok");

    for (int attempt = 1; attempt <= kJailbreakWaitAttempts; ++attempt) {
        struct stat st;
        if (stat(kJailbreakTriggerPath, &st) != 0) {
            BootLogResult("jailbreak trigger consumed attempt", attempt);
            return true;
        }
        sceKernelUsleep(kJailbreakWaitUs);
    }

    BootLog("jailbreak trigger still present");
    return false;
}
}

int main() {
    BootLogReset();
    BootLog("main begin");
    if (!RequestKylinJailbreak()) {
        BootLog("jailbreak request failed; idle");
        IdleForever();
    }
    BootLog("jailbreak request ok");
#ifdef KYLIN_MINIMAL_BOOT_TEST
    BootLog("minimal idle loop");
    IdleForever();
#else
    LoggerInit();
    App app;
    LOG_INFO("app constructed");
    if (!app.Init()) {
        LOG_INFO("main init failed");
        return -1;
    }
    LOG_INFO("main run begin");
    app.Run();
    return 0;
#endif
}
