#include "PasswordManager.h"
#include "mbedtls/sha256.h"
#include <ArduinoJson.h>
#include "LittleFS.h"
#include "config.h"
#include "crypto_manager.h"
#include "log_manager.h"
#include <algorithm>
#include <map>

PasswordManager::PasswordManager() {
    // Constructor is now empty
}

void PasswordManager::begin() {
    LOG_INFO("PasswordManager", "Initializing...");
    if (loadPasswords()) {
        LOG_INFO("PasswordManager", "Initialized successfully");
        if (_needsSave) {
            savePasswords();
            _needsSave = false;
            LOG_INFO("PasswordManager", "Migrated legacy entries: strength/hash computed");
        }
    } else {
        LOG_ERROR("PasswordManager", "Failed to load passwords during initialization");
    }
}

// Compute first 8 bytes of SHA256(password) as 16-char hex string
// Used for duplicate detection — no full password recoverable from this
String PasswordManager::computePwHash(const String& password) {
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0); // 0 = SHA256
    mbedtls_sha256_update(&ctx,
                          (const unsigned char*)password.c_str(), password.length());
    unsigned char hash[32];
    mbedtls_sha256_finish(&ctx, hash);
    mbedtls_sha256_free(&ctx);
    
    // Return first 8 bytes as hex (16 chars) — enough for duplicate detection
    String result = "";
    for (int i = 0; i < 8; i++) {
        if (hash[i] < 16) result += "0";
        result += String(hash[i], HEX);
    }
    return result;
}

// Compute password strength score (mirrors JS calculatePasswordStrength logic)
// Returns 1 (weak), 2 (medium), 3 (strong)
uint8_t PasswordManager::computeStrength(const String& password) {
    if (password.length() == 0) return 1;
    
    int len = password.length();
    int score = 0;
    
    // Length score (mirrors JS)
    if (len == 1) score = 1;
    else if (len <= 4)  score = 1 + (len - 1) * 2;
    else if (len <= 8)  score = 7 + (len - 4) * 3;
    else if (len <= 12) score = 19 + (len - 8) * 4;
    else if (len <= 20) score = 35 + (len - 12) * 2;
    else if (len <= 32) score = 51 + (len - 20) * 1;
    else                score = 63 + min((int)(len - 32), 32) / 2;
    
    // Character type bonus
    bool hasLower   = false, hasUpper = false,
         hasNumber  = false, hasSpecial = false;
    for (char c : password) {
        if (c >= 'a' && c <= 'z') hasLower = true;
        else if (c >= 'A' && c <= 'Z') hasUpper = true;
        else if (c >= '0' && c <= '9') hasNumber = true;
        else hasSpecial = true;
    }
    int types = (hasLower?1:0)+(hasUpper?1:0)+(hasNumber?1:0)+(hasSpecial?1:0);
    if (types == 2) score += 5;
    else if (types == 3) score += 10;
    else if (types == 4) score += 15;
    
    // Excellence bonus
    if (len >= 20 && types == 4) score += 5;
    
    if (score > 100) score = 100;
    
    // Map to 3 levels
    if (score >= 50) return 3; // strong / encryption
    if (score >= 25) return 2; // medium
    return 1;                  // weak
}

bool PasswordManager::addPassword(const String& name, const String& password, const String& category, 
                                  bool auto_send, const String& login, bool send_login, const String& nav_mode, 
                                  int login_delay_ms, bool wildcard, int wildcard_len) {
    if (name.isEmpty() || password.isEmpty()) {
        LOG_WARNING("PasswordManager", "Cannot add password with empty name or value");
        return false;
    }
    if (wildcard) {
        for (const auto& pwd : passwords) {
            if (pwd.getWildcard()) {
                LOG_WARNING("PasswordManager", "Wildcard entry already exists, rejecting duplicate");
                return false;
            }
        }
    }
    // Найти максимальный порядок для нового пароля
    int maxOrder = 0;
    for (const auto& pwd : passwords) {
        if (pwd.order > maxOrder) maxOrder = pwd.order;
    }
    PasswordEntry newPassword;
    newPassword.name = name;
    newPassword.password = password;
    newPassword.category = category;
    newPassword.setAutoSend(auto_send);
    newPassword.setSendLogin(send_login);
    newPassword.login = login;
    newPassword.nav_mode = nav_mode;
    newPassword.login_delay_ms = login_delay_ms;
    newPassword.setWildcard(wildcard);
    newPassword.wildcard_len = wildcard_len;
    newPassword.order = maxOrder + 1;
    // Compute strength and hash before storing
    newPassword.strength = computeStrength(password);
    newPassword.pw_hash  = computePwHash(password);
    passwords.push_back(newPassword);
    LOG_INFO("PasswordManager", "Added password entry: [HIDDEN]");
    bool success = savePasswords();
    if (!success) {
        LOG_ERROR("PasswordManager", "Failed to save passwords after adding entry");
    }
    return success;
}

bool PasswordManager::updatePassword(int index, const String& name, const String& password, const String& category, 
                                     bool auto_send, const String& login, bool send_login, const String& nav_mode, 
                                     int login_delay_ms, bool wildcard, int wildcard_len) {
    if (index < 0 || index >= passwords.size()) {
        LOG_WARNING("PasswordManager", "Invalid password index for update: " + String(index));
        return false;
    }
    if (passwords[index].getWildcard()) {
        // Wildcard entries are system-managed: name/password/category/etc.
        // are never user-editable. Only the generated-password length can
        // be changed here.
        passwords[index].wildcard_len = wildcard_len;
        LOG_INFO("PasswordManager", "Updated wildcard entry length at index " + String(index));
        bool wildcardSuccess = savePasswords();
        if (!wildcardSuccess) {
            LOG_ERROR("PasswordManager", "Failed to save passwords after wildcard length update");
        }
        return wildcardSuccess;
    }
    if (name.isEmpty() || password.isEmpty()) {
        LOG_WARNING("PasswordManager", "Cannot update password with empty name or value");
        return false;
    }
    passwords[index].name = name;
    passwords[index].password = password;
    passwords[index].category = category;
    passwords[index].setAutoSend(auto_send);
    passwords[index].setSendLogin(send_login);
    passwords[index].login = login;
    passwords[index].nav_mode = nav_mode;
    passwords[index].login_delay_ms = login_delay_ms;
    // Wildcard status can only be set at creation time (addPassword).
    // updatePassword() must never convert a normal entry into a
    // wildcard entry, regardless of what the caller passes in.
    // (passwords[index].getWildcard() is guaranteed false here — the
    // wildcard-entry branch above already returned early otherwise.)
    passwords[index].wildcard_len = wildcard_len;
    // Recompute strength and hash on update
    passwords[index].strength = computeStrength(password);
    passwords[index].pw_hash  = computePwHash(password);
    // порядок остается прежний
    LOG_INFO("PasswordManager", "Updated password entry at index " + String(index));
    bool success = savePasswords();
    if (!success) {
        LOG_ERROR("PasswordManager", "Failed to save passwords after update");
    }
    return success;
}

bool PasswordManager::deletePassword(int index) {
    if (index < 0 || index >= passwords.size()) {
        LOG_WARNING("PasswordManager", "Invalid password index for deletion: " + String(index));
        return false;
    }
    String deletedName = passwords[index].name;
    passwords.erase(passwords.begin() + index);
    LOG_INFO("PasswordManager", "Deleted password entry");
    bool success = savePasswords();
    if (!success) {
        LOG_ERROR("PasswordManager", "Failed to save passwords after deletion");
    }
    return success;
}

const std::vector<PasswordEntry>& PasswordManager::getAllPasswords() const {
    return passwords;
}

bool PasswordManager::reorderPasswords(const std::vector<std::pair<String, int>>& newOrder) {
    LOG_INFO("PasswordManager", "Reordering passwords");
    
    // Создаем карту имя -> новый порядок
    std::map<String, int> orderMap;
    for (const auto& pair : newOrder) {
        orderMap[pair.first] = pair.second;
    }
    
    // Обновляем порядок для существующих паролей
    bool changed = false;
    for (auto& pwd : passwords) {
        auto it = orderMap.find(pwd.name);
        if (it != orderMap.end() && pwd.order != it->second) {
            pwd.order = it->second;
            changed = true;
        }
    }
    
    if (changed) {
        // Сортируем пароли по порядку после изменения order
        std::sort(passwords.begin(), passwords.end(), [](const PasswordEntry& a, const PasswordEntry& b) {
            return a.order < b.order;
        });
        
        bool success = savePasswords();
        if (success) {
            LOG_INFO("PasswordManager", "Successfully reordered passwords");
        } else {
            LOG_ERROR("PasswordManager", "Failed to save reordered passwords");
        }
        return success;
    }
    
    return true; // Никаких изменений не было
}

std::vector<PasswordEntry> PasswordManager::getAllPasswordsForExport() {
    // Принудительно перезагружаем и расшифровываем пароли из файла
    std::vector<PasswordEntry> exportPasswords;
    
    // Use space-aware path
    String passwordPath = CryptoManager::getInstance().getSpacePath("passwords");
    
    if (!LittleFS.exists(passwordPath)) {
        LOG_INFO("PasswordManager", "Password file does not exist for export");
        return exportPasswords; // Пустой вектор если файл не существует
    }

    File file = LittleFS.open(passwordPath, "r");
    if (!file) {
        LOG_ERROR("PasswordManager", "Failed to open password file for export");
        return exportPasswords;
    }

    String encryptedData = file.readString();
    file.close();

    if (encryptedData.isEmpty()) {
        return exportPasswords; // Пустой вектор если файл пустой
    }

    // Принудительно расшифровываем данные
    String jsonData = CryptoManager::getInstance().decrypt(encryptedData);
    if (jsonData.isEmpty()) {
        LOG_ERROR("PasswordManager", "Failed to decrypt passwords for export");
        return exportPasswords;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonData);
    if (error) {
        LOG_ERROR("PasswordManager", "deserializeJson() failed for export: " + String(error.c_str()));
        return exportPasswords;
    }

    JsonArray array = doc.as<JsonArray>();
    for (JsonObject obj : array) {
        PasswordEntry entry;
        entry.name = obj["name"].as<String>();
        entry.password = obj["password"].as<String>();
        entry.order = obj["order"] | 0;
        entry.strength = obj["strength"] | 0;
        entry.pw_hash = obj["pw_hash"] | "";
        entry.category  = obj["category"]  | "";
        entry.setAutoSend(obj["auto_send"] | false);
        entry.setSendLogin(obj["send_login"] | false);
        entry.login = obj["login"] | "";
        entry.nav_mode = obj["nav_mode"] | "enter";
        entry.login_delay_ms = obj["login_delay_ms"] | 300;
        entry.setWildcard(obj["wildcard"] | false);
        entry.wildcard_len = obj["wildcard_len"] | 16;
        exportPasswords.push_back(entry);
    }

    // Сортируем пароли по порядку перед экспортом
    std::sort(exportPasswords.begin(), exportPasswords.end(), [](const PasswordEntry& a, const PasswordEntry& b) {
        return a.order < b.order;
    });

    return exportPasswords;
}

// --- Новая функция для импорта паролей ---
bool PasswordManager::replaceAllPasswords(const String& jsonContent) {
    LOG_INFO("PasswordManager", "Importing passwords from JSON");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonContent);
    if (error) {
        LOG_ERROR("PasswordManager", "Password import failed, invalid JSON: " + String(error.c_str()));
        return false;
    }

    passwords.clear();
    JsonArray array = doc.as<JsonArray>();
    int currentOrder = 0;
    bool wildcardSeen = false;
    int wildcardDowngraded = 0;
    for (JsonObject obj : array) {
        PasswordEntry entry;
        entry.name = obj["name"].as<String>();
        entry.password = obj["password"].as<String>();
        entry.order = obj["order"] | currentOrder++;
        entry.strength = obj["strength"] | (uint8_t)0;
        entry.pw_hash  = obj["pw_hash"] | String("");
        entry.category  = obj["category"]  | "";
        entry.setAutoSend(obj["auto_send"] | false);
        entry.setSendLogin(obj["send_login"] | false);
        entry.login = obj["login"] | "";
        entry.nav_mode = obj["nav_mode"] | "enter";
        entry.login_delay_ms = obj["login_delay_ms"] | 300;
        bool wantsWildcard = obj["wildcard"] | false;
        if (wantsWildcard && wildcardSeen) {
            // Singleton constraint: an imported backup may contain
            // more than one wildcard-flagged entry (crafted or
            // corrupted file). Only the first is honored; subsequent
            // ones are downgraded to normal entries rather than
            // dropped, so no data is silently lost.
            wantsWildcard = false;
            wildcardDowngraded++;
        } else if (wantsWildcard) {
            wildcardSeen = true;
        }
        entry.setWildcard(wantsWildcard);
        entry.wildcard_len = obj["wildcard_len"] | 16;
        // Recompute missing fields (handles legacy import files)
        if (entry.strength == 0 || entry.pw_hash.isEmpty()) {
            entry.strength = computeStrength(entry.password);
            entry.pw_hash  = computePwHash(entry.password);
        }
        passwords.push_back(entry);
    }
    if (wildcardDowngraded > 0) {
        LOG_WARNING("PasswordManager", "Import contained " + String(wildcardDowngraded) + " extra wildcard-flagged entr(y/ies); downgraded to normal entries (singleton constraint)");
    }

    // Сортируем пароли по порядку после импорта
    std::sort(passwords.begin(), passwords.end(), [](const PasswordEntry& a, const PasswordEntry& b) {
        return a.order < b.order;
    });

    // Сохраняем новый набор паролей, который будет автоматически зашифрован
    bool success = savePasswords();
    if (success) {
        LOG_INFO("PasswordManager", "Successfully imported " + String(passwords.size()) + " passwords");
    } else {
        LOG_ERROR("PasswordManager", "Failed to save imported passwords");
    }
    return success;
}

bool PasswordManager::loadPasswords() {
    LOG_DEBUG("PasswordManager", "Loading passwords from file");
    
    // Use space-aware path
    String passwordPath = CryptoManager::getInstance().getSpacePath("passwords");
    
    if (!LittleFS.exists(passwordPath)) {
        LOG_INFO("PasswordManager", "Password file doesn't exist yet, starting with empty list");
        return true; // File doesn't exist yet, which is fine.
    }

    File file = LittleFS.open(passwordPath, "r");
    if (!file) {
        LOG_ERROR("PasswordManager", "Failed to open password file for reading");
        return false;
    }

    String encryptedData = file.readString();
    file.close();

    if (encryptedData.isEmpty()) {
        return true; // File is empty, nothing to load.
    }

    // Use the static decrypt method from CryptoManager
    String jsonData = CryptoManager::getInstance().decrypt(encryptedData);
    if (jsonData.isEmpty()) {
        LOG_WARNING("PasswordManager", "Failed to decrypt passwords or file is empty");
        // If decryption fails, it might be an old unencrypted file.
        // For safety, we'll just treat it as empty and overwrite on save.
        passwords.clear();
        return true; 
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonData);
    if (error) {
        LOG_ERROR("PasswordManager", "deserializeJson() failed for passwords: " + String(error.c_str()));
        return false;
    }

    passwords.clear();
    JsonArray array = doc.as<JsonArray>();
    int currentOrder = 0;
    int entryIndex = 0;
    int skippedCount = 0;
    for (JsonObject obj : array) {
        if (!obj["name"].is<String>() || !obj["password"].is<String>() ||
            obj["name"].as<String>().isEmpty() || obj["password"].as<String>().isEmpty()) {
            LOG_ERROR("PasswordManager", "Skipping corrupted password entry at index " + String(entryIndex) + " (missing/empty name or password)");
            skippedCount++;
            entryIndex++;
            continue;
        }
        PasswordEntry entry;
        entry.name = obj["name"].as<String>(); 
        entry.password = obj["password"].as<String>();
        entry.order = obj["order"] | currentOrder++;
        entry.strength = obj["strength"] | 0;
        entry.pw_hash = obj["pw_hash"] | "";
        entry.category  = obj["category"]  | "";
        entry.setAutoSend(obj["auto_send"] | false);
        entry.setSendLogin(obj["send_login"] | false);
        entry.login = obj["login"] | "";
        entry.nav_mode = obj["nav_mode"] | "enter";
        entry.login_delay_ms = obj["login_delay_ms"] | 300;
        entry.setWildcard(obj["wildcard"] | false);
        entry.wildcard_len = obj["wildcard_len"] | 16;
        // Migration: compute missing fields for legacy entries
        if (entry.strength == 0 || entry.pw_hash.isEmpty()) {
            entry.strength = computeStrength(entry.password);
            entry.pw_hash  = computePwHash(entry.password);
            _needsSave = true;
        }
        passwords.push_back(entry);
        entryIndex++;
    }
    if (skippedCount > 0) {
        LOG_ERROR("PasswordManager", "Password list loaded with " + String(skippedCount) + " corrupted entr" + (skippedCount == 1 ? "y" : "ies") + " skipped");
        _needsSave = true;
    }

    // Сортируем пароли по порядку после загрузки
    std::sort(passwords.begin(), passwords.end(), [](const PasswordEntry& a, const PasswordEntry& b) {
        return a.order < b.order;
    });

    LOG_INFO("PasswordManager", "Loaded " + String(passwords.size()) + " passwords successfully");
    return true;
}

bool PasswordManager::savePasswords() {
    LOG_DEBUG("PasswordManager", "Saving passwords to file");
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();

    for (const auto& entry : passwords) {
        JsonObject obj = array.add<JsonObject>();
        obj["name"]     = entry.name;
        obj["password"] = entry.password;
        obj["order"]    = entry.order;
        obj["strength"] = entry.strength;
        obj["pw_hash"]  = entry.pw_hash;
        obj["category"]  = entry.category;
        obj["auto_send"] = entry.getAutoSend();
        obj["send_login"] = entry.getSendLogin();
        obj["login"] = entry.login;
        obj["nav_mode"] = entry.nav_mode;
        obj["login_delay_ms"] = entry.login_delay_ms;
        obj["wildcard"] = entry.getWildcard();
        obj["wildcard_len"] = entry.wildcard_len;
    }

    String jsonData;
    size_t jsonSize = serializeJson(doc, jsonData);
    if (jsonSize == 0) {
        LOG_ERROR("PasswordManager", "Failed to serialize passwords to JSON");
        return false;
    }

    // Use the static encrypt method from CryptoManager
    String encryptedData = CryptoManager::getInstance().encrypt(jsonData);
    if (encryptedData.isEmpty()) {
        LOG_ERROR("PasswordManager", "Failed to encrypt passwords");
        return false;
    }

    // Use space-aware path
    String passwordPath = CryptoManager::getInstance().getSpacePath("passwords");
    
    File file = LittleFS.open(passwordPath, "w");
    if (!file) {
        LOG_ERROR("PasswordManager", "Failed to open password file for writing");
        return false;
    }

    size_t bytesWritten = file.print(encryptedData);
    file.close();
    _revision++;
    
    if (bytesWritten > 0) {
        LOG_INFO("PasswordManager", "Saved " + String(passwords.size()) + " passwords successfully");
        return true;
    } else {
        LOG_ERROR("PasswordManager", "Failed to write encrypted password data");
        return false;
    }
}

void PasswordManager::wipePasswords() {
    for (auto& entry : passwords) {
        volatile char* p = const_cast<volatile char*>(entry.password.c_str());
        for (size_t i = 0; i < entry.password.length(); i++) p[i] = 0;
        entry.password = "";
        
        volatile char* pLogin = const_cast<volatile char*>(entry.login.c_str());
        for (size_t i = 0; i < entry.login.length(); i++) pLogin[i] = 0;
        entry.login = "";
    }
    passwords.clear();
}
