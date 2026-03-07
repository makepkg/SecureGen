#ifndef SECURE_LAYER_MANAGER_H
#define SECURE_LAYER_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <map>
#include "crypto_manager.h"
#include "log_manager.h"

// mbedTLS заголовки для криптографических операций  
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/gcm.h"
#include "mbedtls/md.h"

// Конфигурация безопасности
#define SECURE_AES_KEY_SIZE 32      // AES-256
#define SECURE_GCM_IV_SIZE 12       // 96 бит для GCM
#define SECURE_GCM_TAG_SIZE 16      // 128 бит тег аутентификации
#define SECURE_MAX_SESSIONS 5       // Максимальное количество безопасных сессий
#define SECURE_SESSION_TIMEOUT 1800000  // 30 минут timeout

/**
 * @brief Менеджер криптографического слоя для end-to-end шифрования
 * 
 * Обеспечивает безопасное шифрование данных поверх AsyncWebServer
 * без замены существующей архитектуры. Интегрируется с CryptoManager.
 */
class SecureLayerManager {
public:
    static SecureLayerManager& getInstance();
    
    // Инициализация и lifecycle
    bool begin();
    void end();
    // ❌ REMOVED: update() - cleanup not needed for 10min timeout web server
    
    // ECDH Key Exchange
    String getServerPublicKey();
    bool processKeyExchange(const String& clientId, const String& clientPubKeyHex, String& response);
    
    // Protected ECDH Key Exchange with device-specific encryption
    bool processProtectedKeyExchange(const String& clientId, const String& encryptedClientKey, String& response);
    
    // Message encryption/decryption
    // ⚡ IRAM_ATTR - критичные функции в IRAM для максимальной скорости
    IRAM_ATTR bool encryptResponse(const String& clientId, const String& plaintext, String& encryptedJson);
    IRAM_ATTR bool decryptRequest(const String& clientId, const String& encryptedJson, String& plaintext);
    
    // Session management
    bool isSecureSessionValid(const String& clientId);
    void invalidateSecureSession(const String& clientId);
    // ❌ REMOVED: cleanupExpiredSessions() - causes race condition
    int getActiveSecureSessionCount();
    
    // Wipe all sessions from memory
    void wipeAllSessions();
    
    // HTTP Middleware integration
    bool shouldBypassSecurity(const String& endpoint);
    String wrapSecureResponse(const String& clientId, const String& originalResponse);
    bool unwrapSecureRequest(const String& clientId, const String& requestBody, String& unwrappedBody);

private:
    struct SecureSession {
        String clientId;
        uint8_t sessionKey[SECURE_AES_KEY_SIZE];
        uint64_t rxCounter;    // Receive counter (replay protection)
        uint64_t txCounter;    // Transmit counter
        bool keyExchanged;
        unsigned long lastActivity;
        uint8_t clientNonce[16];
    };
    
    SecureLayerManager();
    ~SecureLayerManager();
    SecureLayerManager(const SecureLayerManager&) = delete;
    SecureLayerManager& operator=(const SecureLayerManager&) = delete;
    
    // Session management
    SecureSession* findSession(const String& clientId);
    SecureSession* createSession(const String& clientId);
    void removeSession(const String& clientId);
    
    // Cryptographic operations
    bool performECDH(const uint8_t* clientPubKey, size_t keyLen, uint8_t* sharedSecret);
    bool deriveSessionKey(const uint8_t* sharedSecret, const uint8_t* salt, uint8_t* sessionKey);
    bool encryptData(const uint8_t* key, const uint8_t* plaintext, size_t plaintextLen,
                    uint8_t* ciphertext, size_t* ciphertextLen, 
                    uint8_t* iv, uint8_t* tag);
    bool decryptData(const uint8_t* key, const uint8_t* ciphertext, size_t ciphertextLen,
                    const uint8_t* iv, const uint8_t* tag,
                    uint8_t* plaintext, size_t* plaintextLen);
    
    // Utility functions
    String bytesToHex(const uint8_t* bytes, size_t length);
    bool hexToBytes(const String& hex, uint8_t* bytes, size_t maxLength);
    bool generateNonce(uint8_t* nonce, size_t length);
    
    // mbedTLS contexts
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ecdh_context ecdh_context;
    
    // Session storage
    std::map<String, SecureSession> sessions;
    
    // Configuration
    bool initialized;
    unsigned long sessionTimeout;
    
    // NOTE: bypassEndpoints moved to static local array in shouldBypassSecurity() 
    // to prevent member variable corruption causing crashes
};

#endif // SECURE_LAYER_MANAGER_H
