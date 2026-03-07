#include "log_manager.h"

LogManager& LogManager::getInstance() {
    static LogManager instance;
    return instance;
}

LogManager::LogManager() {}

void LogManager::begin() {
    logInfo("LogManager", "Serial logging system initialized");
}

void LogManager::log(LogLevel level, const String& component, const String& message) {
    if (level < minLevel) return;
    // Output to Serial Monitor
    String levelStr = levelToString(level);
    String timestamp = String(millis());
    Serial.printf("[%s] %s [%s]: %s\n", 
                  timestamp.c_str(), 
                  levelStr.c_str(), 
                  component.c_str(), 
                  message.c_str());
}

void LogManager::setMinLogLevel(LogLevel level) {
    minLevel = level;
}

LogLevel LogManager::getMinLogLevel() const {
    return minLevel;
}

void LogManager::logDebug(const String& component, const String& message) {
    log(LogLevel::DEBUG, component, message);
}

void LogManager::logInfo(const String& component, const String& message) {
    log(LogLevel::INFO, component, message);
}

void LogManager::logWarning(const String& component, const String& message) {
    log(LogLevel::WARNING, component, message);
}

void LogManager::logError(const String& component, const String& message) {
    log(LogLevel::ERROR, component, message);
}

void LogManager::logCritical(const String& component, const String& message) {
    log(LogLevel::CRITICAL, component, message);
}

String LogManager::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRIT";
        default: return "UNKNOWN";
    }
}
