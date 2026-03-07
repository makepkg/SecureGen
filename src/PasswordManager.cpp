#include "PasswordManager.h"
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
    } else {
        LOG_ERROR("PasswordManager", "Failed to load passwords during initialization");
    }
}

bool PasswordManager::addPassword(const String& name, const String& password) {
    if (name.isEmpty() || password.isEmpty()) {
        LOG_WARNING("PasswordManager", "Cannot add password with empty name or value");
        return false;
    }
    // Найти максимальный порядок для нового пароля
    int maxOrder = 0;
    for (const auto& pwd : passwords) {
        if (pwd.order > maxOrder) maxOrder = pwd.order;
    }
    PasswordEntry newPassword;
    newPassword.name = name;
    newPassword.password = password;
    newPassword.order = maxOrder + 1;
    passwords.push_back(newPassword);
    LOG_INFO("PasswordManager", "Added password entry: [HIDDEN]");
    bool success = savePasswords();
    if (!success) {
        LOG_ERROR("PasswordManager", "Failed to save passwords after adding entry");
    }
    return success;
}

bool PasswordManager::updatePassword(int index, const String& name, const String& password) {
    if (index < 0 || index >= passwords.size()) {
        LOG_WARNING("PasswordManager", "Invalid password index for update: " + String(index));
        return false;
    }
    if (name.isEmpty() || password.isEmpty()) {
        LOG_WARNING("PasswordManager", "Cannot update password with empty name or value");
        return false;
    }
    passwords[index].name = name;
    passwords[index].password = password;
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

std::vector<PasswordEntry> PasswordManager::getAllPasswords() {
    // Сортируем пароли по порядку перед возвратом
    std::sort(passwords.begin(), passwords.end(), [](const PasswordEntry& a, const PasswordEntry& b) {
        return a.order < b.order;
    });
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
    
    if (!LittleFS.exists(PASSWORD_FILE)) {
        LOG_INFO("PasswordManager", "Password file does not exist for export");
        return exportPasswords; // Пустой вектор если файл не существует
    }

    File file = LittleFS.open(PASSWORD_FILE, "r");
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
        exportPasswords.push_back(entry);
    }

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
    for (JsonObject obj : array) {
        PasswordEntry entry;
        entry.name = obj["name"].as<String>();
        entry.password = obj["password"].as<String>();
        entry.order = obj["order"] | currentOrder++;  // Используем существующий order или назначаем по порядку
        passwords.push_back(entry);
    }

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
    if (!LittleFS.exists(PASSWORD_FILE)) {
        LOG_INFO("PasswordManager", "Password file doesn't exist yet, starting with empty list");
        return true; // File doesn't exist yet, which is fine.
    }

    File file = LittleFS.open(PASSWORD_FILE, "r");
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
    for (JsonObject obj : array) {
        PasswordEntry entry;
        entry.name = obj["name"].as<String>(); 
        entry.password = obj["password"].as<String>();
        entry.order = obj["order"] | currentOrder++;  // Используем существующий order или назначаем по порядку
        passwords.push_back(entry);
    }

    LOG_INFO("PasswordManager", "Loaded " + String(passwords.size()) + " passwords successfully");
    return true;
}

bool PasswordManager::savePasswords() {
    LOG_DEBUG("PasswordManager", "Saving passwords to file");
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();

    for (const auto& entry : passwords) {
        JsonObject obj = array.add<JsonObject>();
        obj["name"] = entry.name;
        obj["password"] = entry.password;
        obj["order"] = entry.order;
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

    File file = LittleFS.open(PASSWORD_FILE, "w");
    if (!file) {
        LOG_ERROR("PasswordManager", "Failed to open password file for writing");
        return false;
    }

    size_t bytesWritten = file.print(encryptedData);
    file.close();
    
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
    }
    passwords.clear();
}
