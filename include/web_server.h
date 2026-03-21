#ifndef WEBSERVER_MANAGER_H
#define WEBSERVER_MANAGER_H

#include <ESPAsyncWebServer.h>
#include "key_manager.h"
#include "splash_manager.h"
#include "pin_manager.h"
#include "config_manager.h"
#include "PasswordManager.h"
#include "crypto_manager.h"
#include "log_manager.h"
#include "totp_generator.h"
#include "wifi_manager.h"
#include "battery_manager.h"

#ifdef SECURE_LAYER_ENABLED
#include "secure_layer_manager.h"
#include "web_server_secure_integration.h"
#endif

#include "url_obfuscation_manager.h"
#include "url_obfuscation_integration.h"

// Forward declaration
class BleKeyboardManager;

class WebServerManager {
public:
    WebServerManager(KeyManager& keyManager, SplashScreenManager& splashManager, DisplayManager& displayManager, PinManager& pinManager, ConfigManager& configManager, PasswordManager& passwordManager, TOTPGenerator& totpGenerator);
    void start();
    void startConfigServer();
    void stop();
    void update();
    void resetActivityTimer();
    bool isRunning();
    void setBleKeyboardManager(BleKeyboardManager* bleManager);
    void setWifiManager(WifiManager* wifiManager);
    void setBatteryManager(BatteryManager* manager);
    void clearSession(); // Очистка сессии и CSRF токена

private:
    // Handler Functions
    void handleRoot(AsyncWebServerRequest *request);
    void handlePasswords(AsyncWebServerRequest *request);
    void handleAddKey(AsyncWebServerRequest *request);
    void handleDeleteKey(AsyncWebServerRequest *request);
    void handleAddPassword(AsyncWebServerRequest *request);
    void handleDeletePassword(AsyncWebServerRequest *request);
    void handleNotFound(AsyncWebServerRequest *request);
    void handleLogin(AsyncWebServerRequest *request);
    void handleLogout(AsyncWebServerRequest *request);
    void handlePinSettings(AsyncWebServerRequest *request);
    void handleUpdatePin(AsyncWebServerRequest *request);
    // ... and so on for all handlers

    // Internal Helper Functions
    bool isAuthenticated(AsyncWebServerRequest *request);
    bool verifyCsrfToken(AsyncWebServerRequest *request);
    String generateKeysTable();
    String generatePasswordsTable();
    String getAdminPasswordHash(); // <-- Added declaration

    AsyncWebServer server;
    KeyManager& keyManager;
    SplashScreenManager& splashManager;
    DisplayManager& displayManager;
    PinManager& pinManager;
    ConfigManager& configManager;
    PasswordManager& passwordManager;
    TOTPGenerator& totpGenerator;
    BleKeyboardManager* bleKeyboardManager = nullptr;
    WifiManager* wifiManager = nullptr;
    BatteryManager* batteryManager = nullptr;

#ifdef SECURE_LAYER_ENABLED
    SecureLayerManager& secureLayer = SecureLayerManager::getInstance();
#endif

    URLObfuscationManager& urlObfuscation = URLObfuscationManager::getInstance();

    String session_id;
    String session_csrf_token;
    unsigned long session_created_time = 0;
    
    // Protected handshake state
    bool secureHandshakeActive = false;
    String currentSecureClientId;
    unsigned long handshakeStartTime = 0;
    
    // Session management helpers
    void loadPersistentSession();
    void savePersistentSession();
    void regenerateCsrfTokenIfNeeded();
    
    // Protected handshake helpers
    bool shouldInitializeSecureSession(AsyncWebServerRequest* request);
    void initializeSecureSession(AsyncWebServerRequest* request);
    String generateBrowserClientId(AsyncWebServerRequest* request);
    void injectSecureInitScript(AsyncWebServerRequest* request, String& htmlContent);
    void clearSecureSession();

    unsigned long _lastActivityTimestamp;
    uint16_t _timeoutMinutes;
    bool _isRunning;
    bool _oneMinuteWarningShown;
};

#endif // WEBSERVER_MANAGER_H

