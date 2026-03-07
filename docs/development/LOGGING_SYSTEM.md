# Logging System Documentation

## Overview

The T-Disp-TOTP project uses a flexible logging system that supports both compile-time and runtime log level filtering. This allows you to:

- Enable all logs during development (DEBUG mode)
- Disable verbose logs in production (ERROR/CRITICAL only)
- Dynamically change log levels at runtime without reflashing

## Log Levels

The system supports 5 log levels (from most to least verbose):

| Level | Value | Description | Use Case |
|-------|-------|-------------|----------|
| `DEBUG` | 0 | Detailed diagnostic information | Development, troubleshooting |
| `INFO` | 1 | General informational messages | Normal operation tracking |
| `WARNING` | 2 | Warning messages for potential issues | Non-critical problems |
| `ERROR` | 3 | Error messages for failures | Production default |
| `CRITICAL` | 4 | Critical system failures | Severe errors only |

## Configuration

### Compile-Time Configuration (platformio.ini)

Set the default log level using the `LOG_LEVEL` build flag:

```ini
build_flags = 
    -DLOG_LEVEL=0  ; DEBUG mode - all logs enabled
```

**Common configurations:**

```ini
; Development build - all logs
-DLOG_LEVEL=0

; Testing build - INFO and above
-DLOG_LEVEL=1

; Production build - ERROR and CRITICAL only (recommended)
-DLOG_LEVEL=3

; Critical-only build - only CRITICAL logs
-DLOG_LEVEL=4
```

### Runtime Configuration

You can change the log level at runtime without reflashing:

```cpp
// Enable all logs (DEBUG and above)
LogManager::getInstance().setMinLogLevel(LogLevel::DEBUG);

// Enable INFO and above
LogManager::getInstance().setMinLogLevel(LogLevel::INFO);

// Enable only ERROR and CRITICAL (production mode)
LogManager::getInstance().setMinLogLevel(LogLevel::ERROR);

// Get current log level
LogLevel currentLevel = LogManager::getInstance().getMinLogLevel();
```

## Usage in Code

### Basic Logging

Use the convenience macros for logging:

```cpp
LOG_DEBUG("Component", "Detailed debug information");
LOG_INFO("Component", "General information");
LOG_WARNING("Component", "Warning message");
LOG_ERROR("Component", "Error occurred");
LOG_CRITICAL("Component", "Critical failure!");
```

### Example Output

```
[12345] DEBUG [WiFiManager]: Connecting to WiFi...
[12456] INFO [Main]: WiFi connected successfully
[12567] WARNING [BatteryManager]: Battery level low: 15%
[12678] ERROR [WebServer]: Failed to start server
[12789] CRITICAL [CryptoManager]: Encryption key corrupted!
```

### Log Filtering Behavior

When `LOG_LEVEL=3` (ERROR):

```cpp
LOG_DEBUG("Test", "Debug message");    // ❌ Not printed (0 < 3)
LOG_INFO("Test", "Info message");      // ❌ Not printed (1 < 3)
LOG_WARNING("Test", "Warning");        // ❌ Not printed (2 < 3)
LOG_ERROR("Test", "Error");            // ✅ Printed (3 >= 3)
LOG_CRITICAL("Test", "Critical");      // ✅ Printed (4 >= 3)
```

## Implementation Details

### Header File (include/log_manager.h)

```cpp
#ifndef LOG_LEVEL
#define LOG_LEVEL 3  // Default: ERROR level
#endif

class LogManager {
public:
    void setMinLogLevel(LogLevel level);
    LogLevel getMinLogLevel() const;
    
private:
    LogLevel minLevel = static_cast<LogLevel>(LOG_LEVEL);
};
```

### Implementation (src/log_manager.cpp)

```cpp
void LogManager::log(LogLevel level, const String& component, const String& message) {
    if (level < minLevel) return;  // Filter out logs below minimum level
    
    // Format and print to Serial
    String timestamp = String(millis());
    String levelStr = levelToString(level);
    Serial.printf("[%s] %s [%s]: %s\n", 
                  timestamp.c_str(), 
                  levelStr.c_str(), 
                  component.c_str(), 
                  message.c_str());
}
```

## Best Practices

### Development

1. **Use DEBUG level** during active development:
   ```ini
   -DLOG_LEVEL=0
   ```

2. **Add descriptive component names**:
   ```cpp
   LOG_DEBUG("WiFiManager", "Attempting connection to SSID: " + ssid);
   ```

3. **Use appropriate log levels**:
   - `DEBUG`: Variable values, function entry/exit
   - `INFO`: Successful operations, state changes
   - `WARNING`: Recoverable issues, deprecated features
   - `ERROR`: Failed operations, exceptions
   - `CRITICAL`: System failures, data corruption

### Production

1. **Use ERROR level** for production builds:
   ```ini
   -DLOG_LEVEL=3
   ```

2. **Keep ERROR/CRITICAL logs minimal** - they will always print

3. **Don't remove DEBUG/INFO logs** - they can be enabled at runtime if needed

### Runtime Debugging

If you need to debug a production device:

```cpp
// In setup() or via web interface
void enableDebugMode() {
    LogManager::getInstance().setMinLogLevel(LogLevel::DEBUG);
    LOG_INFO("System", "Debug mode enabled");
}

void disableDebugMode() {
    LogManager::getInstance().setMinLogLevel(LogLevel::ERROR);
    LOG_INFO("System", "Debug mode disabled");
}
```

## Performance Considerations

### Compile-Time Filtering

- Logs below `LOG_LEVEL` are **still compiled** into the binary
- They are filtered at runtime with a simple comparison: `if (level < minLevel) return;`
- Minimal performance impact (~1 comparison per log call)

### Memory Impact

- Each log call includes the format string in flash memory
- Consider using `#ifdef DEBUG` for extremely verbose logging if flash space is critical

### Serial Output

- Serial.printf() is relatively slow (~1ms per line at 115200 baud)
- In production (ERROR level), only critical logs are printed
- This reduces Serial overhead significantly

## Troubleshooting

### Logs not appearing

1. **Check log level**: Ensure `LOG_LEVEL` is set correctly in platformio.ini
2. **Check Serial Monitor**: Baud rate should be 115200
3. **Check runtime level**: Call `getMinLogLevel()` to verify current setting

### Too many logs

1. **Increase log level**: Change from DEBUG (0) to INFO (1) or WARNING (2)
2. **Filter by component**: Search Serial output for specific component names

### Production logs too verbose

1. **Set LOG_LEVEL=3** in platformio.ini for production builds
2. **Review ERROR/CRITICAL usage**: Ensure they're only used for actual errors

For questions or improvements, see the main project documentation.
