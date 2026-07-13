#include "toast.h"
#include <orbis/libkernel.h>

// Resolved at runtime via sceKernelDlsym
static int (*sceSysUtilSendSystemNotificationWithText)(int messageType, char *message);

namespace {
bool TryResolve() {
    if (sceSysUtilSendSystemNotificationWithText != nullptr) return true;
    int lib = sceKernelLoadStartModule("libSceSysUtil.sprx", 0, nullptr, 0, 0, 0);
    if (lib < 0) return false;
    sceKernelDlsym(lib, "sceSysUtilSendSystemNotificationWithText",
                   reinterpret_cast<void **>(&sceSysUtilSendSystemNotificationWithText));
    return sceSysUtilSendSystemNotificationWithText != nullptr;
}
} // namespace

ToastManager::ToastManager() {
}

void ToastManager::Add(const std::string &message, ToastType type) {
    (void)type;
    if (!TryResolve()) return;
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "%s", message.c_str());
    sceSysUtilSendSystemNotificationWithText(0x81, buffer);
}

void ToastManager::Update() {
}

const std::vector<ToastNotification> &ToastManager::Toasts() const {
    static const std::vector<ToastNotification> empty;
    return empty;
}
