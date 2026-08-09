#include "web_admin_manager.h"
#include "crypto_manager.h"
#include "log_manager.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

WebAdminManager& WebAdminManager::getInstance() {
    static WebAdminManager instance;
    return instance;
}

WebAdminManager::WebAdminManager() : _isRegistered(false), _failed_attempts(0), _lockout_until(0) {}

void WebAdminManager::begin() {
    LOG_INFO("WebAdminManager", "Initializing...");
    loadCredentials();
    loadLoginState();
    LOG_INFO("WebAdminManager", "Initialized. User registered: " + String(_isRegistered ? "Yes" : "No"));
}

void WebAdminManager::loadCredentials() {
    String webAdminPath = CryptoManager::getInstance().getSpacePath("web_admin");
    if (LittleFS.exists(webAdminPath)) {
        fs::File file = LittleFS.open(webAdminPath, "r");
        if (file) {
            String encrypted_base64 = file.readString();
            file.close();
            String json_string = CryptoManager::getInstance().decrypt(encrypted_base64);
            if (json_string.length() > 0) {
                JsonDocument doc;
                if (deserializeJson(doc, json_string) == DeserializationError::Ok) {
                    _username = doc["username"].as<String>();
                    _isRegistered = !_username.isEmpty();
                }
            }
        }
    } else {
        _isRegistered = false;
    }
}

void WebAdminManager::loadLoginState() {
    if (LittleFS.exists(LOGIN_STATE_FILE)) {
        fs::File file = LittleFS.open(LOGIN_STATE_FILE, "r");
        if (file) {
            JsonDocument doc;
            if (deserializeJson(doc, file) == DeserializationError::Ok) {
                _failed_attempts = doc["failed_attempts"] | 0;
                _lockout_until = doc["lockout_until"] | 0;
            }
            file.close();
        }
    }
}

void WebAdminManager::saveLoginState() {
    JsonDocument doc;
    doc["failed_attempts"] = _failed_attempts;
    doc["lockout_until"] = _lockout_until;
    fs::File file = LittleFS.open(LOGIN_STATE_FILE, "w");
    if (file) {
        serializeJson(doc, file);
        file.close();
    }
}

void WebAdminManager::resetLoginAttempts() {
    _failed_attempts = 0;
    _lockout_until = 0;
    saveLoginState();
    LOG_INFO("WebAdminManager", "Login attempts have been reset.");
}

void WebAdminManager::handleFailedLoginAttempt() {
    _failed_attempts++;
    LOG_WARNING("WebAdminManager", "Failed login attempt #" + String(_failed_attempts));
    
    if (_failed_attempts >= 5) {
        _lockout_until = millis() + 15 * 60 * 1000; // 15 минут
        LOG_CRITICAL("WebAdminManager", "5 failed attempts. Locked out for 15 minutes.");
    } else if (_failed_attempts == 4) {
        _lockout_until = millis() + 5 * 60 * 1000; // 5 минут
        LOG_WARNING("WebAdminManager", "4 failed attempts. Locked out for 5 minutes.");
    } else if (_failed_attempts == 3) {
        _lockout_until = millis() + 1 * 60 * 1000; // 1 минута
        LOG_WARNING("WebAdminManager", "3 failed attempts. Locked out for 1 minute.");
    } else {
        _lockout_until = 0;
    }
    saveLoginState();
}

unsigned long WebAdminManager::getLockoutTimeRemaining() {
    if (_lockout_until == 0) return 0;
    unsigned long current_time = millis();
    if (current_time < _lockout_until) {
        return (_lockout_until - current_time) / 1000;
    }
    return 0;
}

bool WebAdminManager::isRegistered() {
    return _isRegistered;
}

String WebAdminManager::getUsername() {
    return _username;
}

bool WebAdminManager::registerAdmin(const String& username, const String& password) {
    if (isRegistered()) return false;
    String hashedPassword = CryptoManager::getInstance().hashPassword(password);
    JsonDocument doc;
    doc["username"] = username;
    doc["hash"] = hashedPassword;
    String json_string;
    serializeJson(doc, json_string);
    String encrypted_base64 = CryptoManager::getInstance().encrypt(json_string);
    String webAdminPath = CryptoManager::getInstance().getSpacePath("web_admin");
    fs::File file = LittleFS.open(webAdminPath, "w");
    if (!file) return false;
    file.print(encrypted_base64);
    file.close();
    _username = username;
    _isRegistered = true;
    resetLoginAttempts();
    return true;
}

bool WebAdminManager::verifyCredentials(const String& username, const String& password) {
    // Эта функция теперь ТОЛЬКО проверяет пароль, без побочных эффектов.
    if (!isRegistered() || username != _username) {
        return false;
    }
    String webAdminPath = CryptoManager::getInstance().getSpacePath("web_admin");
    fs::File file = LittleFS.open(webAdminPath, "r");
    if (!file) return false;
    String encrypted_base64 = file.readString();
    file.close();
    String json_string = CryptoManager::getInstance().decrypt(encrypted_base64);
    if (json_string.length() == 0) return false;
    JsonDocument doc;
    deserializeJson(doc, json_string);
    String storedHash = doc["hash"].as<String>();
    return CryptoManager::getInstance().verifyPassword(password, storedHash);
}

bool WebAdminManager::changePassword(const String& newPassword) {
    if (!isRegistered()) return false;
    String newHashedPassword = CryptoManager::getInstance().hashPassword(newPassword);
    JsonDocument doc;
    doc["username"] = _username;
    doc["hash"] = newHashedPassword;
    String json_string;
    serializeJson(doc, json_string);
    String encrypted_base64 = CryptoManager::getInstance().encrypt(json_string);
    String webAdminPath = CryptoManager::getInstance().getSpacePath("web_admin");
    fs::File file = LittleFS.open(webAdminPath, "w");
    if (!file) return false;
    file.print(encrypted_base64);
    file.close();
    resetLoginAttempts();
    return true;
}
