#ifndef CRYPTO_MANAGER_H
#define CRYPTO_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <map>
#include "LittleFS.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

#define DEVICE_KEY_FILE "/device.key"

// ActiveSpace enum for dual-slot hidden volume support
enum class ActiveSpace {
    NONE = -1,
    A = 0,
    B = 1
};

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
    
    // --- NEW: Chip-derived shared encryption (for WiFi sharing between spaces) ---
    String encryptForSharedCache(const String& plaintext);
    String decryptFromSharedCache(const String& ciphertext);
    
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

    // Duress PIN — PBKDF2-hashed, verified before device key unlock
    bool saveDuressPin(const String& pin);
    bool verifyDuressPin(const String& pin);
    bool isDuressPinConfigured();
    bool isDuressPinEnabled();
    bool setDuressPinEnabled(bool enabled);
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
    
    // --- NEW: Duress PIN Collision Detection ---
    // Check if a PIN would unlock a specific slot without side effects
    // Returns true if PIN can decrypt the slot (MAGIC matches)
    // Does NOT modify _deviceKey or _activeSpace
    bool wouldUnlockSlot(const String& pin, size_t slotOffset);
    
    // --- NEW: Hidden Volume (Dual-Slot) Support ---
    // Get currently active space (A, B, or NONE)
    ActiveSpace getActiveSpace() const { return _activeSpace; }
    
    // Check if hidden space (slot B) is provisioned
    bool isHiddenSpaceProvisioned();
    
    // Create hidden space (slot B) with a different PIN - must be called from Space A
    bool createHiddenSpace(const String& pin);
    
    // Remove hidden space (slot B) - must be called from Space A, requires
    // Space B's own PIN to correctly locate and delete its HMAC-derived files.
    // Does NOT touch _deviceKey/_activeSpace of the caller's active Space A session.
    bool removeHiddenSpaceWithPin(const String& spaceBPin);
    
    // --- NEW: Space-Aware File Paths ---
    // Get the actual filesystem path for a logical file name based on active space
    String getSpacePath(const char* logicalName) const;
    
    // Initialize space-specific file paths (called after successful unlock)
    void initSpacePaths();
    
    // --- NEW: WiFi Sharing with Hidden Space ---
    bool isWifiSharedWithHiddenSpace() const { return _shareWifiWithHiddenSpace; }
    bool setShareWifiWithHiddenSpace(bool share);

private:
    // Secure memory zeroing — see include/secure_utils.h

    CryptoManager(); // Private constructor
    CryptoManager(const CryptoManager&) = delete;
    void operator=(const CryptoManager&) = delete;

    unsigned char _deviceKey[32]; // 256-bit AES key
    bool _isKeyInitialized;
    mbedtls_entropy_context _entropy;
    mbedtls_ctr_drbg_context _drbg;
    bool _isDrbgInitialized;
    
    // --- NEW: Dual-slot state tracking ---
    ActiveSpace _activeSpace;
    bool _hiddenSpaceProvisioned;
    bool _shareWifiWithHiddenSpace;
    
    // --- NEW: Space-aware file path mapping ---
    std::map<String, String> _spacePaths;  // logicalName → actual LittleFS path

    void generateAndSaveKey();
    void loadKey();
    
    // --- NEW: Internal PIN encryption helpers ---
    bool encryptDeviceKeyWithPin(const String& pin);
    bool decryptDeviceKeyWithPin(const String& pin);
    void generateNewDeviceKey();
    bool saveDeviceKeyEncrypted(const uint8_t* salt, const uint8_t* encryptedKey);
    bool saveDeviceKeyUnencrypted();
    
    // --- NEW: Dual-slot internal helpers ---
    static constexpr uint8_t DEVICE_KEY_MAGIC[4] = {0xA3, 0x7F, 0x2C, 0x91};
    static constexpr int SLOT_A_OFFSET = 0;
    static constexpr int SLOT_B_OFFSET = 80;
    static constexpr int KEY_FILE_SIZE = 256;
    static constexpr char SPACE_FLAGS_FILE[] = "/.net_prefs";
    
    bool tryDecryptSlot(const String& pin, int slotOffset);
    bool tryDecryptSlotDry(const String& pin, size_t slotOffset, uint8_t* outKey);
    bool saveSpaceFlags();
    bool loadSpaceFlags();
    String deriveSpaceBSentinelPath();
    
    // --- NEW: Chip-derived shared key helper ---
    void deriveChipSharedKey(uint8_t* outKey32);
};

#endif // CRYPTO_MANAGER_H

