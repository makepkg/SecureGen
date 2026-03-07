#ifndef CRYPTO_MANAGER_H
#define CRYPTO_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "LittleFS.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

#define DEVICE_KEY_FILE "/device.key"

class CryptoManager {
public:
    static CryptoManager& getInstance();
    void begin();

    // --- Password Hashing ---
    String hashPassword(const String& password);
    bool verifyPassword(const String& password, const String& hash);

    // --- Base64 Encoding/Decoding ---
    String base64Encode(const uint8_t* data, size_t len);
    std::vector<uint8_t> base64Decode(const String& encoded);

    // --- Symmetric Encryption/Decryption (for files) ---
    String encrypt(const String& plaintext);
    String decrypt(const String& base64_ciphertext);

    // --- New Password-based Encryption for Import/Export ---
    String encryptWithPassword(const String& plaintext, const String& password);
    String decryptWithPassword(const String& encryptedJson, const String& password);
    
    // Session ID generation
    String generateSecureSessionId();
    
    // CSRF token generation
    String generateCsrfToken();
    
    // Client ID generation for secure sessions
    String generateClientId(const String& fingerprint);
    
    // Secure random number generation using mbedTLS CTR_DRBG
    void secureRandom(uint8_t* buffer, size_t length);
    
    // --- Persistent Session Management ---
    bool saveSession(const String& sessionId, const String& csrfToken, unsigned long createdTime);
    bool loadSession(String& sessionId, String& csrfToken, unsigned long& createdTime);
    bool clearSession();
    bool isSessionValid(unsigned long createdTime, unsigned long maxLifetimeSeconds = 21600); // Default 6 hours
    bool isSessionValidEpoch(unsigned long epochCreatedTime, unsigned long maxLifetimeSeconds = 21600); // Epoch-based validation default

    bool encryptData(const uint8_t* plain, size_t plain_len, std::vector<uint8_t>& output);
    bool decryptData(const uint8_t* encrypted, size_t encrypted_len, std::vector<uint8_t>& output);

    // --- BLE PIN Management ---
    bool saveBlePin(uint32_t pin);
    uint32_t loadBlePin();
    bool isBlePinConfigured();
    
    // --- Device BLE PIN Management ---
    bool saveDeviceBlePin(uint32_t pin);
    uint32_t loadDeviceBlePin();
    bool isDeviceBlePinConfigured();
    bool isDeviceBlePinEnabled();
    void setDeviceBlePinEnabled(bool enabled);
    bool verifyDeviceBlePin(const String& pin); // Проверка Device BLE PIN
    
    uint32_t generateSecurePin();

    // --- NEW: PIN-based Device Key Encryption ---
    // Проверяет существует ли device.key файл
    bool isDeviceKeyFileExists();
    
    // Проверяет зашифрован ли device.key (flag=0x01)
    bool isDeviceKeyEncrypted();
    
    // Создает новый device key и шифрует его PIN-кодом
    bool createEncryptedDeviceKey(const String& pin);
    
    // Расшифровывает device key используя PIN
    bool unlockDeviceKeyWithPin(const String& pin);
    
    // Проверяет инициализирован ли device key в памяти
    bool isDeviceKeyInitialized() const { return _isKeyInitialized; }
    
    // Изменяет PIN (перешифровывает device key)
    bool changePinEncryption(const String& oldPin, const String& newPin);
    
    // Отключает PIN защиту (сохраняет device key незашифрованным)
    bool disablePinEncryption(const String& currentPin);
    
    // Включает PIN защиту (шифрует существующий device key)
    bool enablePinEncryption(const String& newPin);
    
    // Wipe device key from memory
    void wipeDeviceKey();

private:
    CryptoManager(); // Private constructor
    CryptoManager(const CryptoManager&) = delete;
    void operator=(const CryptoManager&) = delete;

    unsigned char _deviceKey[32]; // 256-bit AES key
    bool _isKeyInitialized;
    mbedtls_entropy_context _entropy;
    mbedtls_ctr_drbg_context _drbg;
    bool _isDrbgInitialized;

    void generateAndSaveKey();
    void loadKey();
    
    // --- NEW: Internal PIN encryption helpers ---
    bool encryptDeviceKeyWithPin(const String& pin);
    bool decryptDeviceKeyWithPin(const String& pin);
    void generateNewDeviceKey();
    bool saveDeviceKeyEncrypted(const uint8_t* salt, const uint8_t* encryptedKey);
    bool saveDeviceKeyUnencrypted();
};

#endif // CRYPTO_MANAGER_H

