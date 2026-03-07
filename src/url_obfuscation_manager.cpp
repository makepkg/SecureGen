#include "url_obfuscation_manager.h"
#include "log_manager.h"
#include "crypto_manager.h"
#include <map>
#include <vector>
#include <mbedtls/sha256.h>
#include <esp_random.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <time.h>

URLObfuscationManager& URLObfuscationManager::getInstance() {
    static URLObfuscationManager instance;
    return instance;
}

URLObfuscationManager::URLObfuscationManager() 
    : initialized(false), lastRotationTime(0) {
}

URLObfuscationManager::~URLObfuscationManager() {
    // Clean up resources if needed
}

bool URLObfuscationManager::begin() {
    if (initialized) return true;
    
    LOG_INFO("URLObfuscation", "Initializing URL obfuscation manager...");
    
    // 🎯 Увеличиваем счетчик перезагрузок
    incrementBootCounter();
    
    // Регистрируем критические эндпоинты для тестовой страницы
    registerCriticalEndpoint("/api/secure/keyexchange", "ECDH Key Exchange");
    registerCriticalEndpoint("/login", "Login Page");
    registerCriticalEndpoint("/api/tunnel", "Method Tunneling");
    registerCriticalEndpoint("/api/keys", "TOTP Keys & Codes");
    registerCriticalEndpoint("/api/add", "TOTP Key Addition");
    registerCriticalEndpoint("/api/remove", "TOTP Key Removal");
    registerCriticalEndpoint("/api/show_qr", "QR Code Display");
    registerCriticalEndpoint("/api/hotp/generate", "HOTP Code Generation");
    registerCriticalEndpoint("/api/keys/reorder", "TOTP Keys Reordering");
    registerCriticalEndpoint("/api/export", "TOTP Keys Export");
    registerCriticalEndpoint("/api/import", "TOTP Keys Import");
    registerCriticalEndpoint("/api/passwords", "All Passwords List");
    registerCriticalEndpoint("/api/passwords/add", "Password Addition");
    registerCriticalEndpoint("/api/passwords/delete", "Password Deletion");
    registerCriticalEndpoint("/api/passwords/update", "Password Update");
    registerCriticalEndpoint("/api/passwords/get", "Password Retrieval");
    registerCriticalEndpoint("/api/passwords/reorder", "Password Reordering");
    registerCriticalEndpoint("/api/passwords/export", "Password Export");
    registerCriticalEndpoint("/api/passwords/import", "Password Import");
    registerCriticalEndpoint("/api/config", "Server Configuration");
    registerCriticalEndpoint("/api/pincode_settings", "PIN Security Settings");
    registerCriticalEndpoint("/api/change_password", "Web Cabinet Password Change");
    registerCriticalEndpoint("/api/change_ap_password", "WiFi AP Password Change");
    registerCriticalEndpoint("/api/session_duration", "Session Duration Settings");
    registerCriticalEndpoint("/api/activity", "Activity Timer Reset");
    registerCriticalEndpoint("/api/enable_import_export", "API Access Control");
    registerCriticalEndpoint("/api/import_export_status", "API Access Status");
    registerCriticalEndpoint("/api/theme", "Display Theme");
    registerCriticalEndpoint("/api/display_settings", "Display Settings");
    registerCriticalEndpoint("/api/splash/mode", "Splash Screen Selection");
    registerCriticalEndpoint("/api/clock_settings", "Clock Display Settings");
    registerCriticalEndpoint("/api/ble_pin_update", "BLE PIN Update");
    registerCriticalEndpoint("/api/clear_ble_clients", "Clear BLE Clients");
    registerCriticalEndpoint("/api/ble_settings", "BLE Device Settings");
    registerCriticalEndpoint("/api/mdns_settings", "mDNS Settings");
    registerCriticalEndpoint("/api/startup_mode", "Startup Mode");
    registerCriticalEndpoint("/api/settings", "Device Settings");
    registerCriticalEndpoint("/api/reboot", "System Reboot");
    registerCriticalEndpoint("/api/reboot_with_web", "Reboot With Web Server");
    registerCriticalEndpoint("/logout", "Admin Logout");
    // /api/upload_splash removed - custom splash upload disabled for security
    
    // Генерируем initial mapping
    generateDailyMapping();
    
    initialized = true;
    LOG_INFO("URLObfuscation", "URL obfuscation initialized with " + String(realToObfuscated.size()) + " mappings");
    
    return true;
}

void URLObfuscationManager::update() {
    // 🚫 Ротация происходит ТОЛЬКО при перезагрузке ESP32
    // Этот метод больше не нужен, но оставлен для обратной совместимости
    // Вы можете удалить вызов URLObfuscationManager::getInstance().update() из main.cpp
}

String URLObfuscationManager::obfuscateURL(const String& realPath) {
    if (!initialized) return realPath;
    
    auto it = realToObfuscated.find(realPath);
    if (it != realToObfuscated.end()) {
        return it->second;
    }
    
    // 🔗 Автоматическая генерация маппинга для нового эндпоинта
    LOG_WARNING("URLObfuscation", "No mapping found for: " + realPath + ", generating on-the-fly...");
    
    // Используем текущий seed
    uint32_t currentSeed = getCurrentSeed();
    String obfuscatedPath = generateObfuscatedPath(realPath, currentSeed);
    
    // Сохраняем в обе стороны
    realToObfuscated[realPath] = obfuscatedPath;
    obfuscatedToReal[obfuscatedPath] = realPath;
    
    // Сохраняем на диск с тем же seed
    saveMappingsToStorage(currentSeed);
    
    LOG_INFO("URLObfuscation", "✅ Generated mapping: " + realPath + " -> " + obfuscatedPath);
    
    return obfuscatedPath;
}

String URLObfuscationManager::deobfuscateURL(const String& obfuscatedPath) {
    if (!initialized) return obfuscatedPath;
    
    auto it = obfuscatedToReal.find(obfuscatedPath);
    if (it != obfuscatedToReal.end()) {
        return it->second;
    }
    
    return ""; // No mapping found
}

bool URLObfuscationManager::isObfuscatedPath(const String& path) {
    if (!initialized) return false;
    return obfuscatedToReal.find(path) != obfuscatedToReal.end();
}

String URLObfuscationManager::getRealPath(const String& obfuscatedPath) {
    return deobfuscateURL(obfuscatedPath);
}

void URLObfuscationManager::generateDailyMapping() {
    LOG_INFO("URLObfuscation", "Generating URL mapping...");
    
    uint32_t currentSeed = getCurrentSeed();
    
    // Проверяем есть ли сохраненные mappings для этого дня
    if (loadMappingsFromStorage(currentSeed)) {
        LOG_INFO("URLObfuscation", "Loaded existing mappings from storage");
        return;
    }
    
    // Очищаем существующие mappings
    realToObfuscated.clear();
    obfuscatedToReal.clear();
    
    // Генерируем mappings для всех критических эндпоинтов
    for (const String& endpoint : criticalEndpoints) {
        String obfuscatedPath = generateObfuscatedPath(endpoint, currentSeed);
        
        realToObfuscated[endpoint] = obfuscatedPath;
        obfuscatedToReal[obfuscatedPath] = endpoint;
        
        LOG_INFO("URLObfuscation", "Mapped: " + endpoint + " -> " + obfuscatedPath);
    }
    
    lastRotationTime = millis();
    
    // Сохраняем новые mappings в flash
    saveMappingsToStorage(currentSeed);
    
    LOG_INFO("URLObfuscation", "Daily mapping complete: " + String(realToObfuscated.size()) + " endpoints mapped");
}

bool URLObfuscationManager::needsRotation() {
    // 🚫 Ротация происходит автоматически при перезагрузке
    // Этот метод оставлен для API совместимости
    return false;
}

bool URLObfuscationManager::registerCriticalEndpoint(const String& realPath, const String& description) {
    if (realPath.isEmpty()) return false;
    
    // Проверяем, не зарегистрирован ли уже
    for (const String& existing : criticalEndpoints) {
        if (existing.equals(realPath)) {
            return true;
        }
    }
    
    // Проверяем лимиты памяти
    if (criticalEndpoints.size() >= MAX_MAPPINGS) {
        LOG_ERROR("URLObfuscation", "Cannot register more endpoints - limit reached: " + String(MAX_MAPPINGS));
        return false;
    }
    
    criticalEndpoints.push_back(realPath);
    
    return true;
}

std::vector<String> URLObfuscationManager::getAllCriticalEndpoints() {
    return criticalEndpoints;
}

String URLObfuscationManager::getObfuscatedMappingJSON() {
    if (!initialized) return "{\"mappings\":{}}";
    
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    JsonObject mappings = root["mappings"].to<JsonObject>();
    
    for (const auto& pair : realToObfuscated) {
        mappings[pair.first] = pair.second;
    }
    
    // Добавляем метаинформацию
    root["boot_counter"] = bootCounter;
    uint32_t currentEpoch = bootCounter / ROTATION_THRESHOLD;
    uint32_t bootInEpoch = ((bootCounter - 1) % ROTATION_THRESHOLD) + 1;
    uint32_t bootsUntilRotation = ROTATION_THRESHOLD - bootInEpoch;
    root["current_epoch"] = currentEpoch;
    root["boot_in_epoch"] = String(bootInEpoch) + "/" + String(ROTATION_THRESHOLD);
    root["boots_until_rotation"] = bootsUntilRotation;
    root["mappings_count"] = realToObfuscated.size();
    root["rotation_threshold"] = ROTATION_THRESHOLD;
    
    String result;
    serializeJson(doc, result);
    return result;
}

String URLObfuscationManager::generateObfuscatedPath(const String& realPath, uint32_t seed) {
    // Генерируем уникальный hash на основе пути и seed
    String hashInput = realPath + String(seed) + "ESP32_URL_OBFUSCATION";
    String hash = generateSecureHash(hashInput, seed);
    
    // Создаем обфусцированный путь фиксированной длины
    String obfuscatedPath = "/";
    
    // Используем первые символы hash для создания пути
    for (size_t i = 0; i < OBFUSCATED_PATH_LENGTH && i < hash.length(); i++) {
        char c = hash.charAt(i);
        // Используем только alphanumeric символы
        if (isalnum(c)) {
            obfuscatedPath += c;
        } else {
            // Заменяем не-alphanumeric на цифры
            obfuscatedPath += String((uint8_t)c % 10);
        }
    }
    
    // Дополняем до нужной длины если необходимо
    while (obfuscatedPath.length() < OBFUSCATED_PATH_LENGTH + 1) {
        obfuscatedPath += String(esp_random() % 10);
    }
    
    return obfuscatedPath;
}

String URLObfuscationManager::generateSecureHash(const String& input, uint32_t seed) {
    // Используем mbedTLS SHA256 API напрямую
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    
    // Начинаем SHA256 операцию (0 = SHA256, не SHA224)
    mbedtls_sha256_starts(&ctx, 0);
    
    // Добавляем seed в hash для уникальности
    mbedtls_sha256_update(&ctx, (const unsigned char*)&seed, sizeof(seed));
    
    // Добавляем input string
    mbedtls_sha256_update(&ctx, (const unsigned char*)input.c_str(), input.length());
    
    unsigned char hash[32];
    mbedtls_sha256_finish(&ctx, hash);
    mbedtls_sha256_free(&ctx);
    
    // Конвертируем в hex string
    String result = "";
    for (int i = 0; i < 32; i++) {
        if (hash[i] < 16) result += "0";
        result += String(hash[i], HEX);
    }
    
    return result;
}

uint32_t URLObfuscationManager::getCurrentSeed() {
    // 🎯 Seed на основе номера эпохи (каждые 100 перезагрузок)
    uint32_t epoch = bootCounter / ROTATION_THRESHOLD;
    
    LOG_DEBUG("URLObfuscation", "🔢 Current epoch: " + String(epoch) + 
             " (boot " + String(bootCounter) + "/" + String(ROTATION_THRESHOLD) + ")");
    
    return epoch;
}

bool URLObfuscationManager::loadMappingsFromStorage(uint32_t seed) {
    String filename = "/url_mappings_" + String(seed) + ".json";
    
    if (!LittleFS.exists(filename)) {
        return false;
    }
    
    File file = LittleFS.open(filename, "r");
    if (!file) {
        LOG_ERROR("URLObfuscation", "Failed to open mappings file: " + filename);
        return false;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        LOG_ERROR("URLObfuscation", "Failed to parse mappings JSON: " + String(error.c_str()));
        return false;
    }
    
    // Очищаем текущие mappings
    realToObfuscated.clear();
    obfuscatedToReal.clear();
    
    // Загружаем mappings из JSON
    JsonObject mappings = doc["mappings"];
    for (JsonPair pair : mappings) {
        String real = pair.key().c_str();
        String obfuscated = pair.value().as<String>();
        
        realToObfuscated[real] = obfuscated;
        obfuscatedToReal[obfuscated] = real;
    }
    
    lastRotationTime = doc["rotation_time"] | millis();
    
    LOG_INFO("URLObfuscation", "Loaded " + String(realToObfuscated.size()) + " mappings from storage");
    return true;
}

bool URLObfuscationManager::saveMappingsToStorage(uint32_t seed) {
    String filename = "/url_mappings_" + String(seed) + ".json";
    
    File file = LittleFS.open(filename, "w");
    if (!file) {
        LOG_ERROR("URLObfuscation", "Failed to create mappings file: " + filename);
        return false;
    }
    
    JsonDocument doc;
    JsonObject mappings = doc["mappings"].to<JsonObject>();
    
    // Сохраняем все mappings
    for (const auto& pair : realToObfuscated) {
        mappings[pair.first] = pair.second;
    }
    
    // Добавляем метаданные
    doc["seed"] = seed;
    doc["rotation_time"] = lastRotationTime;
    doc["created_at"] = millis();
    doc["version"] = "1.0";
    
    if (serializeJson(doc, file) == 0) {
        LOG_ERROR("URLObfuscation", "Failed to write mappings JSON");
        file.close();
        return false;
    }
    
    file.close();
    LOG_INFO("URLObfuscation", "Saved " + String(realToObfuscated.size()) + " mappings to storage: " + filename);
    
    // Удаляем старые файлы mappings чтобы не засорять flash
    cleanupOldMappings(seed);
    
    return true;
}

void URLObfuscationManager::cleanupOldMappings(uint32_t currentSeed) {
    // Удаляем mapping файлы старше 2 дней
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    
    while (file) {
        String filename = file.name();
        if (filename.startsWith("url_mappings_") && filename.endsWith(".json")) {
            // Извлекаем seed из имени файла
            int seedStart = filename.indexOf('_') + 1;
            int seedEnd = filename.lastIndexOf('.');
            if (seedStart > 0 && seedEnd > seedStart) {
                uint32_t fileSeed = filename.substring(seedStart, seedEnd).toInt();
                
                // Удаляем файлы со старыми seeds (разница больше 2)
                if (abs((int32_t)(currentSeed - fileSeed)) > 2) {
                    String fullPath = "/" + filename;
                    LittleFS.remove(fullPath);
                }
            }
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();
}

// ========================================
// Boot Counter Management (New!)
// ========================================

uint32_t URLObfuscationManager::loadBootCounter() {
    if (!LittleFS.exists("/boot_counter.txt")) {
        LOG_INFO("URLObfuscation", "📝 Boot counter file not found - starting from 0");
        return 0;
    }
    
    File file = LittleFS.open("/boot_counter.txt", "r");
    if (!file) {
        LOG_ERROR("URLObfuscation", "❌ Failed to open boot counter file");
        return 0;
    }
    
    uint32_t counter = file.parseInt();
    file.close();
    
    LOG_DEBUG("URLObfuscation", "📂 Loaded boot counter: " + String(counter));
    return counter;
}

void URLObfuscationManager::saveBootCounter(uint32_t counter) {
    File file = LittleFS.open("/boot_counter.txt", "w");
    if (!file) {
        LOG_ERROR("URLObfuscation", "❌ Failed to save boot counter");
        return;
    }
    
    file.print(counter);
    file.close();
    
    LOG_DEBUG("URLObfuscation", "💾 Saved boot counter: " + String(counter));
}

void URLObfuscationManager::incrementBootCounter() {
    // Загружаем текущий счетчик
    bootCounter = loadBootCounter();
    
    // Увеличиваем
    bootCounter++;
    
    // Вычисляем номер внутри текущей эпохи (1-100)
    uint32_t bootInEpoch = ((bootCounter - 1) % ROTATION_THRESHOLD) + 1;
    uint32_t currentEpoch = bootCounter / ROTATION_THRESHOLD;
    
    // 🎯 Красивый лог с номером перезагрузки
    LOG_INFO("URLObfuscation", "🔄 Boot #" + String(bootCounter) + 
             " [" + String(bootInEpoch) + "/" + String(ROTATION_THRESHOLD) + 
             "] Epoch: " + String(currentEpoch));
    
    // Проверяем ротацию
    if (bootInEpoch == 1 && bootCounter > 1) {
        LOG_INFO("URLObfuscation", "🔄 ROTATION! New epoch: " + String(currentEpoch));
    }
    
    // Сохраняем обратно
    saveBootCounter(bootCounter);
}
