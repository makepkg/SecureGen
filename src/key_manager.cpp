#include "key_manager.h"
#include "config.h"
#include "crypto_manager.h"
#include "log_manager.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <algorithm>
#include <map>

KeyManager::KeyManager() {}

bool KeyManager::begin() {
    LOG_INFO("KeyManager", "Initializing...");
    bool success = loadKeys();
    if (success) {
        LOG_INFO("KeyManager", "Initialized successfully");
    } else {
        LOG_ERROR("KeyManager", "Failed to initialize");
    }
    return success;
}

bool KeyManager::addKey(const String& name, const String& secret) {
    if (name.isEmpty() || secret.isEmpty()) {
        LOG_WARNING("KeyManager", "Cannot add key with empty name or secret");
        return false;
    }
    for (const auto& key : keys) {
        if (key.name == name) {
            LOG_WARNING("KeyManager", "Key already exists (duplicate name)");
            return false;
        }
    }
    // Найти максимальный порядок для нового ключа
    int maxOrder = 0;
    for (const auto& key : keys) {
        if (key.order > maxOrder) maxOrder = key.order;
    }
    TOTPKey newKey;
    newKey.name = name;
    newKey.secret = secret; 
    newKey.order = maxOrder + 1;
    // Используем значения по умолчанию для остальных полей
    keys.push_back(newKey);
    LOG_INFO("KeyManager", "Added TOTP key at index " + String(keys.size() - 1));
    bool success = saveKeys();
    if (!success) {
        LOG_ERROR("KeyManager", "Failed to save keys after adding: " + name);
    }
    return success;
}

bool KeyManager::addKeyExtended(const String& name, const String& secret,
                               TOTPType type, TOTPAlgorithm algo,
                               uint8_t digits, uint16_t period) {
    if (name.isEmpty() || secret.isEmpty()) {
        LOG_WARNING("KeyManager", "Cannot add key with empty name or secret");
        return false;
    }
    
    // Проверка на дубликаты
    for (const auto& key : keys) {
        if (key.name == name) {
            LOG_WARNING("KeyManager", "Key already exists (duplicate name)");
            return false;
        }
    }
    
    // Найти максимальный порядок
    int maxOrder = 0;
    for (const auto& key : keys) {
        if (key.order > maxOrder) maxOrder = key.order;
    }
    
    TOTPKey newKey;
    newKey.name = name;
    newKey.secret = secret;
    newKey.order = maxOrder + 1;
    newKey.type = type;
    newKey.algorithm = algo;
    newKey.digits = digits;
    newKey.period = period;
    newKey.counter = 0; // Начальное значение
    
    keys.push_back(newKey);
    
    LOG_INFO("KeyManager", "Added extended TOTP key at index " + String(keys.size() - 1));
    LOG_DEBUG("KeyManager", "Key params: [Type=" + String(type == TOTPType::HOTP ? "HOTP" : "TOTP") +
             ", Algo=" + String(algo == TOTPAlgorithm::SHA1 ? "SHA1" : 
                               algo == TOTPAlgorithm::SHA256 ? "SHA256" : "SHA512") +
             ", Digits=" + String(digits) + ", Period=" + String(period) + "]");
    
    bool success = saveKeys();
    if (!success) {
        LOG_ERROR("KeyManager", "Failed to save keys after adding: " + name);
    }
    return success;
}

bool KeyManager::updateKey(int index, const String& name, const String& secret) {
    if (index < 0 || index >= keys.size()) {
        LOG_WARNING("KeyManager", "Invalid key index for update: " + String(index));
        return false;
    }
    if (name.isEmpty() || secret.isEmpty()) {
        LOG_WARNING("KeyManager", "Cannot update key with empty name or secret");
        return false;
    }
    keys[index].name = name;
    keys[index].secret = secret;
    // порядок остается прежний
    LOG_INFO("KeyManager", "Updated TOTP key at index " + String(index));
    bool success = saveKeys();
    if (!success) {
        LOG_ERROR("KeyManager", "Failed to save keys after update");
    }
    return success;
}

bool KeyManager::updateKeyExtended(int index, const String& name, const String& secret,
                                  TOTPType type, TOTPAlgorithm algo,
                                  uint8_t digits, uint16_t period) {
    if (index < 0 || index >= keys.size()) {
        LOG_WARNING("KeyManager", "Invalid key index for update: " + String(index));
        return false;
    }
    if (name.isEmpty() || secret.isEmpty()) {
        LOG_WARNING("KeyManager", "Cannot update key with empty name or secret");
        return false;
    }
    
    keys[index].name = name;
    keys[index].secret = secret;
    keys[index].type = type;
    keys[index].algorithm = algo;
    keys[index].digits = digits;
    keys[index].period = period;
    // order и counter остаются прежними
    
    LOG_INFO("KeyManager", "Updated extended TOTP key at index " + String(index));
    bool success = saveKeys();
    if (!success) {
        LOG_ERROR("KeyManager", "Failed to save keys after update");
    }
    return success;
}

bool KeyManager::incrementHOTPCounter(int index) {
    if (index < 0 || index >= keys.size()) {
        LOG_WARNING("KeyManager", "Invalid key index for counter increment: " + String(index));
        return false;
    }
    
    if (keys[index].type != TOTPType::HOTP) {
        LOG_WARNING("KeyManager", "Cannot increment counter for non-HOTP key at index " + String(index));
        return false;
    }
    
    keys[index].counter++;
    LOG_DEBUG("KeyManager", "Incremented HOTP counter at index " + String(index) + " to " + String(keys[index].counter));
    
    return saveKeys();
}

TOTPKey& KeyManager::getKeyRef(int index) {
    return keys[index];
}

bool KeyManager::removeKey(int index) {
    if (index < 0 || index >= keys.size()) {
        LOG_WARNING("KeyManager", "Invalid key index for removal: " + String(index));
        return false;
    }
    keys.erase(keys.begin() + index);
    LOG_INFO("KeyManager", "Removed TOTP key at index " + String(index));
    bool success = saveKeys();
    if (!success) {
        LOG_ERROR("KeyManager", "Failed to save keys after removal");
    }
    return success;
}

std::vector<TOTPKey> KeyManager::getAllKeys() {
    // Сортируем ключи по порядку перед возвратом
    std::sort(keys.begin(), keys.end(), [](const TOTPKey& a, const TOTPKey& b) {
        return a.order < b.order;
    });
    return keys;
}

bool KeyManager::reorderKeys(const std::vector<std::pair<String, int>>& newOrder) {
    LOG_INFO("KeyManager", "Reordering TOTP keys");
    
    // Создаем карту имя -> новый порядок
    std::map<String, int> orderMap;
    for (const auto& pair : newOrder) {
        orderMap[pair.first] = pair.second;
    }
    
    // Обновляем порядок для существующих ключей
    bool changed = false;
    for (auto& key : keys) {
        auto it = orderMap.find(key.name);
        if (it != orderMap.end() && key.order != it->second) {
            key.order = it->second;
            changed = true;
        }
    }
    
    if (changed) {
        bool success = saveKeys();
        if (success) {
            LOG_INFO("KeyManager", "Successfully reordered TOTP keys");
        } else {
            LOG_ERROR("KeyManager", "Failed to save reordered keys");
        }
        return success;
    }
    
    return true; // Никаких изменений не было
}

// --- Новая функция для импорта ---
bool KeyManager::replaceAllKeys(const String& jsonContent) {
    LOG_INFO("KeyManager", "Importing TOTP keys from JSON");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonContent);
    if (error) {
        LOG_ERROR("KeyManager", "Import failed, invalid JSON: " + String(error.c_str()));
        return false;
    }

    keys.clear();
    JsonArray array = doc.as<JsonArray>();
    int currentOrder = 0;
    for (JsonObject obj : array) {
        TOTPKey key;
        key.name = obj["name"].as<String>();
        key.secret = obj["secret"].as<String>();
        key.order = obj["order"] | currentOrder++;  // Используем существующий order или назначаем по порядку
        
        // Читаем расширенные метаданные (с обратной совместимостью)
        String typeStr = obj["type"] | "T";
        key.type = (typeStr == "H") ? TOTPType::HOTP : TOTPType::TOTP;
        
        String algoStr = obj["algorithm"] | "1";
        if (algoStr == "256") {
            key.algorithm = TOTPAlgorithm::SHA256;
        } else if (algoStr == "512") {
            key.algorithm = TOTPAlgorithm::SHA512;
        } else {
            key.algorithm = TOTPAlgorithm::SHA1;
        }
        
        key.digits = obj["digits"] | 6;
        key.period = obj["period"] | 30;
        key.counter = obj["counter"] | 0;
        
        keys.push_back(key);
    }

    // Сохраняем новый набор ключей, который будет автоматически зашифрован
    bool success = saveKeys();
    if (success) {
        LOG_INFO("KeyManager", "Successfully imported " + String(keys.size()) + " TOTP keys");
    } else {
        LOG_ERROR("KeyManager", "Failed to save imported keys");
    }
    return success;
}

bool KeyManager::loadKeys() {
    LOG_DEBUG("KeyManager", "Loading TOTP keys from file");
    if (!LittleFS.exists(KEYS_FILE)) {
        LOG_INFO("KeyManager", "Keys file doesn't exist yet, starting with empty list");
        return true;
    }

    File file = LittleFS.open(KEYS_FILE, "r");
    if (!file) {
        LOG_ERROR("KeyManager", "Failed to open keys file for reading");
        return false;
    }

    String encrypted_base64 = file.readString();
    file.close();

    if (encrypted_base64.length() == 0) {
        keys.clear();
        LOG_INFO("KeyManager", "Keys file is empty");
        return true;
    }

    String json_string = CryptoManager::getInstance().decrypt(encrypted_base64);
    if (json_string.length() == 0) {
        LOG_ERROR("KeyManager", "Failed to decrypt keys file");
        return false;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json_string);
    if (error) {
        LOG_ERROR("KeyManager", "JSON parsing failed for keys: " + String(error.c_str()));
        return false;
    }

    keys.clear();
    JsonArray array = doc.as<JsonArray>();
    int currentOrder = 0;
    
    for (JsonObject obj : array) {
        TOTPKey key;
        
        // Обратная совместимость: поддержка старого и нового формата
        if (obj.containsKey("name")) {
            // Старый формат или новый полный формат
            key.name = obj["name"].as<String>();
            key.secret = obj["secret"].as<String>();
            key.order = obj["order"] | currentOrder++;
        } else if (obj.containsKey("n")) {
            // Новый компактный формат
            key.name = obj["n"].as<String>();
            key.secret = obj["s"].as<String>();
            key.order = obj["o"] | currentOrder++;
        } else {
            LOG_WARNING("KeyManager", "Skipping invalid key entry");
            continue;
        }
        
        // Парсинг расширенных параметров (новый формат)
        if (obj.containsKey("t")) {
            String typeStr = obj["t"] | "T";
            key.type = (typeStr == "H") ? TOTPType::HOTP : TOTPType::TOTP;
        }
        
        if (obj.containsKey("a")) {
            String algoStr = obj["a"] | "1";
            if (algoStr == "256") {
                key.algorithm = TOTPAlgorithm::SHA256;
            } else if (algoStr == "512") {
                key.algorithm = TOTPAlgorithm::SHA512;
            } else {
                key.algorithm = TOTPAlgorithm::SHA1;
            }
        }
        
        key.digits = obj["d"] | 6;
        key.period = obj["p"] | 30;
        key.counter = obj["c"] | 0;
        
        keys.push_back(key);
    }
    
    LOG_INFO("KeyManager", "Loaded " + String(keys.size()) + " TOTP keys successfully");
    return true;
}

bool KeyManager::saveKeys() {
    LOG_DEBUG("KeyManager", "Saving TOTP keys to file");
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();
    
    for (const auto& key : keys) {
        JsonObject obj = array.add<JsonObject>();
        
        // Компактный формат с однобуквенными ключами
        obj["n"] = key.name;
        obj["s"] = key.secret;
        obj["o"] = key.order;
        
        // Сохраняем только если отличается от значений по умолчанию
        if (key.type != TOTPType::TOTP) {
            obj["t"] = (key.type == TOTPType::HOTP) ? "H" : "T";
        }
        
        if (key.algorithm != TOTPAlgorithm::SHA1) {
            if (key.algorithm == TOTPAlgorithm::SHA256) {
                obj["a"] = "256";
            } else if (key.algorithm == TOTPAlgorithm::SHA512) {
                obj["a"] = "512";
            }
        }
        
        if (key.digits != 6) {
            obj["d"] = key.digits;
        }
        
        if (key.period != 30) {
            obj["p"] = key.period;
        }
        
        if (key.counter != 0) {
            obj["c"] = key.counter;
        }
    }
    
    String json_string;
    size_t jsonSize = serializeJson(doc, json_string);
    if (jsonSize == 0) {
        LOG_ERROR("KeyManager", "Failed to serialize keys to JSON");
        return false;
    }

    String encrypted_base64 = CryptoManager::getInstance().encrypt(json_string);
    if (encrypted_base64.length() == 0) {
        LOG_ERROR("KeyManager", "Failed to encrypt keys");
        return false;
    }

    File file = LittleFS.open(KEYS_FILE, "w");
    if (!file) {
        LOG_ERROR("KeyManager", "Failed to open keys file for writing");
        return false;
    }
    
    size_t bytesWritten = file.print(encrypted_base64);
    file.close();
    
    if (bytesWritten > 0) {
        LOG_INFO("KeyManager", "Saved " + String(keys.size()) + " TOTP keys successfully");
        return true;
    } else {
        LOG_ERROR("KeyManager", "Failed to write encrypted keys data");
        return false;
    }
}

void KeyManager::wipeSecrets() {
    for (auto& key : keys) {
        volatile char* p = const_cast<volatile char*>(key.secret.c_str());
        for (size_t i = 0; i < key.secret.length(); i++) p[i] = 0;
        key.secret = "";
    }
    keys.clear();
}
