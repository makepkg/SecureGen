#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <TFT_eSPI.h>
#include "animation_manager.h"
#include "ui_themes.h" // Include new theme definitions

// Режимы запуска устройства (AP/Offline/WiFi)
enum class StartupMode {
    AP_MODE,        // Access Point режим
    OFFLINE_MODE,   // Полностью offline (WiFi отключен)
    WIFI_MODE       // Подключение к WiFi (default)
};

class DisplayManager {
public:
    enum class HeaderState { INTRO, STATIC, CHARGING };

    DisplayManager();
    void initForSplash(); // Early init for splash screen (PWM + TFT only, no fill)
    void init();
    void update(); 
    void updateHeader();
    
    void drawLayout(const String& serviceName, int batteryPercentage, bool isCharging, bool isWebServerOn); 
    void drawPasswordLayout(const String& name, const String& password, int batteryPercentage, bool isCharging, bool isWebServerOn, uint8_t strength = 0, bool isDuplicate = false, bool isPin = false, bool isName = false);
    void updateBatteryStatus(int percentage, bool isCharging);
    void updateClockStatus();
    void updateTOTPCode(const String& code, int timeRemaining, int period = 30);
    void turnOff();
    void turnOn();
    void setBrightness(uint8_t brightness); // Set backlight brightness (0-255) for fade effects
    bool promptWebServerSelection(); // New function for user prompt
    StartupMode promptModeSelection(StartupMode defaultMode = StartupMode::WIFI_MODE); // Промптинг выбора режима (AP/Offline/WiFi)

    void drawNoItemsPage(const String& text);
    void drawBleInitLoader(int progress);
#ifdef BOARD_HAS_USB_HID
    bool drawHidPrompt(bool defaultIsBle);
    void drawUsbHidPage(const String& passwordName, const String& status);
    void resetUsbHidPage();
    void setUsbHidMode(bool active);
private:
    String _usbHidLastStatus = "";
    bool _usbHidMode = false;
public:
#endif
    void drawGenericLoader(int progress, const String& text); // Новый универсальный лоадер
    void drawHOTPLoader(int progress); // HOTP loader progress 0-100
    void hideLoader(); // Скрыть лоадер и сбросить состояние
    void resetLoaderState(); // Reset loader flags without clearing screen
    void eraseLoaderArea(); // Erase loader bar area and reset flags
    bool isLoaderActive() const { return _loaderActive; } // Проверить активность лоадера
    void drawBleAdvertisingPage(const String& deviceName, const String& status, int timeLeft);
    void drawBleConfirmPage(const String& passwordName, const String& password, const String& deviceName);
    void drawBleSendingPage();
    void drawBleResultPage(bool success);
    
    // QR Code display
    void showQRCode(const String& text, int timeoutSeconds = 30);
    void hideQRCode();
    bool isQRCodeActive() const { return _qrCodeActive; }
    void requestShowQRCode(const String& text, int timeoutSeconds = 30); // Thread-safe request
    void updateQRTimer(int secondsRemaining); // Update timer without full redraw

    void setKeySwitched(bool switched) { _isKeySwitched = switched; } // <-- ADDED
    bool isCharging() const { return _isCharging; }

    void setTheme(Theme theme); // New method to set the theme
    const ThemeColors* getCurrentThemeColors() const { return _currentThemeColors; } // Get current theme colors

    // Deprecated, but kept for compatibility with other code
    void showMessage(const String& text, int x, int y, bool isError = false, int size = 1);
    void showMessage(const String& text, int x, int y, bool isError, int size, bool inverted);
    void updateMessage(const String& text, int x, int y, int size = 2); // Update text without full redraw
    void clearMessageArea(int x, int y, int width, int height); // Clear specific area
    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    TFT_eSPI* getTft();

private:
    // New state machine for TOTP display
    enum class TotpState { IDLE, SCRAMBLING, REVEALING };

    void drawBatteryOnSprite(int percentage, bool isCharging, int chargingValue = 0);
    void drawClockOnSprite();
    void drawTotpContainer();
    void drawTotpText(const String& textToDraw);
    void drawLockIcons(int x, int y, uint8_t strength);
    void drawTextBadge(int x, int y, const char* label, uint16_t bg);
    void drawDupBadge(int x, int y);

    TFT_eSPI tft;
    AnimationManager animationManager;
    TFT_eSprite headerSprite;
    TFT_eSprite batterySprite = TFT_eSprite(&tft);
    TFT_eSprite totpContainerSprite;
    TFT_eSprite totpSprite;
    const ThemeColors* _currentThemeColors; // Pointer to the active theme colors

    // State Machine Variables
    HeaderState _headerState = HeaderState::STATIC;
    String _currentServiceName;
    int _currentBatteryPercentage = 0;
    bool _isCharging = false;
    bool _isWebServerOn = false;

    // Animation-specific variables
    unsigned long _introAnimStartTime = 0;
    unsigned long _chargingAnimStartTime = 0;

    // Variables for flicker-free TOTP updates
    String lastDisplayedCode;
    int lastTimeRemaining;

    // --- New variables for the premium TOTP animation ---
    TotpState _totpState = TotpState::IDLE;
    unsigned long _totpAnimStartTime = 0;
    String _newCode;
    String _currentCode;
    String _lastDrawnTotpString;
    unsigned long _lastScrambleFrameTime = 0;
    bool _totpContainerNeedsRedraw = true;
    bool _isKeySwitched = false;
    
    // Лоадер состояние
    bool _loaderActive = false;
    String _lastLoaderText = "";
    int _lastLoaderProgress = -1;

    // Состояние для страницы "No Items"
    bool _isNoItemsPageActive = false;
    
    // QR Code состояние
    bool _qrCodeActive = false;
    unsigned long _qrCodeTimeout = 0;
    int _qrCodeTimerY = 0; // Y позиция таймера
    int _lastQRTimerSeconds = -1; // Последнее значение таймера
    
    // QR Code request (thread-safe)
    bool _qrCodeRequested = false;
    String _qrCodeText = "";
    int _qrCodeTimeoutSeconds = 30;
};

#endif // DISPLAY_MANAGER_H