#include "crypto_manager.h"
#include "secure_utils.h"
#include "config.h" // <-- ADDED for BLE PIN constants
#include "config_manager.h" // <-- ADDED for session duration
#include "log_manager.h"
#include "mbedtls/sha256.h"
#include "mbedtls/base64.h"
#include "mbedtls/aes.h"
#include "mbedtls/pkcs5.h" // For PBKDF2
#include "mbedtls/md.h" // For PBKDF2 MD context
#include <esp_system.h>
#include <esp_task_wdt.h> // <-- ADDED for watchdog reset during PBKDF2
#include <ArduinoJson.h> // <-- ADDED for new functions

// Out-of-class definitions for static constexpr (required by C++11/14)
constexpr uint8_t CryptoManager::DEVICE_KEY_MAGIC[4];
constexpr int CryptoManager::SLOT_A_OFFSET;
constexpr int CryptoManager::SLOT_B_OFFSET;
constexpr int CryptoManager::KEY_FILE_SIZE;
constexpr char CryptoManager::SPACE_FLAGS_FILE[];

// ✅ ВСПОМОГАТЕЛЬНАЯ ФУНКЦИЯ: Безопасная генерация случайных байтов (fallback)
// Использует esp_random() эффективно (по 4 байта) вместо побайтовой генерации
static void generateSecureRandomBytes_fallback(uint8_t* buffer, size_t length) {
    size_t i = 0;
    
    // Генерируем по 4 байта за раз (эффективное использование esp_random)
    while (i + 4 <= length) {
        uint32_t r = esp_random();
        buffer[i]     = (r >> 24) & 0xFF;
        buffer[i + 1] = (r >> 16) & 0xFF;
        buffer[i + 2] = (r >> 8)  & 0xFF;
        buffer[i + 3] = r & 0xFF;
        i += 4;
    }
    
    // Оставшиеся байты (если length не кратна 4)
    if (i < length) {
        uint32_t r = esp_random();
        for (size_t j = 0; i < length; i++, j++) {
            buffer[i] = (r >> (24 - j * 8)) & 0xFF;
        }
    }
}

// --- New Password-based Encryption for Import/Export ---

String CryptoManager::encryptWithPassword(const String& plaintext, const String& password) {
    LOG_DEBUG("CryptoManager", "Encrypting data with user password.");
    const int salt_len = 16;
    const int key_len = 32; // 256-bit derived key
    const int iterations = PBKDF2_ITERATIONS_EXPORT; // 15,000 iterations, defined in config.h

    LOG_INFO("CryptoManager", "Deriving encryption key with PBKDF2 (" + String(iterations) + " iterations)...");
    unsigned long start_time = millis();

    // 1. Generate random salt and IV
    uint8_t salt[salt_len];
    uint8_t iv[16];
    secureRandom(salt, salt_len);
    secureRandom(iv, 16);
    
    uint8_t iv_copy[16];
    memcpy(iv_copy, iv, 16);

    // 2. Derive encryption key from password and salt
    // ⚠️ Сбрасываем watchdog перед PBKDF2 (~3s)
    esp_task_wdt_reset();
    
    uint8_t derived_key[key_len];
    mbedtls_md_context_t sha256_ctx;
    mbedtls_md_init(&sha256_ctx);
    mbedtls_md_setup(&sha256_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    mbedtls_pkcs5_pbkdf2_hmac(&sha256_ctx, (const unsigned char*)password.c_str(), password.length(), salt, salt_len, iterations, key_len, derived_key);
    mbedtls_md_free(&sha256_ctx);
    
    esp_task_wdt_reset(); // Сбрасываем после PBKDF2
    
    unsigned long pbkdf2_elapsed = millis() - start_time;
    LOG_INFO("CryptoManager", "Key derivation completed in " + String(pbkdf2_elapsed) + "ms");

    // 3. Pad plaintext (PKCS7)
    size_t plain_len = plaintext.length();
    size_t padding_len = 16 - (plain_len % 16);
    size_t padded_len = plain_len + padding_len;
    std::vector<uint8_t> padded_input(padded_len);
    memcpy(padded_input.data(), plaintext.c_str(), plain_len);
    for(size_t i = 0; i < padding_len; i++) {
        padded_input[plain_len + i] = padding_len;
    }

    // 4. Encrypt data
    std::vector<uint8_t> ciphertext(padded_len);
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, derived_key, 256);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, padded_len, iv_copy, padded_input.data(), ciphertext.data());
    mbedtls_aes_free(&aes);

    // 5. Package salt, IV, and ciphertext into a JSON object
    JsonDocument doc;
    doc["salt"] = base64Encode(salt, salt_len);
    doc["iv"] = base64Encode(iv, 16);
    doc["ciphertext"] = base64Encode(ciphertext.data(), ciphertext.size());

    String output;
    serializeJson(doc, output);
    LOG_INFO("CryptoManager", "Data successfully encrypted for export.");
    
    secure_memzero(derived_key, sizeof(derived_key));
    std::fill(padded_input.begin(), padded_input.end(), 0);
    padded_input.clear();
    return output;
}

String CryptoManager::decryptWithPassword(const String& encryptedJson, const String& password) {
    LOG_DEBUG("CryptoManager", "Decrypting data with user password.");
    // 🛡️ Буфер для export/import данных (может быть большой файл)
    // Увеличен до 16KB для поддержки экспорта 50+ паролей (plaintext ~4KB → base64 ciphertext ~6KB + overhead)
    DynamicJsonDocument doc(16384); // 16KB для {salt, iv, ciphertext}
    DeserializationError error = deserializeJson(doc, encryptedJson);
    if (error) {
        LOG_ERROR("CryptoManager", "Failed to parse encrypted JSON: " + String(error.c_str()));
        return "";
    }

    if (!doc["salt"].is<String>() || !doc["iv"].is<String>() || !doc["ciphertext"].is<String>()) {
        LOG_ERROR("CryptoManager", "Encrypted JSON is missing required fields (salt, iv, ciphertext).");
        return "";
    }

    // 1. Decode all components from Base64
    std::vector<uint8_t> salt = base64Decode(doc["salt"].as<String>());
    std::vector<uint8_t> iv = base64Decode(doc["iv"].as<String>());
    std::vector<uint8_t> ciphertext = base64Decode(doc["ciphertext"].as<String>());

    if (salt.empty() || iv.empty() || ciphertext.empty() || iv.size() != 16 || ciphertext.size() % 16 != 0) {
        LOG_ERROR("CryptoManager", "Invalid data format after Base64 decoding.");
        return "";
    }

    // 2. Re-derive the key using the provided password and the extracted salt
    const int key_len = 32;
    const int iterations = PBKDF2_ITERATIONS_EXPORT; // 15,000 iterations, defined in config.h
    
    LOG_INFO("CryptoManager", "Deriving decryption key with PBKDF2 (" + String(iterations) + " iterations)...");
    unsigned long start_time = millis();
    
    // ⚠️ Сбрасываем watchdog перед PBKDF2 (~3s)
    esp_task_wdt_reset();
    
    uint8_t derived_key[key_len];
    mbedtls_md_context_t sha256_ctx;
    mbedtls_md_init(&sha256_ctx);
    mbedtls_md_setup(&sha256_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    mbedtls_pkcs5_pbkdf2_hmac(&sha256_ctx, (const unsigned char*)password.c_str(), password.length(), salt.data(), salt.size(), iterations, key_len, derived_key);
    mbedtls_md_free(&sha256_ctx);
    
    esp_task_wdt_reset(); // Сбрасываем после PBKDF2
    
    unsigned long pbkdf2_elapsed = millis() - start_time;
    LOG_INFO("CryptoManager", "Key derivation completed in " + String(pbkdf2_elapsed) + "ms");

    // 3. Decrypt data
    std::vector<uint8_t> decrypted_padded(ciphertext.size());
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, derived_key, 256);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, ciphertext.size(), iv.data(), ciphertext.data(), decrypted_padded.data());
    mbedtls_aes_free(&aes);

    // 4. Unpad data (PKCS7)
    if(decrypted_padded.empty()) return "";
    uint8_t padding_len = decrypted_padded.back();
    if (padding_len > 16 || padding_len == 0 || padding_len > decrypted_padded.size()) {
        LOG_ERROR("CryptoManager", "Decryption failed: Invalid padding length.");
        return "";
    }
    for(size_t i = 0; i < padding_len; ++i) {
        if (decrypted_padded[decrypted_padded.size() - 1 - i] != padding_len) {
            LOG_ERROR("CryptoManager", "Decryption failed: Padding verification failed.");
            return "";
        }
    }

    size_t plain_len = decrypted_padded.size() - padding_len;
    LOG_INFO("CryptoManager", "Data successfully decrypted from import.");
    
    // Create result string BEFORE clearing sensitive data
    String result((char*)decrypted_padded.data(), plain_len);
    
    // Clean up sensitive data
    secure_memzero(derived_key, sizeof(derived_key));
    std::fill(decrypted_padded.begin(), decrypted_padded.end(), 0);
    decrypted_padded.clear();
    
    return result;
}


CryptoManager& CryptoManager::getInstance() {
    static CryptoManager instance;
    return instance;
}

CryptoManager::CryptoManager() : _isKeyInitialized(false), _isDrbgInitialized(false), 
                                 _activeSpace(ActiveSpace::NONE), _hiddenSpaceProvisioned(false),
                                 _shareWifiWithHiddenSpace(false) {}

void CryptoManager::begin() {
    if (_isKeyInitialized) return;

    LOG_INFO("CryptoManager", "Initializing...");
    
    // Initialize mbedTLS entropy and CTR_DRBG for cryptographic random generation
    mbedtls_entropy_init(&_entropy);
    mbedtls_ctr_drbg_init(&_drbg);
    
    // Create personalization string from chip-unique data
    uint8_t pers[32];
    uint64_t mac = ESP.getEfuseMac();
    uint32_t flash_size = ESP.getFlashChipSize();  // ESP32 compatible
    uint32_t t = millis();
    
    memcpy(pers,      &mac,        8);
    memcpy(pers + 8,  &flash_size, 4);
    memcpy(pers + 12, &t,          4);
    
    // Fill remainder with esp_random() as extra seed material
    for (int i = 16; i < 32; i += 4) {
        uint32_t r = esp_random();
        memcpy(pers + i, &r, 4);
    }
    
    int ret = mbedtls_ctr_drbg_seed(&_drbg, mbedtls_entropy_func, &_entropy,
                                     pers, sizeof(pers));
    if (ret != 0) {
        LOG_ERROR("CryptoManager", "DRBG seed failed: " + String(ret));
        // Fall back to esp_random() - _isDrbgInitialized stays false
    } else {
        _isDrbgInitialized = true;
        LOG_INFO("CryptoManager", "DRBG initialized with hardware entropy");
    }
    
    // НОВАЯ ЛОГИКА: НЕ загружаем device key автоматически
    // Он будет загружен через unlockDeviceKeyWithPin() после ввода PIN
    // Или через createEncryptedDeviceKey() при первой загрузке
    
    LOG_INFO("CryptoManager", "CryptoManager ready. Device key will be loaded after PIN entry.");
}

void CryptoManager::secureRandom(uint8_t* buffer, size_t length) {
    if (_isDrbgInitialized) {
        int ret = mbedtls_ctr_drbg_random(&_drbg, buffer, length);
        if (ret == 0) return;
        LOG_WARNING("CryptoManager", "DRBG random failed, falling back to esp_random");
    }
    generateSecureRandomBytes_fallback(buffer, length);
}

void CryptoManager::generateAndSaveKey() {
    LOG_WARNING("CryptoManager", "DEPRECATED: generateAndSaveKey() should not be called directly!");
    LOG_WARNING("CryptoManager", "Use createEncryptedDeviceKey() instead for PIN-protected keys");
}

void CryptoManager::loadKey() {
    LOG_WARNING("CryptoManager", "DEPRECATED: loadKey() should not be called directly!");
    LOG_WARNING("CryptoManager", "Use unlockDeviceKeyWithPin() instead for PIN-protected keys");
}

// --- NEW: PIN-based Device Key Management ---

void CryptoManager::generateNewDeviceKey() {
    LOG_INFO("CryptoManager", "Generating new random device key (32 bytes)...");
    
    // ✅ УЛУЧШЕННАЯ ГЕНЕРАЦИЯ: используем esp_random() эффективно (по 4 байта)
    // esp_random() возвращает uint32_t (4 байта), используем все биты
    for (int i = 0; i < sizeof(_deviceKey); i += 4) {
        uint32_t random_value = esp_random();
        _deviceKey[i]     = (random_value >> 24) & 0xFF;
        _deviceKey[i + 1] = (random_value >> 16) & 0xFF;
        _deviceKey[i + 2] = (random_value >> 8)  & 0xFF;
        _deviceKey[i + 3] = random_value & 0xFF;
    }
    
    // ✅ ДОПОЛНИТЕЛЬНОЕ ПЕРЕМЕШИВАНИЕ: добавляем entropy mixing через SHA256
    // Это защищает от потенциальных слабостей HWRNG на ранних этапах загрузки
    uint8_t entropy_sources[64]; // Дополнительные источники энтропии
    
    // Источник 1: Текущее время (millis)
    uint32_t time_entropy = millis();
    memcpy(entropy_sources, &time_entropy, 4);
    
    // Источник 2: Ещё 4 вызова esp_random() для дополнительной энтропии
    for (int i = 0; i < 4; i++) {
        uint32_t extra_random = esp_random();
        memcpy(entropy_sources + 4 + (i * 4), &extra_random, 4);
    }
    
    // Источник 3: Текущий Device Key (первая генерация)
    memcpy(entropy_sources + 20, _deviceKey, 32);
    
    // Источники 4-5: дополнительные вызовы esp_random() для заполнения оставшихся байтов
    for (int i = 52; i < 64; i += 4) {
        uint32_t r = esp_random();
        memcpy(entropy_sources + i, &r, 4);
    }
    
    // Перемешиваем все источники энтропии через SHA256
    uint8_t mixed_entropy[32];
    mbedtls_md_context_t hash_ctx;
    mbedtls_md_init(&hash_ctx);
    mbedtls_md_setup(&hash_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
    mbedtls_md_starts(&hash_ctx);
    mbedtls_md_update(&hash_ctx, entropy_sources, sizeof(entropy_sources));
    mbedtls_md_finish(&hash_ctx, mixed_entropy);
    mbedtls_md_free(&hash_ctx);
    
    // XOR перемешанной энтропии с Device Key для финального усиления
    for (int i = 0; i < 32; i++) {
        _deviceKey[i] ^= mixed_entropy[i];
    }
    
    LOG_INFO("CryptoManager", "Device key generated with enhanced entropy mixing");
}

bool CryptoManager::isDeviceKeyFileExists() {
    return LittleFS.exists(DEVICE_KEY_FILE);
}

bool CryptoManager::isDeviceKeyEncrypted() {
    if (!LittleFS.exists(DEVICE_KEY_FILE)) {
        return false;
    }
    
    fs::File keyFile = LittleFS.open(DEVICE_KEY_FILE, "r");
    if (!keyFile) {
        LOG_ERROR("CryptoManager", "Failed to open device.key for reading");
        return false;
    }
    
    size_t fileSize = keyFile.size();
    keyFile.close();
    
    // New dual-slot format
    if (fileSize == KEY_FILE_SIZE) {
        return true;
    }
    
    // Legacy formats
    if (fileSize == 81 || fileSize == 65 || fileSize == 49) {
        return true;
    }
    
    // Unencrypted legacy formats
    if (fileSize == 33 || fileSize == 32) {
        return false;
    }
    
    LOG_ERROR("CryptoManager", "Invalid device.key file size: " + String(fileSize));
    return false;
}

bool CryptoManager::createEncryptedDeviceKey(const String& pin) {
    LOG_INFO("CryptoManager", "Creating new encrypted device key with PIN...");
    
    if (pin.length() < 4 || pin.length() > 10) {
        LOG_ERROR("CryptoManager", "Invalid PIN length. Must be 4-10 digits.");
        return false;
    }
    
    // 1. Generate 256 random bytes for the entire file
    uint8_t fileData[KEY_FILE_SIZE];
    secureRandom(fileData, KEY_FILE_SIZE);
    
    // 2. Write random data to file first
    fs::File keyFile = LittleFS.open(DEVICE_KEY_FILE, "w");
    if (!keyFile) {
        LOG_ERROR("CryptoManager", "Failed to open device.key for writing");
        return false;
    }
    keyFile.write(fileData, KEY_FILE_SIZE);
    keyFile.close();
    
    // 3. Generate new device key
    generateNewDeviceKey();
    
    // 4. Prepare slot A data
    uint8_t salt[16], iv[16];
    secureRandom(salt, 16);
    secureRandom(iv, 16);
    
    // Build payload: [MAGIC:4][device_key:32][padding:12]
    uint8_t payload[48];
    memset(payload, 0, 48);
    memcpy(payload, DEVICE_KEY_MAGIC, 4);
    memcpy(payload + 4, _deviceKey, 32);
    // bytes 36-47 remain zero (padding)
    
    // 5. Derive AES key from PIN
    const int key_len = 32;
    const int iterations = PBKDF2_ITERATIONS_PIN;
    
    esp_task_wdt_reset();
    
    uint8_t derived_key[key_len];
    mbedtls_md_context_t sha256_ctx;
    mbedtls_md_init(&sha256_ctx);
    mbedtls_md_setup(&sha256_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    
    int ret = mbedtls_pkcs5_pbkdf2_hmac(
        &sha256_ctx,
        (const unsigned char*)pin.c_str(), pin.length(),
        salt, 16,
        iterations,
        key_len,
        derived_key
    );
    
    mbedtls_md_free(&sha256_ctx);
    esp_task_wdt_reset();
    
    if (ret != 0) {
        LOG_ERROR("CryptoManager", "PBKDF2 derivation failed");
        secure_memzero(derived_key, sizeof(derived_key));
        secure_memzero(payload, sizeof(payload));
        return false;
    }
    
    // 6. Encrypt payload with AES-256-CBC
    uint8_t iv_copy[16];
    memcpy(iv_copy, iv, 16);
    
    uint8_t encrypted[48];
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, derived_key, 256);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, 48, iv_copy, payload, encrypted);
    mbedtls_aes_free(&aes);
    
    // 7. Write slot A to file (overwrite bytes 0-79)
    keyFile = LittleFS.open(DEVICE_KEY_FILE, "r+");
    if (!keyFile) {
        LOG_ERROR("CryptoManager", "Failed to reopen device.key for slot A write");
        secure_memzero(derived_key, sizeof(derived_key));
        secure_memzero(payload, sizeof(payload));
        return false;
    }
    
    keyFile.seek(SLOT_A_OFFSET);
    keyFile.write(salt, 16);
    keyFile.write(iv, 16);
    keyFile.write(encrypted, 48);
    keyFile.close();
    
    secure_memzero(derived_key, sizeof(derived_key));
    secure_memzero(payload, sizeof(payload));
    
    _isKeyInitialized = true;
    _activeSpace = ActiveSpace::A;
    _hiddenSpaceProvisioned = false;
    initSpacePaths();  // Initialize space-aware paths
    
    LOG_INFO("CryptoManager", "Device key created in dual-slot format (256 bytes, slot A)");
    return true;
}

bool CryptoManager::unlockDeviceKeyWithPin(const String& pin) {
    LOG_INFO("CryptoManager", "Unlocking device key with PIN...");
    
    if (!LittleFS.exists(DEVICE_KEY_FILE)) {
        LOG_ERROR("CryptoManager", "Device key file does not exist");
        return false;
    }
    
    // Check file size to determine format
    fs::File keyFile = LittleFS.open(DEVICE_KEY_FILE, "r");
    if (!keyFile) {
        LOG_ERROR("CryptoManager", "Failed to open device.key");
        return false;
    }
    
    size_t fileSize = keyFile.size();
    keyFile.close();
    
    // New dual-slot format
    if (fileSize == KEY_FILE_SIZE) {
        LOG_INFO("CryptoManager", "Detected dual-slot format (256 bytes)");
        
        // Try slot A first
        if (tryDecryptSlot(pin, SLOT_A_OFFSET)) {
            _activeSpace = ActiveSpace::A;
            _isKeyInitialized = true;
            initSpacePaths();  // Initialize space-aware paths
            loadSpaceFlags();  // Load flags from /.net_prefs
            LOG_INFO("CryptoManager", "Device key unlocked from slot A");
            return true;
        }
        
        // Try slot B
        if (tryDecryptSlot(pin, SLOT_B_OFFSET)) {
            _activeSpace = ActiveSpace::B;
            _isKeyInitialized = true;
            _hiddenSpaceProvisioned = true;
            initSpacePaths();  // Initialize space-aware paths
            loadSpaceFlags();  // Load flags from /.net_prefs
            LOG_INFO("CryptoManager", "Device key unlocked from alternate slot");
            return true;
        }
        
        LOG_ERROR("CryptoManager", "Failed to decrypt with PIN from both slots");
        return false;
    }
    
    // Legacy format - use old decryption logic
    LOG_INFO("CryptoManager", "Detected legacy format, using backward compatibility mode");
    if (!decryptDeviceKeyWithPin(pin)) {
        LOG_ERROR("CryptoManager", "Failed to decrypt device key with PIN");
        return false;
    }
    
    _isKeyInitialized = true;
    _activeSpace = ActiveSpace::A; // Legacy formats are always space A
    initSpacePaths();  // Initialize space-aware paths
    LOG_INFO("CryptoManager", "Device key unlocked successfully (legacy format)");
    return true;
}

bool CryptoManager::encryptDeviceKeyWithPin(const String& pin) {
    LOG_DEBUG("CryptoManager", "Encrypting device key with PIN...");
    
    const int salt_len = 16;
    const int iv_len = 16;
    const int key_len = 32;
    const int checksum_len = 4; // Первые 4 байта SHA256
    const int plaintext_len = checksum_len + key_len; // 36 байт
    const int encrypted_len = 48; // 3 блока AES (округляем до 16)
    const int iterations = PBKDF2_ITERATIONS_PIN;
    
    // 1. Генерируем случайную соль и IV
    uint8_t salt[salt_len];
    uint8_t iv[iv_len];
    secureRandom(salt, salt_len);
    secureRandom(iv, iv_len);
    
    uint8_t iv_copy[iv_len];
    memcpy(iv_copy, iv, iv_len);  // mbedtls_aes_crypt_cbc modifies IV in place
    
    // 2. Вычисляем SHA256 checksum device key (первые 4 байта)
    uint8_t key_hash[32];
    mbedtls_md_context_t hash_ctx;
    mbedtls_md_init(&hash_ctx);
    mbedtls_md_setup(&hash_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
    mbedtls_md_starts(&hash_ctx);
    mbedtls_md_update(&hash_ctx, _deviceKey, 32);
    mbedtls_md_finish(&hash_ctx, key_hash);
    mbedtls_md_free(&hash_ctx);
    
    // 3. Формируем plaintext: [checksum 4][key 32] + padding до 48 байт
    uint8_t plaintext[encrypted_len];
    memset(plaintext, 0, encrypted_len); // Заполняем нулями для padding
    memcpy(plaintext, key_hash, checksum_len); // Первые 4 байта - checksum
    memcpy(plaintext + checksum_len, _deviceKey, key_len); // Следующие 32 байта - ключ
    
    // 4. Выводим ключ шифрования из PIN + salt
    esp_task_wdt_reset();
    
    uint8_t derived_key[key_len];
    mbedtls_md_context_t sha256_ctx;
    mbedtls_md_init(&sha256_ctx);
    mbedtls_md_setup(&sha256_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    
    int ret = mbedtls_pkcs5_pbkdf2_hmac(
        &sha256_ctx,
        (const unsigned char*)pin.c_str(), pin.length(),
        salt, salt_len,
        iterations,
        key_len,
        derived_key
    );
    
    mbedtls_md_free(&sha256_ctx);
    esp_task_wdt_reset();
    
    if (ret != 0) {
        LOG_ERROR("CryptoManager", "PBKDF2 derivation failed");
        secure_memzero(derived_key, sizeof(derived_key));
        secure_memzero(plaintext, sizeof(plaintext));
        return false;
    }
    
    // 5. AES-256-CBC — IV prevents identical plaintexts producing identical ciphertext
    uint8_t encrypted[encrypted_len];
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, derived_key, 256);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, encrypted_len, iv_copy, plaintext, encrypted);
    mbedtls_aes_free(&aes);
    
    // 6. v3 format: [flag=0x03][salt 16][iv 16][encrypted 48] = 81 bytes
    fs::File keyFile = LittleFS.open(DEVICE_KEY_FILE, "w");
    if (!keyFile) {
        LOG_ERROR("CryptoManager", "Failed to open device.key for writing");
        secure_memzero(derived_key, sizeof(derived_key));
        secure_memzero(plaintext, sizeof(plaintext));
        return false;
    }
    
    uint8_t flag = 0x03;
    keyFile.write(&flag, 1);
    keyFile.write(salt, salt_len);
    keyFile.write(iv, iv_len);
    keyFile.write(encrypted, encrypted_len);
    keyFile.close();
    
    secure_memzero(derived_key, sizeof(derived_key));
    secure_memzero(plaintext, sizeof(plaintext));
    LOG_INFO("CryptoManager", "Device key encrypted with AES-256-CBC (v3, 81 bytes)");
    
    return true;
}

bool CryptoManager::decryptDeviceKeyWithPin(const String& pin) {
    LOG_DEBUG("CryptoManager", "Decrypting device key with PIN...");
    
    fs::File keyFile = LittleFS.open(DEVICE_KEY_FILE, "r");
    if (!keyFile) {
        LOG_ERROR("CryptoManager", "Failed to open device.key");
        return false;
    }
    
    size_t fileSize = keyFile.size();
    LOG_DEBUG("CryptoManager", "Device key file size: " + String(fileSize) + " bytes");
    
    // Проверяем формат файла
    if (fileSize == 81) {
        // ✅ v3: AES-256-CBC with IV [flag=0x03][salt 16][iv 16][encrypted 48]
        uint8_t flag = keyFile.read();
        LOG_DEBUG("CryptoManager", "CBC decrypt: flag=0x" + String(flag, HEX) + " size=81");
        
        if (flag != 0x03) {
            LOG_ERROR("CryptoManager", "Invalid v3 flag (expected 0x03, got 0x" + String(flag,HEX) + ")");
            keyFile.close();
            return false;
        }
        
        uint8_t salt[16];
        uint8_t iv[16];
        uint8_t encrypted[48];
        keyFile.read(salt, 16);
        keyFile.read(iv, 16);
        keyFile.read(encrypted, 48);
        keyFile.close();
        
        const int key_len = 32;
        const int iterations = PBKDF2_ITERATIONS_PIN;
        
        esp_task_wdt_reset();
        
        uint8_t derived_key[key_len];
        mbedtls_md_context_t sha256_ctx;
        mbedtls_md_init(&sha256_ctx);
        mbedtls_md_setup(&sha256_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
        
        int ret = mbedtls_pkcs5_pbkdf2_hmac(
            &sha256_ctx,
            (const unsigned char*)pin.c_str(), pin.length(),
            salt, 16,
            iterations,
            key_len,
            derived_key
        );
        
        mbedtls_md_free(&sha256_ctx);
        esp_task_wdt_reset();
        
        if (ret != 0) {
            LOG_ERROR("CryptoManager", "PBKDF2 failed");
            secure_memzero(derived_key, sizeof(derived_key));
            return false;
        }
        
        // CBC decrypt — IV is consumed in place, use a copy
        uint8_t iv_copy[16];
        memcpy(iv_copy, iv, 16);
        
        uint8_t plaintext[48];
        mbedtls_aes_context aes;
        mbedtls_aes_init(&aes);
        mbedtls_aes_setkey_dec(&aes, derived_key, 256);
        mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, 48, iv_copy, encrypted, plaintext);
        mbedtls_aes_free(&aes);
        
        // verify PIN via checksum: SHA256(decrypted_key)[0:4] == plaintext[0:4]
        uint8_t decrypted_key[32];
        memcpy(decrypted_key, plaintext + 4, 32);
        
        uint8_t computed_hash[32];
        mbedtls_md_context_t hash_ctx;
        mbedtls_md_init(&hash_ctx);
        mbedtls_md_setup(&hash_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
        mbedtls_md_starts(&hash_ctx);
        mbedtls_md_update(&hash_ctx, decrypted_key, 32);
        mbedtls_md_finish(&hash_ctx, computed_hash);
        mbedtls_md_free(&hash_ctx);
        
        if (memcmp(computed_hash, plaintext, 4) != 0) {
            LOG_ERROR("CryptoManager", "PIN verification failed (checksum mismatch)");
            secure_memzero(derived_key, sizeof(derived_key));
            secure_memzero(plaintext, sizeof(plaintext));
            secure_memzero(decrypted_key, sizeof(decrypted_key));
            secure_memzero(computed_hash, sizeof(computed_hash));
            return false;
        }
        
        memcpy(_deviceKey, decrypted_key, 32);
        secure_memzero(derived_key, sizeof(derived_key));
        secure_memzero(plaintext, sizeof(plaintext));
        secure_memzero(decrypted_key, sizeof(decrypted_key));
        secure_memzero(computed_hash, sizeof(computed_hash));
        LOG_INFO("CryptoManager", "Device key decrypted OK (AES-256-CBC v3)");
        return true;
        
    } else if (fileSize == 65) {
        // ✅ Новый формат v2 с SHA256 checksum: [flag=0x02][salt 16][encrypted 48]
        uint8_t flag = keyFile.read();
        LOG_DEBUG("CryptoManager", "File flag: 0x" + String(flag, HEX));
        
        if (flag != 0x02) {
            LOG_ERROR("CryptoManager", "Invalid encrypted device key flag (expected 0x02)");
            keyFile.close();
            return false;
        }
        
        uint8_t salt[16];
        uint8_t encrypted[48];
        
        keyFile.read(salt, 16);
        keyFile.read(encrypted, 48);
        keyFile.close();
        
        // Выводим ключ из PIN + salt
        const int key_len = 32;
        const int iterations = PBKDF2_ITERATIONS_PIN;
        
        esp_task_wdt_reset();
        
        uint8_t derived_key[key_len];
        mbedtls_md_context_t sha256_ctx;
        mbedtls_md_init(&sha256_ctx);
        mbedtls_md_setup(&sha256_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
        
        int ret = mbedtls_pkcs5_pbkdf2_hmac(
            &sha256_ctx,
            (const unsigned char*)pin.c_str(), pin.length(),
            salt, 16,
            iterations,
            key_len,
            derived_key
        );
        
        mbedtls_md_free(&sha256_ctx);
        esp_task_wdt_reset();
        
        if (ret != 0) {
            LOG_ERROR("CryptoManager", "PBKDF2 failed (v2)");
            secure_memzero(derived_key, sizeof(derived_key));
            return false;
        }
        
        // Расшифровываем данные (3 блока по 16 байт)
        uint8_t plaintext[48];
        mbedtls_aes_context aes;
        mbedtls_aes_init(&aes);
        mbedtls_aes_setkey_dec(&aes, derived_key, 256);
        
        for (int i = 0; i < 48; i += 16) {
            mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, encrypted + i, plaintext + i);
        }
        
        mbedtls_aes_free(&aes);
        
        // ✅ ПРОВЕРКА PIN: вычисляем SHA256 расшифрованного ключа
        uint8_t decrypted_key[32];
        memcpy(decrypted_key, plaintext + 4, 32); // Ключ начинается с байта 4
        
        uint8_t computed_hash[32];
        mbedtls_md_context_t hash_ctx;
        mbedtls_md_init(&hash_ctx);
        mbedtls_md_setup(&hash_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
        mbedtls_md_starts(&hash_ctx);
        mbedtls_md_update(&hash_ctx, decrypted_key, 32);
        mbedtls_md_finish(&hash_ctx, computed_hash);
        mbedtls_md_free(&hash_ctx);
        
        // Сравниваем первые 4 байта hash с checksum в plaintext
        if (memcmp(computed_hash, plaintext, 4) != 0) {
            LOG_ERROR("CryptoManager", "❌ PIN verification failed - incorrect PIN!");
            secure_memzero(derived_key, sizeof(derived_key));
            secure_memzero(plaintext, sizeof(plaintext));
            secure_memzero(decrypted_key, sizeof(decrypted_key));
            secure_memzero(computed_hash, sizeof(computed_hash));
            return false;
        }
        
        // Checksum совпадает - PIN правильный, копируем ключ
        memcpy(_deviceKey, decrypted_key, 32);
        
        secure_memzero(derived_key, sizeof(derived_key));
        secure_memzero(plaintext, sizeof(plaintext));
        secure_memzero(decrypted_key, sizeof(decrypted_key));
        secure_memzero(computed_hash, sizeof(computed_hash));
        LOG_INFO("CryptoManager", "✅ Device key decrypted and verified successfully");
        return true;
        
    } else if (fileSize == 49) {
        // Старый формат v1 БЕЗ проверки: [flag=0x01][salt 16][encrypted_key 32]
        LOG_WARNING("CryptoManager", "Legacy encrypted format v1 detected (no PIN verification)");
        uint8_t flag = keyFile.read();
        
        if (flag != 0x01) {
            LOG_ERROR("CryptoManager", "Invalid encrypted device key flag");
            keyFile.close();
            return false;
        }
        
        uint8_t salt[16];
        uint8_t encrypted_key[32];
        
        keyFile.read(salt, 16);
        keyFile.read(encrypted_key, 32);
        keyFile.close();
        
        // Выводим ключ из PIN + salt
        const int key_len = 32;
        const int iterations = PBKDF2_ITERATIONS_PIN;
        
        esp_task_wdt_reset();
        
        uint8_t derived_key[key_len];
        mbedtls_md_context_t sha256_ctx;
        mbedtls_md_init(&sha256_ctx);
        mbedtls_md_setup(&sha256_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
        
        int ret = mbedtls_pkcs5_pbkdf2_hmac(
            &sha256_ctx,
            (const unsigned char*)pin.c_str(), pin.length(),
            salt, 16,
            iterations,
            key_len,
            derived_key
        );
        
        mbedtls_md_free(&sha256_ctx);
        esp_task_wdt_reset();
        
        if (ret != 0) {
            LOG_ERROR("CryptoManager", "PBKDF2 derivation failed");
            return false;
        }
        
        // Расшифровываем device key (БЕЗ проверки - legacy)
        mbedtls_aes_context aes;
        mbedtls_aes_init(&aes);
        mbedtls_aes_setkey_dec(&aes, derived_key, 256);
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, encrypted_key, _deviceKey);
        mbedtls_aes_free(&aes);
        
        LOG_WARNING("CryptoManager", "⚠️ Legacy format - PIN correctness NOT verified!");
        LOG_INFO("CryptoManager", "Device key decrypted (legacy format)");
        return true;
        
    } else if (fileSize == 33) {
        // Старый формат с флагом: [flag=0x00][key 32]
        LOG_WARNING("CryptoManager", "Legacy unencrypted device key format detected (33 bytes)");
        uint8_t flag = keyFile.read();
        
        if (flag == 0x00) {
            // Незашифрованный ключ
            keyFile.read(_deviceKey, 32);
            keyFile.close();
            LOG_INFO("CryptoManager", "Legacy unencrypted key loaded");
            return true;
        } else {
            LOG_ERROR("CryptoManager", "Invalid legacy key format");
            keyFile.close();
            return false;
        }
        
    } else if (fileSize == 32) {
        // Совсем старый формат без флага
        LOG_WARNING("CryptoManager", "Very old unencrypted device key format detected (32 bytes)");
        keyFile.read(_deviceKey, 32);
        keyFile.close();
        LOG_INFO("CryptoManager", "Very old unencrypted key loaded");
        return true;
        
    } else {
        LOG_ERROR("CryptoManager", "Invalid device.key file size: " + String(fileSize));
        keyFile.close();
        return false;
    }
}

bool CryptoManager::saveDeviceKeyEncrypted(const uint8_t* salt, const uint8_t* encryptedKey) {
    fs::File keyFile = LittleFS.open(DEVICE_KEY_FILE, "w");
    if (!keyFile) {
        LOG_ERROR("CryptoManager", "Failed to open device.key for writing");
        return false;
    }
    
    // Формат: [flag=0x01][salt 16][encrypted_key 32] = 49 bytes
    uint8_t flag = 0x01;
    keyFile.write(&flag, 1);
    keyFile.write(salt, 16);
    keyFile.write(encryptedKey, 32);
    keyFile.close();
    
    LOG_DEBUG("CryptoManager", "Encrypted device key saved (49 bytes)");
    return true;
}

bool CryptoManager::saveDeviceKeyUnencrypted() {
    fs::File keyFile = LittleFS.open(DEVICE_KEY_FILE, "w");
    if (!keyFile) {
        LOG_ERROR("CryptoManager", "Failed to open device.key for writing");
        return false;
    }
    
    // Формат: [flag=0x00][key 32] = 33 bytes
    uint8_t flag = 0x00;
    keyFile.write(&flag, 1);
    keyFile.write(_deviceKey, 32);
    keyFile.close();
    
    LOG_DEBUG("CryptoManager", "Unencrypted device key saved (33 bytes)");
    return true;
}

bool CryptoManager::changePinEncryption(const String& oldPin, const String& newPin) {
    LOG_INFO("CryptoManager", "Changing PIN encryption...");
    
    // 1. Расшифровываем текущий device key старым PIN
    if (!unlockDeviceKeyWithPin(oldPin)) {
        LOG_ERROR("CryptoManager", "Failed to unlock with old PIN");
        return false;
    }
    
    // 2. Перешифровываем новым PIN
    if (!encryptDeviceKeyWithPin(newPin)) {
        LOG_ERROR("CryptoManager", "Failed to encrypt with new PIN");
        return false;
    }
    
    LOG_INFO("CryptoManager", "PIN changed successfully");
    return true;
}

bool CryptoManager::disablePinEncryption(const String& currentPin) {
    LOG_INFO("CryptoManager", "Disabling PIN encryption...");
    
    // 1. Расшифровываем device key
    if (!unlockDeviceKeyWithPin(currentPin)) {
        LOG_ERROR("CryptoManager", "Failed to unlock with current PIN");
        return false;
    }
    
    // 2. Сохраняем незашифрованным
    if (!saveDeviceKeyUnencrypted()) {
        LOG_ERROR("CryptoManager", "Failed to save unencrypted key");
        return false;
    }
    
    // 3. Duress PIN is only meaningful when startup PIN entry exists on
    //    the lock screen — with PIN disabled there is no entry point to
    //    trigger it, so it must not be left behind as an orphan.
    if (LittleFS.exists("/duress_pin.hash")) {
        LittleFS.remove("/duress_pin.hash");
        LOG_INFO("CryptoManager", "Duress PIN removed along with startup PIN");
    }
    
    LOG_INFO("CryptoManager", "PIN encryption disabled");
    return true;
}

bool CryptoManager::enablePinEncryption(const String& newPin) {
    LOG_INFO("CryptoManager", "Enabling PIN encryption...");
    
    if (!_isKeyInitialized) {
        LOG_ERROR("CryptoManager", "Device key not initialized");
        return false;
    }
    
    // Шифруем существующий device key
    if (!encryptDeviceKeyWithPin(newPin)) {
        LOG_ERROR("CryptoManager", "Failed to encrypt with new PIN");
        return false;
    }
    
    LOG_INFO("CryptoManager", "PIN encryption enabled");
    return true;
}

// --- Password Hashing (PBKDF2) ---

// Helper to convert byte array to hex string
static String bytesToHex(const uint8_t* data, size_t len) {
    String hexStr = "";
    for (size_t i = 0; i < len; i++) {
        char hex[3];
        sprintf(hex, "%02x", data[i]);
        hexStr += hex;
    }
    return hexStr;
}

// Helper to convert hex string to byte array
static std::vector<uint8_t> hexToBytes(const String& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        String byteString = hex.substring(i, i + 2);
        uint8_t byte = (uint8_t)strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

String CryptoManager::hashPassword(const String& password) {
    const int salt_len = 16;
    const int key_len = 32; // 256-bit derived key
    const int iterations = PBKDF2_ITERATIONS_LOGIN; // 10,000 iterations, defined in config.h

    LOG_INFO("CryptoManager", "Hashing password with PBKDF2 (" + String(iterations) + " iterations)...");
    unsigned long start_time = millis();

    // 1. Generate a random salt
    uint8_t salt[salt_len];
    secureRandom(salt, salt_len);

    // 2. Derive the key using PBKDF2
    // ⚠️ Временно отключаем watchdog, так как PBKDF2 может занять 2+ секунды
    esp_task_wdt_reset();
    
    uint8_t derived_key[key_len];
    mbedtls_md_context_t sha256_ctx;
    mbedtls_md_init(&sha256_ctx);
    mbedtls_md_setup(&sha256_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1); // HMAC

    mbedtls_pkcs5_pbkdf2_hmac(
        &sha256_ctx,
        (const unsigned char*)password.c_str(), password.length(),
        salt, salt_len,
        iterations,
        key_len,
        derived_key
    );

    mbedtls_md_free(&sha256_ctx);
    
    // Сбрасываем watchdog после PBKDF2
    esp_task_wdt_reset();
    
    unsigned long elapsed = millis() - start_time;
    LOG_INFO("CryptoManager", "Password hashed in " + String(elapsed) + "ms");

    // 3. Combine salt and key into "salt:key" format
    String salt_hex = bytesToHex(salt, salt_len);
    String key_hex = bytesToHex(derived_key, key_len);

    secure_memzero(derived_key, sizeof(derived_key));
    return salt_hex + ":" + key_hex;
}

bool CryptoManager::verifyPassword(const String& password, const String& salt_and_hash) {
    int separator_index = salt_and_hash.indexOf(':');
    if (separator_index == -1) return false;

    // 1. Extract salt and original hash
    String salt_hex = salt_and_hash.substring(0, separator_index);
    String original_hash_hex = salt_and_hash.substring(separator_index + 1);

    std::vector<uint8_t> salt = hexToBytes(salt_hex);
    
    const int key_len = 32;
    const int iterations = PBKDF2_ITERATIONS_LOGIN; // 10,000 iterations, defined in config.h

    LOG_DEBUG("CryptoManager", "Verifying password with PBKDF2 (" + String(iterations) + " iterations)...");
    unsigned long start_time = millis();

    // ⚠️ Сбрасываем watchdog перед длительной операцией PBKDF2
    esp_task_wdt_reset();

    // 2. Derive a key from the provided password and the extracted salt
    uint8_t derived_key[key_len];
    mbedtls_md_context_t sha256_ctx;
    mbedtls_md_init(&sha256_ctx);
    mbedtls_md_setup(&sha256_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1); // HMAC

    mbedtls_pkcs5_pbkdf2_hmac(
        &sha256_ctx,
        (const unsigned char*)password.c_str(), password.length(),
        salt.data(), salt.size(),
        iterations,
        key_len,
        derived_key
    );

    mbedtls_md_free(&sha256_ctx);
    
    // Сбрасываем watchdog после PBKDF2
    esp_task_wdt_reset();
    
    unsigned long elapsed = millis() - start_time;
    LOG_DEBUG("CryptoManager", "Password verification completed in " + String(elapsed) + "ms");

    // 3. Compare the new key with the original one
    String derived_key_hex = bytesToHex(derived_key, key_len);
    
    secure_memzero(derived_key, sizeof(derived_key));
    return derived_key_hex.equals(original_hash_hex);
}

// --- Base64 Encoding/Decoding ---

String CryptoManager::base64Encode(const uint8_t* data, size_t len) {
    if (len == 0) return "";

    size_t output_len;
    mbedtls_base64_encode(NULL, 0, &output_len, data, len);

    // Безопасное выделение памяти с автоматическим освобождением
    std::vector<uint8_t> encoded_buf(output_len);
    
    mbedtls_base64_encode(encoded_buf.data(), output_len, &output_len, data, len);
    
    String encoded_str = String((char*)encoded_buf.data());
    return encoded_str;
}

std::vector<uint8_t> CryptoManager::base64Decode(const String& encoded) {
    std::vector<uint8_t> result;
    if (encoded.length() == 0) {
        return result;
    }
    
    size_t output_len;
    mbedtls_base64_decode(NULL, 0, &output_len, (const unsigned char*)encoded.c_str(), encoded.length());

    result.resize(output_len);

    int ret = mbedtls_base64_decode(result.data(), output_len, &output_len, (const unsigned char*)encoded.c_str(), encoded.length());
    if (ret != 0) {
        result.clear();
    }
    result.resize(output_len); // Resize to actual decoded length
    return result;
}


// --- Symmetric Encryption/Decryption ---

bool CryptoManager::encryptData(const uint8_t* plain, size_t plain_len, std::vector<uint8_t>& output) {
    if (!_isKeyInitialized) return false;
    
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, _deviceKey, 256);

    // --- IV Generation ---
    unsigned char iv[16];
    secureRandom(iv, 16);
    unsigned char iv_copy[16];
    memcpy(iv_copy, iv, 16); // mbedtls_aes_crypt_cbc modifies the IV, so we need a copy

    // PKCS7 Padding
    size_t padding_len = 16 - (plain_len % 16);
    size_t padded_len = plain_len + padding_len;
    
    std::vector<uint8_t> padded_input(padded_len);
    memcpy(padded_input.data(), plain, plain_len);
    for(size_t i = 0; i < padding_len; i++) {
        padded_input[plain_len + i] = padding_len;
    }

    // The output will be [IV] + [Ciphertext]
    output.resize(16 + padded_len);
    memcpy(output.data(), iv, 16); // Prepend the IV

    // Encrypt the padded data
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, padded_len, iv_copy, padded_input.data(), output.data() + 16);
    
    mbedtls_aes_free(&aes);
    return true;
}

bool CryptoManager::decryptData(const uint8_t* encrypted, size_t encrypted_len, std::vector<uint8_t>& output) {
    // Must be at least 16 bytes for IV + one block of data, and a multiple of 16
    if (encrypted_len < 32 || encrypted_len % 16 != 0 || !_isKeyInitialized) return false;

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, _deviceKey, 256);

    // Extract IV from the beginning of the data
    unsigned char iv[16];
    memcpy(iv, encrypted, 16);

    const uint8_t* ciphertext = encrypted + 16;
    size_t ciphertext_len = encrypted_len - 16;

    std::vector<uint8_t> decrypted_padded(ciphertext_len);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, ciphertext_len, iv, ciphertext, decrypted_padded.data());
    mbedtls_aes_free(&aes);

    // PKCS7 Unpadding
    if(decrypted_padded.empty()) return false;
    uint8_t padding_len = decrypted_padded.back();
    if (padding_len > 16 || padding_len == 0) return false;
    if (padding_len > decrypted_padded.size()) return false;

    // Verify padding bytes
    for(size_t i = 0; i < padding_len; ++i) {
        if (decrypted_padded[decrypted_padded.size() - 1 - i] != padding_len) {
            return false; // Invalid padding
        }
    }

    size_t plain_len = ciphertext_len - padding_len;
    output.assign(decrypted_padded.begin(), decrypted_padded.begin() + plain_len);
    
    return true;
}

String CryptoManager::encrypt(const String& plaintext) {
    std::vector<uint8_t> encrypted_buffer;
    if (!encryptData((const uint8_t*)plaintext.c_str(), plaintext.length(), encrypted_buffer)) {
        return "";
    }
    return base64Encode(encrypted_buffer.data(), encrypted_buffer.size());
}

String CryptoManager::decrypt(const String& base64_ciphertext) {
    std::vector<uint8_t> encrypted_buffer = base64Decode(base64_ciphertext);
    if (encrypted_buffer.empty()) {
        return "";
    }

    std::vector<uint8_t> decrypted_buffer;
    if (!decryptData(encrypted_buffer.data(), encrypted_buffer.size(), decrypted_buffer)) {
        return "";
    }

    return String((char*)decrypted_buffer.data(), decrypted_buffer.size());
}

// --- BLE PIN Management ---
bool CryptoManager::saveBlePin(uint32_t pin) {
    if (!_isKeyInitialized) {
        Serial.println("[CryptoManager] Error: Key not initialized for BLE PIN encryption");
        return false;
    }

    try {
        // Преобразуем PIN в строку с ведущими нулями (6 цифр)
        char pinStr[7];
        snprintf(pinStr, sizeof(pinStr), "%06u", pin);
        
        LOG_INFO("CryptoManager", "Saving BLE pairing PIN (value protected)");
        
        JsonDocument doc;
        doc["ble_pin"] = String(pinStr); // Сохраняем как строку
        doc["timestamp"] = millis();
        
        String jsonStr;
        serializeJson(doc, jsonStr);
        
        String encrypted = encrypt(jsonStr);
        if (encrypted.isEmpty()) {
            Serial.println("[CryptoManager] Error: Failed to encrypt BLE PIN");
            return false;
        }
        
        // Use space-aware path
        String blePinPath = getSpacePath("ble_pin");
        
        File file = LittleFS.open(blePinPath, "w");
        if (!file) {
            Serial.println("[CryptoManager] Error: Cannot create BLE PIN file");
            return false;
        }
        
        file.print(encrypted);
        file.close();
        
        Serial.println("[CryptoManager] BLE PIN saved and encrypted successfully");
        return true;
        
    } catch (const std::exception& e) {
        Serial.println("[CryptoManager] Exception saving BLE PIN: " + String(e.what()));
        return false;
    }
}

uint32_t CryptoManager::loadBlePin() {
    // Use space-aware path
    String blePinPath = getSpacePath("ble_pin");
    
    if (!LittleFS.exists(blePinPath)) {
        LOG_INFO("CryptoManager", "BLE PIN file not found - first boot detected");
        
        #if BLE_PIN_AUTO_GENERATE
            // Генерируем новый случайный PIN при первом запуске
            uint32_t newPin = generateSecurePin();
            
            // Сохраняем сгенерированный PIN
            if (saveBlePin(newPin)) {
                LOG_INFO("CryptoManager", "New secure BLE PIN generated and saved on first boot");
            } else {
                LOG_ERROR("CryptoManager", "Failed to save generated PIN - using for this session only");
            }
            return newPin;
        #else
            LOG_INFO("CryptoManager", "Auto-generation disabled, using default PIN");
            return 123456; // Default PIN если автогенерация отключена
        #endif
    }
    
    try {
        File file = LittleFS.open(blePinPath, "r");
        if (!file) {
            LOG_ERROR("CryptoManager", "Cannot open BLE PIN file - regenerating");
            uint32_t newPin = generateSecurePin();
            saveBlePin(newPin);
            return newPin;
        }
        
        String encrypted = file.readString();
        file.close();
        
        String decrypted = decrypt(encrypted);
        if (decrypted.isEmpty()) {
            LOG_ERROR("CryptoManager", "Failed to decrypt BLE PIN - regenerating");
            uint32_t newPin = generateSecurePin();
            saveBlePin(newPin);
            return newPin;
        }
        
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, decrypted);
        if (error) {
            LOG_ERROR("CryptoManager", "Failed to parse BLE PIN JSON - regenerating");
            uint32_t newPin = generateSecurePin();
            saveBlePin(newPin);
            return newPin;
        }
        
        // Загружаем PIN как строку и преобразуем в число
        String pinStr = doc["ble_pin"] | "123456";
        uint32_t pin = pinStr.toInt();
        
        LOG_INFO("CryptoManager", "BLE pairing PIN loaded successfully (value protected)");
        return pin;
        
    } catch (const std::exception& e) {
        LOG_ERROR("CryptoManager", "Exception loading BLE PIN: " + String(e.what()) + " - regenerating");
        uint32_t newPin = generateSecurePin();
        saveBlePin(newPin);
        return newPin;
    }
}

bool CryptoManager::isBlePinConfigured() {
    // Use space-aware path
    String blePinPath = getSpacePath("ble_pin");
    return LittleFS.exists(blePinPath);
}

// --- Device BLE PIN Management ---
bool CryptoManager::saveDeviceBlePin(uint32_t pin) {
    if (!_isKeyInitialized) {
        Serial.println("[CryptoManager] Error: Key not initialized for Device BLE PIN encryption");
        return false;
    }

    try {
        // Преобразуем PIN в строку с ведущими нулями (6 цифр)
        char pinStr[7];
        snprintf(pinStr, sizeof(pinStr), "%06u", pin);
        
        LOG_INFO("CryptoManager", "Saving Device BLE PIN (value protected)");
        
        JsonDocument doc;
        doc["device_ble_pin"] = String(pinStr); // Сохраняем как строку
        doc["enabled"] = true;
        doc["timestamp"] = millis();
        
        String jsonStr;
        serializeJson(doc, jsonStr);
        
        LOG_DEBUG("CryptoManager", "Device BLE PIN JSON prepared (content protected)");
        
        String encrypted = encrypt(jsonStr);
        if (encrypted.isEmpty()) {
            Serial.println("[CryptoManager] Error: Failed to encrypt Device BLE PIN");
            return false;
        }
        
        // Use space-aware path
        String deviceBlePinPath = getSpacePath("device_ble_pin");
        
        File file = LittleFS.open(deviceBlePinPath, "w");
        if (!file) {
            Serial.println("[CryptoManager] Error: Cannot create Device BLE PIN file");
            return false;
        }
        
        file.print(encrypted);
        file.close();
        
        Serial.println("[CryptoManager] Device BLE PIN saved and encrypted successfully");
        return true;
        
    } catch (const std::exception& e) {
        Serial.println("[CryptoManager] Exception saving Device BLE PIN: " + String(e.what()));
        return false;
    }
}

uint32_t CryptoManager::loadDeviceBlePin() {
    // Use space-aware path
    String deviceBlePinPath = getSpacePath("device_ble_pin");
    
    if (!LittleFS.exists(deviceBlePinPath)) {
        LOG_INFO("CryptoManager", "Device BLE PIN file not found");
        return 0xFFFFFFFF; // Специальное значение означающее "не настроен"
    }
    
    try {
        File file = LittleFS.open(deviceBlePinPath, "r");
        if (!file) {
            LOG_ERROR("CryptoManager", "Cannot open Device BLE PIN file");
            return 0xFFFFFFFF;
        }
        
        String encrypted = file.readString();
        file.close();
        
        String decrypted = decrypt(encrypted);
        if (decrypted.isEmpty()) {
            LOG_ERROR("CryptoManager", "Failed to decrypt Device BLE PIN");
            return 0xFFFFFFFF;
        }
        
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, decrypted);
        if (error) {
            LOG_ERROR("CryptoManager", "Failed to parse Device BLE PIN JSON");
            return 0xFFFFFFFF;
        }
        
        // Загружаем PIN как строку и преобразуем в число
        String pinStr = doc["device_ble_pin"] | "000000";
        uint32_t pin = pinStr.toInt();
        
        LOG_INFO("CryptoManager", "Device BLE PIN loaded successfully (value protected)");
        return pin;
        
    } catch (const std::exception& e) {
        LOG_ERROR("CryptoManager", "Exception loading Device BLE PIN: " + String(e.what()));
        return 0xFFFFFFFF;
    }
}

bool CryptoManager::isDeviceBlePinConfigured() {
    // Просто проверяем существование файла без попытки расшифровки
    // Это избегает ошибок "Failed to decrypt Device BLE PIN" когда PIN отключен
    // Use space-aware path
    String deviceBlePinPath = getSpacePath("device_ble_pin");
    return LittleFS.exists(deviceBlePinPath);
}

bool CryptoManager::isDeviceBlePinEnabled() {
    // Use space-aware path
    String deviceBlePinPath = getSpacePath("device_ble_pin");
    
    if (!LittleFS.exists(deviceBlePinPath)) {
        return false;
    }
    
    try {
        File file = LittleFS.open(deviceBlePinPath, "r");
        if (!file) {
            return false;
        }
        
        String encrypted = file.readString();
        file.close();
        
        String decrypted = decrypt(encrypted);
        if (decrypted.isEmpty()) {
            return false;
        }
        
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, decrypted);
        if (error) {
            return false;
        }
        
        bool enabled = doc["enabled"] | false;
        LOG_DEBUG("CryptoManager", "Device BLE PIN enabled status: " + String(enabled));
        return enabled;
        
    } catch (const std::exception& e) {
        LOG_ERROR("CryptoManager", "Exception checking Device BLE PIN enabled: " + String(e.what()));
        return false;
    }
}

void CryptoManager::setDeviceBlePinEnabled(bool enabled) {
    if (!isDeviceBlePinConfigured()) {
        LOG_WARNING("CryptoManager", "Device BLE PIN not configured, cannot change enabled state");
        return;
    }
    
    try {
        // Загружаем текущий PIN
        uint32_t currentPin = loadDeviceBlePin();
        if (currentPin == 0xFFFFFFFF) {
            LOG_ERROR("CryptoManager", "Failed to load current Device BLE PIN");
            return;
        }
        
        // Преобразуем PIN в строку с ведущими нулями
        char pinStr[7];
        snprintf(pinStr, sizeof(pinStr), "%06u", currentPin);
        
        // Сохраняем с новым статусом enabled
        JsonDocument doc;
        doc["device_ble_pin"] = String(pinStr);
        doc["enabled"] = enabled;
        doc["timestamp"] = millis();
        
        String jsonStr;
        serializeJson(doc, jsonStr);
        
        String encrypted = encrypt(jsonStr);
        if (encrypted.isEmpty()) {
            LOG_ERROR("CryptoManager", "Failed to encrypt Device BLE PIN");
            return;
        }
        
        // Use space-aware path
        String deviceBlePinPath = getSpacePath("device_ble_pin");
        
        File file = LittleFS.open(deviceBlePinPath, "w");
        if (!file) {
            LOG_ERROR("CryptoManager", "Cannot open Device BLE PIN file for writing");
            return;
        }
        
        file.print(encrypted);
        file.close();
        
        LOG_INFO("CryptoManager", "Device BLE PIN enabled state updated: " + String(enabled ? "enabled" : "disabled"));
        
    } catch (const std::exception& e) {
        LOG_ERROR("CryptoManager", "Exception updating Device BLE PIN enabled state: " + String(e.what()));
    }
}

// ─── Duress PIN ───────────────────────────────────────────────────────────────
// File layout (50 bytes): [0x01 version][0x01/0x00 enabled][16 salt][32 hash]

static int _readDuressPinExpectedLength() {
    int len = 6;
    if (LittleFS.exists("/.sys_ui_prefs")) {
        fs::File f = LittleFS.open("/.sys_ui_prefs", "r");
        if (f) { int v = f.readStringUntil('\n').toInt(); f.close(); if (v >= 4 && v <= 10) len = v; }
    }
    return len;
}

bool CryptoManager::saveDuressPin(const String& pin) {
    int expected = _readDuressPinExpectedLength();
    if ((int)pin.length() != expected) return false;
    for (char c : pin) { if (!isdigit(c)) return false; }
    if (!_isKeyInitialized) { LOG_ERROR("CryptoManager","saveDuressPin: key not ready"); return false; }
    
    // ═══ COLLISION CHECK: duress PIN must not match any space PIN ═══
    bool matchesSlotA = wouldUnlockSlot(pin, 0);
    bool matchesSlotB = _hiddenSpaceProvisioned && wouldUnlockSlot(pin, 80);
    if (matchesSlotA || matchesSlotB) {
        LOG_WARNING("CryptoManager", "saveDuressPin: conflicts with space PIN - rejected");
        return false;
    }
    uint8_t salt[16];
    secureRandom(salt, 16);
    uint8_t hash[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    int ret = mbedtls_pkcs5_pbkdf2_hmac(&ctx, (const unsigned char*)pin.c_str(), pin.length(),
                                         salt, 16, PBKDF2_ITERATIONS_PIN, 32, hash);
    mbedtls_md_free(&ctx);
    if (ret != 0) { LOG_ERROR("CryptoManager","saveDuressPin: PBKDF2 failed"); return false; }
    
    // Use hardcoded Space A path (duress PIN is global)
    String duressPinPath = "/duress_pin.hash";
    
    File f = LittleFS.open(duressPinPath, "w");
    if (!f) { LOG_ERROR("CryptoManager","saveDuressPin: cannot create file"); return false; }
    uint8_t header[2] = {0x01, 0x01};
    size_t w1 = f.write(header, 2);
    size_t w2 = f.write(salt, 16);
    size_t w3 = f.write(hash, 32);
    f.close();
    if (w1 != 2 || w2 != 16 || w3 != 32) {
        LOG_ERROR("CryptoManager", "saveDuressPin: write failed, removing incomplete file");
        LittleFS.remove(duressPinPath);
        return false;
    }
    LOG_INFO("CryptoManager","Duress PIN saved");
    return true;
}

bool CryptoManager::verifyDuressPin(const String& pin) {
    int expected = _readDuressPinExpectedLength();
    if ((int)pin.length() != expected) return false;
    for (char c : pin) { if (!isdigit(c)) return false; }
    
    // Use hardcoded Space A path (duress PIN is global)
    String duressPinPath = "/duress_pin.hash";
    
    if (!LittleFS.exists(duressPinPath)) return false;
    File f = LittleFS.open(duressPinPath, "r");
    if (!f || f.size() != 50) { if (f) f.close(); return false; }
    uint8_t buf[50]; f.read(buf, 50); f.close();
    if (buf[0] != 0x01 || buf[1] != 0x01) {
        secure_memzero(buf, sizeof(buf));
        return false;
    }
    uint8_t computed[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    int ret = mbedtls_pkcs5_pbkdf2_hmac(&ctx, (const unsigned char*)pin.c_str(), pin.length(),
                                         buf+2, 16, PBKDF2_ITERATIONS_PIN, 32, computed);
    mbedtls_md_free(&ctx);
    if (ret != 0) {
        secure_memzero(buf, sizeof(buf));
        secure_memzero(computed, sizeof(computed));
        return false;
    }
    int diff = 0;
    for (int i = 0; i < 32; i++) diff |= (buf[18+i] ^ computed[i]);
    secure_memzero(buf, sizeof(buf));
    secure_memzero(computed, sizeof(computed));
    return (diff == 0);
}

bool CryptoManager::isDuressPinConfigured() {
    // Use hardcoded Space A path (duress PIN is global)
    String duressPinPath = "/duress_pin.hash";
    
    if (!LittleFS.exists(duressPinPath)) return false;
    File f = LittleFS.open(duressPinPath, "r");
    bool ok = (f && f.size() == 50); if (f) f.close(); return ok;
}

bool CryptoManager::isDuressPinEnabled() {
    // Use hardcoded Space A path (duress PIN is global)
    String duressPinPath = "/duress_pin.hash";
    
    if (!LittleFS.exists(duressPinPath)) return false;
    File f = LittleFS.open(duressPinPath, "r");
    if (!f || f.size() != 50) { if (f) f.close(); return false; }
    uint8_t h[2]; f.read(h, 2); f.close();
    return (h[0] == 0x01 && h[1] == 0x01);
}

bool CryptoManager::setDuressPinEnabled(bool enabled) {
    // Use hardcoded Space A path (duress PIN is global)
    String duressPinPath = "/duress_pin.hash";
    
    if (!LittleFS.exists(duressPinPath)) return false;
    File f = LittleFS.open(duressPinPath, "r+");
    if (!f || f.size() != 50) { if (f) f.close(); return false; }
    f.seek(1); f.write(enabled ? 0x01 : 0x00); f.close();
    LOG_INFO("CryptoManager","Duress PIN enabled=" + String(enabled));
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────

bool CryptoManager::verifyDeviceBlePin(const String& pin) {
    LOG_INFO("CryptoManager", "Verifying Device BLE PIN...");
    
    // Проверка длины PIN
    if (pin.length() != 6) {
        LOG_WARNING("CryptoManager", "Device BLE PIN must be 6 digits");
        return false;
    }
    
    // Проверка что PIN состоит только из цифр
    for (char c : pin) {
        if (!isdigit(c)) {
            LOG_WARNING("CryptoManager", "Device BLE PIN must contain only digits");
            return false;
        }
    }
    
    // Загружаем сохранённый PIN
    uint32_t savedPin = loadDeviceBlePin();
    if (savedPin == 0xFFFFFFFF) {
        LOG_WARNING("CryptoManager", "Device BLE PIN not configured");
        return false;
    }
    
    // Преобразуем сохранённый PIN в строку с ведущими нулями
    char savedPinStr[7];
    snprintf(savedPinStr, sizeof(savedPinStr), "%06u", savedPin);
    
    // Сравниваем строки
    bool match = (pin == String(savedPinStr));
    
    if (match) {
        LOG_INFO("CryptoManager", "Device BLE PIN verified successfully");
    } else {
        LOG_WARNING("CryptoManager", "Device BLE PIN verification failed");
    }
    
    return match;
}

uint32_t CryptoManager::generateSecurePin() {
    LOG_INFO("CryptoManager", "Generating secure random BLE PIN...");
    
    // range 100000-999999 guarantees 6 digits; no length check needed
    uint32_t randomPin = BLE_PIN_MIN_VALUE + (esp_random() % (BLE_PIN_MAX_VALUE - BLE_PIN_MIN_VALUE + 1));
    
    LOG_DEBUG("CryptoManager", "BLE PIN generated");
    return randomPin;
}

String CryptoManager::generateSecureSessionId() {
    // Generate 128-bit (16 bytes) cryptographically secure session ID
    uint8_t sessionBytes[16];
    secureRandom(sessionBytes, 16);
    
    // Convert to hex string (32 characters)
    String sessionId = "";
    for (int i = 0; i < 16; i++) {
        if (sessionBytes[i] < 16) sessionId += "0";
        sessionId += String(sessionBytes[i], HEX);
    }
    
    LOG_INFO("CryptoManager", "Generated secure session ID (128-bit)");
    return sessionId;
}

String CryptoManager::generateCsrfToken() {
    LOG_DEBUG("CryptoManager", "Generating new CSRF token");
    
    const int token_len = 32; // 256-bit token
    uint8_t token_bytes[token_len];
    
    // Generate random bytes using secure function
    secureRandom(token_bytes, token_len);
    
    // Convert to hex string
    String token = "";
    for (int i = 0; i < token_len; i++) {
        char hex[3];
        sprintf(hex, "%02x", token_bytes[i]);
        token += hex;
    }
    
    LOG_DEBUG("CryptoManager", "CSRF token generated, length: " + String(token.length()));
    return token;
}

String CryptoManager::generateClientId(const String& fingerprint) {
    LOG_DEBUG("CryptoManager", "Generating client ID from fingerprint");
    
    // Hash fingerprint to create consistent client ID
    const uint8_t* input = (const uint8_t*)fingerprint.c_str();
    size_t input_len = fingerprint.length();
    uint8_t hash[32]; // SHA-256 output
    
    // Compute SHA-256 hash of fingerprint
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0); // 0 = SHA-256
    mbedtls_sha256_update(&ctx, input, input_len);
    mbedtls_sha256_finish(&ctx, hash);
    mbedtls_sha256_free(&ctx);
    
    // Convert first 16 bytes to hex string for client ID
    String clientId = "";
    for (int i = 0; i < 16; i++) {
        char hex[3];
        sprintf(hex, "%02x", hash[i]);
        clientId += hex;
    }
    
    LOG_DEBUG("CryptoManager", "Client ID generated: " + clientId.substring(0,8) + "...");
    return clientId;
}

bool CryptoManager::saveSession(const String& sessionId, const String& csrfToken, unsigned long createdTime) {
    if (!_isKeyInitialized) {
        LOG_ERROR("CryptoManager", "Cannot save session: crypto not initialized");
        return false;
    }
    
    // ИСПРАВЛЕНО: Сохраняем epoch time вместо millis() для персистентности
    struct tm timeinfo;
    time_t now;
    time(&now);
    
    JsonDocument doc;
    doc["session_id"] = sessionId;
    doc["csrf_token"] = csrfToken;
    doc["created_time_millis"] = createdTime; // Сохраняем millis() для совместимости
    doc["created_time_epoch"] = (unsigned long)now; // Добавляем epoch time
    doc["version"] = 2; // Увеличиваем версию
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Encrypt and save
    String encryptedSession = encrypt(jsonString);
    if (encryptedSession.isEmpty()) {
        LOG_ERROR("CryptoManager", "Failed to encrypt session data");
        return false;
    }
    
    // Use space-aware path
    String sessionPath = getSpacePath("session");
    
    File file = LittleFS.open(sessionPath, "w");
    if (!file) {
        LOG_ERROR("CryptoManager", "Failed to open session file for writing");
        return false;
    }
    
    file.print(encryptedSession);
    file.close();
    
    LOG_INFO("CryptoManager", "Session saved to encrypted flash storage with epoch time");
    return true;
}

bool CryptoManager::loadSession(String& sessionId, String& csrfToken, unsigned long& createdTime) {
    if (!_isKeyInitialized) {
        LOG_ERROR("CryptoManager", "Cannot load session: crypto not initialized");
        return false;
    }
    
    // Check session duration mode - if "until reboot", don't load persistent session
    ConfigManager configManager;
    unsigned long sessionLifetime = configManager.getSessionLifetimeSeconds();
    if (sessionLifetime == 0) {
        LOG_DEBUG("CryptoManager", "Session mode: until reboot - not loading persistent session after reboot");
        return false;
    }
    
    // Use space-aware path
    String sessionPath = getSpacePath("session");
    
    if (!LittleFS.exists(sessionPath)) {
        LOG_DEBUG("CryptoManager", "No persistent session file found");
        return false;
    }
    
    File sessionFile = LittleFS.open(sessionPath, "r");
    if (!sessionFile) {
        LOG_ERROR("CryptoManager", "Failed to open session file");
        return false;
    }
    
    String encryptedData = sessionFile.readString();
    sessionFile.close();
    
    // Decrypt session data
    String decryptedJson = decrypt(encryptedData);
    if (decryptedJson.isEmpty()) {
        LOG_ERROR("CryptoManager", "Failed to decrypt session data");
        clearSession(); // Remove corrupted session
        return false;
    }
    
    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, decryptedJson);
    if (error) {
        LOG_ERROR("CryptoManager", "Failed to parse session JSON: " + String(error.c_str()));
        clearSession(); // Remove corrupted session
        return false;
    }
    
    // Extract session data
    sessionId = doc["session_id"].as<String>();
    csrfToken = doc["csrf_token"].as<String>();
    createdTime = doc["created_time_millis"].as<unsigned long>(); // Для совместимости с web_server.cpp
    
    // Проверяем версию для выбора алгоритма валидации
    int version = doc["version"].as<int>();
    unsigned long epochCreatedTime = 0;
    
    if (version >= 2 && doc.containsKey("created_time_epoch")) {
        epochCreatedTime = doc["created_time_epoch"].as<unsigned long>();
    }
    
    // Validate session data
    if (sessionId.isEmpty() || csrfToken.isEmpty()) {
        LOG_ERROR("CryptoManager", "Invalid session data loaded");
        clearSession();
        return false;
    }
    
    // Check if session is still valid - используем epoch time если доступно
    bool sessionValid = false;
    if (epochCreatedTime > 0) {
        // Новый алгоритм: проверка по epoch time (переживает reboot)
        sessionValid = isSessionValidEpoch(epochCreatedTime);
        LOG_DEBUG("CryptoManager", "Validating session using epoch time");
    } else {
        // Старый алгоритм: проверка по millis() (не переживает reboot)
        sessionValid = isSessionValid(createdTime);
        LOG_DEBUG("CryptoManager", "Validating session using millis() time (legacy)");
    }
    
    if (!sessionValid) {
        LOG_INFO("CryptoManager", "Loaded session has expired, clearing");
        clearSession();
        return false;
    }
    
    LOG_INFO("CryptoManager", "Valid session loaded from encrypted storage");
    return true;
}

bool CryptoManager::clearSession() {
    // Use space-aware path
    String sessionPath = getSpacePath("session");
    
    if (LittleFS.exists(sessionPath)) {
        if (LittleFS.remove(sessionPath)) {
            LOG_INFO("CryptoManager", "Session file cleared from storage");
            return true;
        } else {
            LOG_ERROR("CryptoManager", "Failed to remove session file");
            return false;
        }
    }
    LOG_DEBUG("CryptoManager", "No session file to clear");
    return true;
}

bool CryptoManager::isSessionValid(unsigned long createdTime, unsigned long maxLifetimeSeconds) {
    unsigned long currentTime = millis();
    
    // ИСПРАВЛЕНО: Различаем reboot от настоящего millis() overflow
    if (currentTime < createdTime) {
        // Настоящий overflow происходит только после ~49 дней работы
        // При reboot currentTime будет маленьким (< 1 часа)
        const unsigned long ONE_HOUR_MS = 3600 * 1000;
        
        if (currentTime < ONE_HOUR_MS) {
            // Это reboot, не overflow - сессия могла быть создана до перезагрузки
            LOG_INFO("CryptoManager", "Device rebooted, session from previous boot - treating as expired");
            return false;
        } else {
            // Это настоящий overflow после долгой работы
            LOG_WARNING("CryptoManager", "millis() overflow detected, invalidating session for safety");
            return false;
        }
    }
    
    unsigned long sessionAge = (currentTime - createdTime) / 1000; // Convert to seconds
    bool isValid = sessionAge <= maxLifetimeSeconds;
    
    if (!isValid) {
        LOG_INFO("CryptoManager", "Session expired: age=" + String(sessionAge) + "s, max=" + String(maxLifetimeSeconds) + "s");
    }
    
    return isValid;
}

bool CryptoManager::isSessionValidEpoch(unsigned long epochCreatedTime, unsigned long maxLifetimeSeconds) {
    // ИСПРАВЛЕНО: Получаем актуальную длительность из конфига
    ConfigManager configManager;
    unsigned long actualLifetimeSeconds = configManager.getSessionLifetimeSeconds();
    
    // Специальный режим: сессия до ребута
    if (actualLifetimeSeconds == 0) {
        // В режиме "до ребута" сессия всегда валидна, если файл существует
        LOG_DEBUG("CryptoManager", "Session mode: valid until reboot");
        return true;
    }
    
    time_t now;
    time(&now);
    unsigned long currentEpoch = (unsigned long)now;
    
    // Проверяем что время синхронизировано (не 1970 год)
    if (currentEpoch < 1000000000) { // Примерно 2001 год
        // No wall-clock time available (AP mode without RTC, or time not yet synced).
        // Cannot validate epoch age — treat session as valid for current boot only.
        LOG_DEBUG("CryptoManager", "System time not synchronized — session valid for current boot only (until-reboot mode)");
        return true;
    }
    
    unsigned long sessionAge = currentEpoch - epochCreatedTime;
    bool isValid = sessionAge <= actualLifetimeSeconds;
    
    if (!isValid) {
        LOG_INFO("CryptoManager", "Session expired: age=" + String(sessionAge) + "s, max=" + String(actualLifetimeSeconds) + "s (epoch-based)");
    } else {
        LOG_DEBUG("CryptoManager", "Session valid: age=" + String(sessionAge) + "s, remaining=" + String(actualLifetimeSeconds - sessionAge) + "s (epoch-based)");
    }
    
    return isValid;
}

void CryptoManager::wipeDeviceKey() {
    if (_isKeyInitialized) {
        secure_memzero(_deviceKey, sizeof(_deviceKey));
        _isKeyInitialized = false;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// DUAL-SLOT HIDDEN VOLUME SUPPORT (VeraCrypt-style)
// ═══════════════════════════════════════════════════════════════════════════

bool CryptoManager::tryDecryptSlot(const String& pin, int slotOffset) {
    LOG_DEBUG("CryptoManager", "Attempting to decrypt slot at offset " + String(slotOffset));
    
    // 1. Open file and verify size
    fs::File keyFile = LittleFS.open(DEVICE_KEY_FILE, "r");
    if (!keyFile) {
        LOG_ERROR("CryptoManager", "Failed to open device.key");
        return false;
    }
    
    if (keyFile.size() != KEY_FILE_SIZE) {
        LOG_ERROR("CryptoManager", "Invalid file size: " + String(keyFile.size()));
        keyFile.close();
        return false;
    }
    
    // 2. Read slot data: [salt:16][iv:16][encrypted:48]
    keyFile.seek(slotOffset);
    
    uint8_t salt[16], iv[16], encrypted[48];
    keyFile.read(salt, 16);
    keyFile.read(iv, 16);
    keyFile.read(encrypted, 48);
    keyFile.close();
    
    // 3. Derive AES key from PIN + salt
    const int key_len = 32;
    const int iterations = PBKDF2_ITERATIONS_PIN;
    
    esp_task_wdt_reset();
    
    uint8_t derived_key[key_len];
    mbedtls_md_context_t sha256_ctx;
    mbedtls_md_init(&sha256_ctx);
    mbedtls_md_setup(&sha256_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    
    int ret = mbedtls_pkcs5_pbkdf2_hmac(
        &sha256_ctx,
        (const unsigned char*)pin.c_str(), pin.length(),
        salt, 16,
        iterations,
        key_len,
        derived_key
    );
    
    mbedtls_md_free(&sha256_ctx);
    esp_task_wdt_reset();
    
    if (ret != 0) {
        LOG_ERROR("CryptoManager", "PBKDF2 failed");
        secure_memzero(derived_key, sizeof(derived_key));
        return false;
    }
    
    // 4. Decrypt with AES-256-CBC
    uint8_t iv_copy[16];
    memcpy(iv_copy, iv, 16);
    
    uint8_t plaintext[48];
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, derived_key, 256);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, 48, iv_copy, encrypted, plaintext);
    mbedtls_aes_free(&aes);
    
    // 5. Verify MAGIC bytes
    if (memcmp(plaintext, DEVICE_KEY_MAGIC, 4) != 0) {
        LOG_DEBUG("CryptoManager", "MAGIC mismatch at slot offset " + String(slotOffset));
        secure_memzero(derived_key, sizeof(derived_key));
        secure_memzero(plaintext, sizeof(plaintext));
        return false;
    }
    
    // 6. Extract device key
    memcpy(_deviceKey, plaintext + 4, 32);
    
    secure_memzero(derived_key, sizeof(derived_key));
    secure_memzero(plaintext, sizeof(plaintext));
    
    LOG_INFO("CryptoManager", "Successfully decrypted slot at offset " + String(slotOffset));
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// NEW: Duress PIN Collision Detection
// ═══════════════════════════════════════════════════════════════════════════

bool CryptoManager::wouldUnlockSlot(const String& pin, size_t slotOffset) {
    LOG_DEBUG("CryptoManager", "Checking if PIN would unlock slot at offset " + String(slotOffset));
    
    // Use temporary buffer for key - no side effects
    uint8_t tmpKey[32] = {0};
    bool result = tryDecryptSlotDry(pin, slotOffset, tmpKey);
    secure_memzero(tmpKey, 32);
    
    return result;
}

bool CryptoManager::tryDecryptSlotDry(const String& pin, size_t slotOffset, uint8_t* outKey) {
    // Mirror of tryDecryptSlot but writes to outKey (temp buffer)
    // and does NOT update any member variables (_deviceKey, _activeSpace)
    // Returns true if MAGIC matches
    
    // 1. Open file and verify size
    fs::File keyFile = LittleFS.open(DEVICE_KEY_FILE, "r");
    if (!keyFile) {
        return false;
    }
    
    if (keyFile.size() != KEY_FILE_SIZE) {
        keyFile.close();
        return false;
    }
    
    // 2. Read slot data: [salt:16][iv:16][encrypted:48]
    keyFile.seek(slotOffset);
    
    uint8_t salt[16], iv[16], encrypted[48];
    keyFile.read(salt, 16);
    keyFile.read(iv, 16);
    keyFile.read(encrypted, 48);
    keyFile.close();
    
    // 3. Derive AES key from PIN + salt
    const int key_len = 32;
    const int iterations = PBKDF2_ITERATIONS_PIN;
    
    esp_task_wdt_reset();
    
    uint8_t derived_key[key_len];
    mbedtls_md_context_t sha256_ctx;
    mbedtls_md_init(&sha256_ctx);
    mbedtls_md_setup(&sha256_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    
    int ret = mbedtls_pkcs5_pbkdf2_hmac(
        &sha256_ctx,
        (const unsigned char*)pin.c_str(), pin.length(),
        salt, 16,
        iterations,
        key_len,
        derived_key
    );
    
    mbedtls_md_free(&sha256_ctx);
    esp_task_wdt_reset();
    
    if (ret != 0) {
        secure_memzero(derived_key, sizeof(derived_key));
        return false;
    }
    
    // 4. Decrypt with AES-256-CBC
    uint8_t iv_copy[16];
    memcpy(iv_copy, iv, 16);
    
    uint8_t plaintext[48];
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, derived_key, 256);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, 48, iv_copy, encrypted, plaintext);
    mbedtls_aes_free(&aes);
    
    // 5. Verify MAGIC bytes
    bool magicMatch = (memcmp(plaintext, DEVICE_KEY_MAGIC, 4) == 0);
    
    if (magicMatch && outKey) {
        // Extract device key to output buffer (for caller inspection if needed)
        memcpy(outKey, plaintext + 4, 32);
    }
    
    secure_memzero(derived_key, sizeof(derived_key));
    secure_memzero(plaintext, sizeof(plaintext));
    
    return magicMatch;
}

bool CryptoManager::isHiddenSpaceProvisioned() {
    // This flag is set when:
    // 1. createHiddenSpace() succeeds
    // 2. unlockDeviceKeyWithPin() successfully decrypts slot B
    // 3. tryDecryptSlot() reads the flag from slot A payload
    return _hiddenSpaceProvisioned;
}

bool CryptoManager::createHiddenSpace(const String& pin) {
    LOG_INFO("CryptoManager", "Initializing secondary slot...");
    
    // 1. Verify we're in Space A
    if (_activeSpace != ActiveSpace::A) {
        LOG_ERROR("CryptoManager", "Secondary slot can only be created from primary context");
        return false;
    }
    
    if (!_isKeyInitialized) {
        LOG_ERROR("CryptoManager", "Device key not initialized");
        return false;
    }

    if (!isDeviceKeyEncrypted()) {
        LOG_ERROR("CryptoManager", "Cannot create Hidden Space: startup PIN is not enabled (device key is unencrypted)");
        return false;
    }
    
    // 2. Generate new random device key for space B
    uint8_t device_key_B[32];
    secureRandom(device_key_B, 32);
    
    // 3. Generate salt and IV for slot B
    uint8_t salt_B[16], iv_B[16];
    secureRandom(salt_B, 16);
    secureRandom(iv_B, 16);
    
    // 4. Build payload B: [MAGIC:4][device_key_B:32][padding:12]
    uint8_t payload_B[48];
    memset(payload_B, 0, 48);
    memcpy(payload_B, DEVICE_KEY_MAGIC, 4);
    memcpy(payload_B + 4, device_key_B, 32);
    // bytes 36-47 remain zero (padding)
    
    // 5. Derive AES key from PIN
    const int key_len = 32;
    const int iterations = PBKDF2_ITERATIONS_PIN;
    
    esp_task_wdt_reset();
    
    uint8_t derived_key_B[key_len];
    mbedtls_md_context_t sha256_ctx;
    mbedtls_md_init(&sha256_ctx);
    mbedtls_md_setup(&sha256_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    
    int ret = mbedtls_pkcs5_pbkdf2_hmac(
        &sha256_ctx,
        (const unsigned char*)pin.c_str(), pin.length(),
        salt_B, 16,
        iterations,
        key_len,
        derived_key_B
    );
    
    mbedtls_md_free(&sha256_ctx);
    esp_task_wdt_reset();
    
    if (ret != 0) {
        LOG_ERROR("CryptoManager", "PBKDF2 derivation failed");
        secure_memzero(derived_key_B, sizeof(derived_key_B));
        secure_memzero(payload_B, sizeof(payload_B));
        secure_memzero(device_key_B, sizeof(device_key_B));
        return false;
    }
    
    // 6. Encrypt payload B with AES-256-CBC
    uint8_t iv_copy[16];
    memcpy(iv_copy, iv_B, 16);
    
    uint8_t encrypted_B[48];
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, derived_key_B, 256);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, 48, iv_copy, payload_B, encrypted_B);
    mbedtls_aes_free(&aes);
    
    // 7. Write slot B to file
    fs::File keyFile = LittleFS.open(DEVICE_KEY_FILE, "r+");
    if (!keyFile) {
        LOG_ERROR("CryptoManager", "Failed to open device key file for secondary slot write");
        secure_memzero(derived_key_B, sizeof(derived_key_B));
        secure_memzero(payload_B, sizeof(payload_B));
        secure_memzero(device_key_B, sizeof(device_key_B));
        return false;
    }
    
    keyFile.seek(SLOT_B_OFFSET);
    keyFile.write(salt_B, 16);
    keyFile.write(iv_B, 16);
    keyFile.write(encrypted_B, 48);
    keyFile.close();
    
    // 8. Write Space B sentinel — proves provisioning without explicit flag
    String sentinelPath = deriveSpaceBSentinelPath();
    fs::File sf = LittleFS.open(sentinelPath, "w");
    if (sf) {
        sf.print("{}");  // minimal valid content
        sf.close();
        LOG_DEBUG("CryptoManager", "Space B sentinel written: " + sentinelPath);
    } else {
        LOG_WARNING("CryptoManager", "Failed to write configuration marker");
    }
    
    // Clean up sensitive data
    secure_memzero(derived_key_B, sizeof(derived_key_B));
    secure_memzero(payload_B, sizeof(payload_B));
    secure_memzero(device_key_B, sizeof(device_key_B));
    
    LOG_INFO("CryptoManager", "Secondary slot initialized successfully");
    return true;
}

bool CryptoManager::removeHiddenSpaceWithPin(const String& spaceBPin) {
    LOG_INFO("CryptoManager", "Removing hidden space (Space B PIN required)...");

    if (_activeSpace != ActiveSpace::A) {
        LOG_ERROR("CryptoManager", "removeHiddenSpaceWithPin() must be called from Space A");
        return false;
    }

    // 1. Verify the provided PIN actually unlocks Slot B, and obtain key B
    //    into a local buffer WITHOUT touching _deviceKey/_activeSpace.
    uint8_t device_key_B[32] = {0};
    if (!tryDecryptSlotDry(spaceBPin, SLOT_B_OFFSET, device_key_B)) {
        secure_memzero(device_key_B, sizeof(device_key_B));
        LOG_ERROR("CryptoManager", "Space B PIN incorrect - hidden space NOT removed");
        return false;
    }

    // 2. Delete the sentinel file using the CURRENT _deviceKey (Space A key),
    //    since the sentinel is intentionally addressed via Space A's key.
    String sentinelPath = deriveSpaceBSentinelPath();
    if (LittleFS.exists(sentinelPath)) {
        LittleFS.remove(sentinelPath);
        LOG_INFO("CryptoManager", "Deleted sentinel file: " + sentinelPath);
    }

    // 3. Delete all Space B private files using the correct key (device_key_B)
    const char* logicalNames[] = {
        "keys", "passwords", "wifi", "session",
        "ble_pin", "device_ble_pin", "duress_pin", "web_admin", "pin_config",
        "theme", "startup_mode", "hid_mode", "boot_mode"
    };

    for (const char* name : logicalNames) {
        uint8_t hmac[32];
        mbedtls_md_context_t md_ctx;
        mbedtls_md_init(&md_ctx);
        mbedtls_md_setup(&md_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
        mbedtls_md_hmac_starts(&md_ctx, device_key_B, 32);
        mbedtls_md_hmac_update(&md_ctx, (const uint8_t*)name, strlen(name));
        mbedtls_md_hmac_finish(&md_ctx, hmac);
        mbedtls_md_free(&md_ctx);

        char hexBuf[9];
        snprintf(hexBuf, sizeof(hexBuf), "%02x%02x%02x%02x",
                 hmac[0], hmac[1], hmac[2], hmac[3]);
        String filePath = String("/") + hexBuf + ".enc";

        if (LittleFS.exists(filePath)) {
            LittleFS.remove(filePath);
            LOG_INFO("CryptoManager", "Deleted Space B file: " + filePath);
        }
    }

    secure_memzero(device_key_B, sizeof(device_key_B));

    // 4. Also delete shared cache and network preferences
    if (LittleFS.exists("/.conn_cache")) {
        LittleFS.remove("/.conn_cache");
        LOG_INFO("CryptoManager", "Deleted shared WiFi cache");
    }

    if (LittleFS.exists(SPACE_FLAGS_FILE)) {
        LittleFS.remove(SPACE_FLAGS_FILE);
        LOG_INFO("CryptoManager", "Deleted network preferences file");
    }

    // 5. Overwrite Slot B region with random data (position-based, no key needed)
    uint8_t random_data[80];
    secureRandom(random_data, 80);

    fs::File keyFile = LittleFS.open(DEVICE_KEY_FILE, "r+");
    if (!keyFile) {
        LOG_ERROR("CryptoManager", "Failed to open device key file for secondary slot clear");
        secure_memzero(random_data, sizeof(random_data));
        return false;
    }
    keyFile.seek(SLOT_B_OFFSET);
    keyFile.write(random_data, 80);
    keyFile.close();
    secure_memzero(random_data, sizeof(random_data));

    _hiddenSpaceProvisioned = false;
    _shareWifiWithHiddenSpace = false;

    LOG_INFO("CryptoManager", "Hidden space removed successfully");
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// SPACE-AWARE FILE PATH DERIVATION
// ═══════════════════════════════════════════════════════════════════════════

void CryptoManager::initSpacePaths() {
    LOG_INFO("CryptoManager", "Initializing space-aware file paths for Space " + 
             String(_activeSpace == ActiveSpace::A ? "A" : (_activeSpace == ActiveSpace::B ? "B" : "NONE")));
    
    _spacePaths.clear();
    
    if (_activeSpace == ActiveSpace::A) {
        // Space A: use original paths from config.h
        _spacePaths["keys"] = KEYS_FILE;                          // "/keys.json.enc"
        _spacePaths["passwords"] = PASSWORD_FILE;                 // "/passwords.json.enc"
        _spacePaths["wifi"] = "/wifi_config.json.enc";
        _spacePaths["session"] = "/session.json.enc";
        _spacePaths["ble_pin"] = "/ble_pin.json.enc";
        _spacePaths["device_ble_pin"] = "/device_ble_pin.json.enc";
        _spacePaths["duress_pin"] = "/duress_pin.hash";
        _spacePaths["web_admin"] = "/web_admin.json";
        _spacePaths["pin_config"] = "/pin_config.json";
        _spacePaths["sentinel"] = "/sentinel.bin";                // Space A sentinel (unused)
        _spacePaths["theme"] = "/theme_pref.json";
        _spacePaths["startup_mode"] = "/startup_pref.json";
        _spacePaths["hid_mode"] = "/hid_mode.json";
        _spacePaths["boot_mode"] = "/boot_mode.json";
        
        LOG_DEBUG("CryptoManager", "Space A paths initialized (using original paths)");
        
    } else if (_activeSpace == ActiveSpace::B) {
        // Space B: derive paths from HMAC-SHA256(deviceKey_B, logicalName)
        const char* logicalNames[] = {
            "keys", "passwords", "wifi", "session",
            "ble_pin", "device_ble_pin", "duress_pin", "web_admin", "pin_config", "sentinel", "theme", "startup_mode", "hid_mode", "boot_mode"
        };
        
        for (const char* name : logicalNames) {
            uint8_t hmac[32];
            
            // Compute HMAC-SHA256(deviceKey_B, logicalName)
            mbedtls_md_context_t md_ctx;
            mbedtls_md_init(&md_ctx);
            mbedtls_md_setup(&md_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
            mbedtls_md_hmac_starts(&md_ctx, _deviceKey, 32);
            mbedtls_md_hmac_update(&md_ctx, (const uint8_t*)name, strlen(name));
            mbedtls_md_hmac_finish(&md_ctx, hmac);
            mbedtls_md_free(&md_ctx);
            
            // Take first 8 hex chars (4 bytes)
            char hexBuf[9];
            snprintf(hexBuf, sizeof(hexBuf), "%02x%02x%02x%02x",
                     hmac[0], hmac[1], hmac[2], hmac[3]);
            
            // Build path: /<hex>.enc (or .bin for sentinel)
            if (strcmp(name, "sentinel") == 0) {
                _spacePaths[name] = String("/") + hexBuf + ".bin";
            } else {
                _spacePaths[name] = String("/") + hexBuf + ".enc";
            }
            
            LOG_DEBUG("CryptoManager", "Space B path: " + String(name) + " -> " + _spacePaths[name]);
        }
        
        LOG_INFO("CryptoManager", "Alternate slot paths initialized (HMAC-derived, indistinguishable from random)");
    }
}

String CryptoManager::getSpacePath(const char* logicalName) const {
    auto it = _spacePaths.find(logicalName);
    if (it != _spacePaths.end()) {
        return it->second;
    }
    
    // Fallback — should never happen if initSpacePaths() was called
    LOG_WARNING("CryptoManager", "getSpacePath: logical name '" + String(logicalName) + 
                "' not found in _spacePaths, using fallback");
    return String("/") + logicalName + ".enc";
}

// ═══════════════════════════════════════════════════════════════════════════
// SPACE B SENTINEL PATH DERIVATION
// ═══════════════════════════════════════════════════════════════════════════

String CryptoManager::deriveSpaceBSentinelPath() {
    // Derive Space B sentinel path using HMAC-SHA256(_deviceKey, "sentinel")
    // Same logic as initSpacePaths() Space B branch
    // Can be called from Space A context using Space A's _deviceKey
    
    uint8_t hmac[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    mbedtls_md_hmac_starts(&ctx, _deviceKey, 32);
    const char* name = "sentinel";
    mbedtls_md_hmac_update(&ctx, (const uint8_t*)name, strlen(name));
    mbedtls_md_hmac_finish(&ctx, hmac);
    mbedtls_md_free(&ctx);
    
    // Take first 4 bytes as hex (8 chars) + .bin extension
    char hexPath[32];
    snprintf(hexPath, sizeof(hexPath), "/%02x%02x%02x%02x.bin",
             hmac[0], hmac[1], hmac[2], hmac[3]);
    
    return String(hexPath);
}

// ═══════════════════════════════════════════════════════════════════════════
// SPACE FLAGS MANAGEMENT (/.net_prefs encrypted file)
// ═══════════════════════════════════════════════════════════════════════════

bool CryptoManager::saveSpaceFlags() {
    if (!_isKeyInitialized) {
        LOG_ERROR("CryptoManager", "Cannot save network preferences: device key not initialized");
        return false;
    }
    
    LOG_DEBUG("CryptoManager", "Saving network preferences: share_wifi=" + String(_shareWifiWithHiddenSpace));
    
    // 1. Build plaintext: [share_wifi:1][padding:15] (hidden_provisioned removed)
    uint8_t plaintext[16];
    memset(plaintext, 0, 16);
    plaintext[0] = _shareWifiWithHiddenSpace ? 0x01 : 0x00;
    // bytes 1-15 remain zero (padding)
    
    // 2. Generate deterministic IV from device key (XOR with 0x5A)
    uint8_t iv[16];
    memcpy(iv, _deviceKey, 16);
    for (int i = 0; i < 16; i++) {
        iv[i] ^= 0x5A;
    }
    
    // 3. Encrypt with AES-256-CBC
    uint8_t ciphertext[16];
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, _deviceKey, 256);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, 16, iv, plaintext, ciphertext);
    mbedtls_aes_free(&aes);
    
    // 4. Write to file
    fs::File file = LittleFS.open(SPACE_FLAGS_FILE, "w");
    if (!file) {
        LOG_ERROR("CryptoManager", "Failed to open network preferences file for writing");
        secure_memzero(plaintext, sizeof(plaintext));
        return false;
    }
    
    file.write(ciphertext, 16);
    file.close();
    
    secure_memzero(plaintext, sizeof(plaintext));
    LOG_INFO("CryptoManager", "Network preferences saved successfully");
    return true;
}

bool CryptoManager::loadSpaceFlags() {
    if (!_isKeyInitialized) {
        LOG_ERROR("CryptoManager", "Cannot load space flags: device key not initialized");
        return false;
    }
    
    // Check hidden space provisioning via sentinel file (HMAC-derived path)
    String sentinelPath = deriveSpaceBSentinelPath();
    _hiddenSpaceProvisioned = LittleFS.exists(sentinelPath);
    LOG_DEBUG("CryptoManager", _hiddenSpaceProvisioned ? 
              "Hidden space detected via sentinel: " + sentinelPath : 
              "No hidden space (sentinel not found)");
    
    // If /.net_prefs doesn't exist, use defaults (wifi not shared)
    if (!LittleFS.exists(SPACE_FLAGS_FILE)) {
        LOG_DEBUG("CryptoManager", "Network preferences file not found, using defaults");
        _shareWifiWithHiddenSpace = false;
        return true;
    }
    
    // 1. Read encrypted data
    fs::File file = LittleFS.open(SPACE_FLAGS_FILE, "r");
    if (!file) {
        LOG_ERROR("CryptoManager", "Failed to open network preferences file for reading");
        return false;
    }
    
    if (file.size() != 16) {
        LOG_ERROR("CryptoManager", "Invalid network preferences file size: " + String(file.size()));
        file.close();
        return false;
    }
    
    uint8_t ciphertext[16];
    file.read(ciphertext, 16);
    file.close();
    
    // 2. Generate deterministic IV from device key (XOR with 0x5A)
    uint8_t iv[16];
    memcpy(iv, _deviceKey, 16);
    for (int i = 0; i < 16; i++) {
        iv[i] ^= 0x5A;
    }
    
    // 3. Decrypt with AES-256-CBC
    uint8_t plaintext[16];
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, _deviceKey, 256);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, 16, iv, ciphertext, plaintext);
    mbedtls_aes_free(&aes);
    
    // 4. Extract flags (only share_wifi now, hidden_provisioned removed)
    _shareWifiWithHiddenSpace = (plaintext[0] == 0x01);
    
    secure_memzero(plaintext, sizeof(plaintext));
    
    LOG_INFO("CryptoManager", "Network preferences loaded: share_wifi=" + String(_shareWifiWithHiddenSpace));
    return true;
}

bool CryptoManager::setShareWifiWithHiddenSpace(bool share) {
    if (!_isKeyInitialized) {
        LOG_ERROR("CryptoManager", "Cannot set WiFi sharing: device key not initialized");
        return false;
    }
    
    LOG_INFO("CryptoManager", "Setting WiFi sharing with alternate slot: " + String(share));
    _shareWifiWithHiddenSpace = share;
    return saveSpaceFlags();
}

// ═══════════════════════════════════════════════════════════════════════════
// CHIP-DERIVED SHARED KEY (for WiFi sharing between spaces)
// ═══════════════════════════════════════════════════════════════════════════

void CryptoManager::deriveChipSharedKey(uint8_t* outKey32) {
    // Hardware chip ID — constant across reboots and spaces
    uint64_t chipId = ESP.getEfuseMac();
    uint8_t seed[8];
    memcpy(seed, &chipId, 8);
    
    // Static salt for domain separation
    const uint8_t salt[16] = {
        0x57, 0x69, 0x46, 0x69, 0x53, 0x68, 0x61, 0x72,
        0x65, 0x64, 0x4B, 0x65, 0x79, 0x00, 0x00, 0x01
    };
    
    // PBKDF2 with 1000 iterations (fast — key is hardware-derived, not user secret)
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    mbedtls_pkcs5_pbkdf2_hmac(&ctx, seed, 8, salt, 16, 1000, 32, outKey32);
    mbedtls_md_free(&ctx);
}

String CryptoManager::encryptForSharedCache(const String& plaintext) {
    LOG_DEBUG("CryptoManager", "Encrypting data for shared cache with chip-derived key");
    
    // Derive chip-based shared key
    uint8_t chipKey[32];
    deriveChipSharedKey(chipKey);
    
    // Generate random IV
    uint8_t iv[16];
    secureRandom(iv, 16);
    uint8_t iv_copy[16];
    memcpy(iv_copy, iv, 16);
    
    // Pad plaintext (PKCS7)
    size_t plain_len = plaintext.length();
    size_t padding_len = 16 - (plain_len % 16);
    size_t padded_len = plain_len + padding_len;
    std::vector<uint8_t> padded_input(padded_len);
    memcpy(padded_input.data(), plaintext.c_str(), plain_len);
    for(size_t i = 0; i < padding_len; i++) {
        padded_input[plain_len + i] = padding_len;
    }
    
    // Encrypt with AES-256-CBC
    std::vector<uint8_t> ciphertext(padded_len);
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, chipKey, 256);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, padded_len, iv_copy, padded_input.data(), ciphertext.data());
    mbedtls_aes_free(&aes);
    
    // Combine IV + ciphertext
    std::vector<uint8_t> combined(16 + padded_len);
    memcpy(combined.data(), iv, 16);
    memcpy(combined.data() + 16, ciphertext.data(), padded_len);
    
    // Clean up sensitive data
    secure_memzero(chipKey, sizeof(chipKey));
    std::fill(padded_input.begin(), padded_input.end(), 0);
    
    // Return base64
    return base64Encode(combined.data(), combined.size());
}

String CryptoManager::decryptFromSharedCache(const String& ciphertext) {
    LOG_DEBUG("CryptoManager", "Decrypting data from shared cache with chip-derived key");
    
    // Decode base64
    std::vector<uint8_t> combined = base64Decode(ciphertext);
    if (combined.size() < 16 || (combined.size() - 16) % 16 != 0) {
        LOG_ERROR("CryptoManager", "Invalid shared cache ciphertext format");
        return "";
    }
    
    // Extract IV and ciphertext
    uint8_t iv[16];
    memcpy(iv, combined.data(), 16);
    size_t encrypted_len = combined.size() - 16;
    std::vector<uint8_t> encrypted_data(encrypted_len);
    memcpy(encrypted_data.data(), combined.data() + 16, encrypted_len);
    
    // Derive chip-based shared key
    uint8_t chipKey[32];
    deriveChipSharedKey(chipKey);
    
    // Decrypt with AES-256-CBC
    std::vector<uint8_t> decrypted_padded(encrypted_len);
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, chipKey, 256);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, encrypted_len, iv, encrypted_data.data(), decrypted_padded.data());
    mbedtls_aes_free(&aes);
    
    // Unpad (PKCS7)
    if(decrypted_padded.empty()) {
        secure_memzero(chipKey, sizeof(chipKey));
        return "";
    }
    uint8_t padding_len = decrypted_padded.back();
    if (padding_len > 16 || padding_len == 0 || padding_len > decrypted_padded.size()) {
        LOG_ERROR("CryptoManager", "Shared cache decryption failed: Invalid padding");
        secure_memzero(chipKey, sizeof(chipKey));
        return "";
    }
    for(size_t i = 0; i < padding_len; ++i) {
        if (decrypted_padded[decrypted_padded.size() - 1 - i] != padding_len) {
            LOG_ERROR("CryptoManager", "Shared cache decryption failed: Padding verification failed");
            secure_memzero(chipKey, sizeof(chipKey));
            return "";
        }
    }
    
    size_t plain_len = decrypted_padded.size() - padding_len;
    String result((char*)decrypted_padded.data(), plain_len);
    
    // Clean up sensitive data
    secure_memzero(chipKey, sizeof(chipKey));
    std::fill(decrypted_padded.begin(), decrypted_padded.end(), 0);
    
    return result;
}
