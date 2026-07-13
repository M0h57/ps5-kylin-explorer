#ifndef KYLIN_EXPLORER_LOGGER_H
#define KYLIN_EXPLORER_LOGGER_H

#include <string>

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
};

void LoggerInit();
void LogMessage(LogLevel level, const char *fmt, ...);

#define LOG_INFO(...) LogMessage(LogLevel::Info, __VA_ARGS__)
#define LOG_ERROR(...) LogMessage(LogLevel::Error, __VA_ARGS__)

#ifdef KYLIN_DEBUG
#define LOG_DEBUG(...) Log(LogLevel::Debug, __VA_ARGS__)
#else
#define LOG_DEBUG(...) ((void)0)
#endif

#endif
