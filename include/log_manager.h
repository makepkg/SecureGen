#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <deque>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    CRITICAL = 4
};

struct LogEntry {
    unsigned long timestamp;
    LogLevel level;
    String component;
    String message;
};

#ifndef LOG_LEVEL
#define LOG_LEVEL 3  // 0=DEBUG 1=INFO 2=WARNING 3=ERROR 4=CRITICAL
#endif

class LogManager {
public:
    static LogManager& getInstance();
    
    void begin();
    void log(LogLevel level, const String& component, const String& message);
    void logDebug(const String& component, const String& message);
    void logInfo(const String& component, const String& message);
    void logWarning(const String& component, const String& message);
    void logError(const String& component, const String& message);
    void logCritical(const String& component, const String& message);
    
    void setMinLogLevel(LogLevel level);
    LogLevel getMinLogLevel() const;

private:
    LogManager();
    LogManager(const LogManager&) = delete;
    void operator=(const LogManager&) = delete;
    
    String levelToString(LogLevel level);
    LogLevel minLevel = static_cast<LogLevel>(LOG_LEVEL);
};

// Convenience macros for easy logging
#define LOG_DEBUG(component, message) LogManager::getInstance().logDebug(component, message)
#define LOG_INFO(component, message) LogManager::getInstance().logInfo(component, message)
#define LOG_WARNING(component, message) LogManager::getInstance().logWarning(component, message)
#define LOG_ERROR(component, message) LogManager::getInstance().logError(component, message)
#define LOG_CRITICAL(component, message) LogManager::getInstance().logCritical(component, message)

#endif // LOG_MANAGER_H
