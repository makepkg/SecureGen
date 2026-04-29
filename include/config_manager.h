#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"
#include "ui_themes.h"

class ConfigManager {
public:
    ConfigManager();
    void begin();
    Theme loadTheme();
    void saveTheme(Theme theme);
    
    // BLE Device Name functions
    String loadBleDeviceName();
    void saveBleDeviceName(const String& deviceName);

    // mDNS Hostname functions
    String loadMdnsHostname();
    void saveMdnsHostname(const String& hostname);
    
    // Startup Mode functions
    String getStartupMode();
    bool saveStartupMode(const String& mode);
    
    // Boot Mode functions (AP/Offline/WiFi default boot selection)
    String getBootMode();
    bool saveBootMode(const String& mode);
    
#ifdef BOARD_HAS_USB_HID
    String getDefaultHidMode();
    bool saveDefaultHidMode(const String& mode);
#endif
    
    // Web server configuration
    uint16_t getWebServerTimeout();
    void setWebServerTimeout(uint16_t timeoutMinutes);
    bool getWebServerAutoStart();
    void setWebServerAutoStart(bool autoStart);
    
    // Session duration configuration (5 modes)
    enum SessionDuration {
        UNTIL_REBOOT = 0,    // До ребута устройства
        ONE_HOUR = 1,        // 1 час
        SIX_HOURS = 6,       // 6 часов (по умолчанию)
        TWENTY_FOUR_HOURS = 24, // 24 часа
        THREE_DAYS = 72      // 3 дня
    };
    SessionDuration getSessionDuration();
    void setSessionDuration(SessionDuration duration);
    unsigned long getSessionLifetimeSeconds(); // Возвращает секунды для текущей настройки

    // Display Timeout functions
    uint16_t getDisplayTimeout();
    bool saveDisplayTimeout(uint16_t timeout);
    
    uint32_t getAutoLockTimeout();
    bool saveAutoLockTimeout(uint32_t timeout);

    String getTimezone();
    bool saveTimezone(const String& tz);

    // WiFi AP Password functions
    String loadApPassword();  // Загрузить пароль AP режима (default: "12345678", AES-256 encrypted)
    bool saveApPassword(const String& password);  // Сохранить пароль AP режима

private:
    // Internal state for configuration values
    Theme _currentTheme = Theme::DARK; // Default theme
    String _currentBleDeviceName = DEFAULT_BLE_DEVICE_NAME; // Default BLE name
    String _currentMdnsHostname = DEFAULT_MDNS_HOSTNAME; // Default mDNS hostname
    uint16_t _cachedDisplayTimeout = 30; // Default display timeout (cached)
    bool _displayTimeoutCached = false; // Flag to track if timeout is cached
    uint32_t _cachedAutoLockTimeout = 0; // Default auto lock timeout (cached, 0=Never)
    bool _autoLockTimeoutCached = false; // Flag to track if auto lock timeout is cached
    SessionDuration _currentSessionDuration = SIX_HOURS; // Default session duration
};

#endif // CONFIG_MANAGER_H
