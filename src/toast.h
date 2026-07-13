#ifndef KYLIN_EXPLORER_TOAST_H
#define KYLIN_EXPLORER_TOAST_H

#include <string>
#include <vector>
#include <queue>

enum class ToastType {
    Success,
    Info,
    Warning,
    Error
};

struct ToastNotification {
    std::string message;
    ToastType type;
    int durationFrames;
    int elapsedFrames;
    float currentY;
    float targetY;
    float slideX;   // pre-computed horizontal slide offset
};

class ToastManager {
public:
    ToastManager();
    void Add(const std::string &message, ToastType type = ToastType::Success);
    void Update();
    const std::vector<ToastNotification> &Toasts() const;

private:
    std::vector<ToastNotification> toasts_;
    std::queue<std::pair<std::string, ToastType>> queue_;
};

#endif
