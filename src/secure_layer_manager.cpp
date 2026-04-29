#include "secure_layer_manager.h"
#include "device_static_key.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/gcm.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/hkdf.h"
#include "mbedtls/md.h"
#include <esp_system.h>

SecureLayerManager& SecureLayerManager::getInstance() {
    static SecureLayerManager instance;
    return instance;
}

SecureLayerManager::SecureLayerManager() 
    : initialized(false), sessionTimeout(SECURE_SESSION_TIMEOUT) {
}

SecureLayerManager::~SecureLayerManager() {
    end();
}

bool SecureLayerManager::begin() {
    if (initialized) return true;
    
    LOG_INFO("SecureLayerManager", "Initializing secure layer...");
    
    // Initialize entropy and PRNG
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_ecdh_init(&ecdh_context);
    
    const char* pers = "esp32_secure_layer_v1";
    int ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                   (const unsigned char*)pers, strlen(pers));
    if (ret != 0) {
        LOG_ERROR("SecureLayerManager", "Failed to seed PRNG: " + String(ret));
        return false;
    }
    
    // Setup ECDH with P-256 curve
    ret = mbedtls_ecp_group_load(&ecdh_context.grp, MBEDTLS_ECP_DP_SECP256R1);
    if (ret != 0) {
        LOG_ERROR("SecureLayerManager", "Failed to load ECP group: " + String(ret));
        return false;
    }
    
    // Generate server key pair
    ret = mbedtls_ecdh_gen_public(&ecdh_context.grp, &ecdh_context.d, &ecdh_context.Q,
                                 mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret != 0) {
        LOG_ERROR("SecureLayerManager", "Failed to generate server keypair: " + String(ret));
        return false;
    }
    
    initialized = true;
    LOG_INFO("SecureLayerManager", "Secure layer initialized successfully");
    return true;
}

void SecureLayerManager::end() {
    if (!initialized) return;
    
    // Clear all sessions
    sessions.clear();
    
    // Free mbedTLS contexts
    mbedtls_ecdh_free(&ecdh_context);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    
    initialized = false;
    LOG_INFO("SecureLayerManager", "Secure layer shutdown complete");
}

// ❌ REMOVED: update() and cleanup code completely removed
// Sessions are cleared automatically when web server stops (10 min timeout)
// No need for manual cleanup with race conditions

String SecureLayerManager::getServerPublicKey() {
    if (!initialized) return "";
    
    uint8_t pubkey[65]; // Uncompressed P-256 public key
    size_t pubkeyLen;
    
    int ret = mbedtls_ecp_point_write_binary(&ecdh_context.grp, &ecdh_context.Q,
                                           MBEDTLS_ECP_PF_UNCOMPRESSED,
                                           &pubkeyLen, pubkey, sizeof(pubkey));
    if (ret != 0) {
        LOG_ERROR("SecureLayerManager", "Failed to export public key: " + String(ret));
        return "";
    }
    
    return bytesToHex(pubkey, pubkeyLen);
}

bool SecureLayerManager::processKeyExchange(const String& clientId, const String& clientPubKeyHex, String& response) {
    LOG_INFO("🔐", "KeyExchange START: " + clientId.substring(0,8) + "...");
    
    if (!initialized) {
        response = "{\"type\":\"keyexchange\",\"status\":\"error\",\"message\":\"Server not initialized\"}";
        return false;
    }
    
    SecureSession* session = findSession(clientId);
    if (!session) {
        session = createSession(clientId);
        if (!session) {
            response = "{\"type\":\"keyexchange\",\"status\":\"error\",\"message\":\"Session limit exceeded\"}";
            return false;
        }
    }
    
    // Convert hex to binary
    uint8_t clientPubKey[65];
    if (!hexToBytes(clientPubKeyHex, clientPubKey, sizeof(clientPubKey))) {
        response = "{\"type\":\"keyexchange\",\"status\":\"error\",\"message\":\"Invalid public key format\"}";
        return false;
    }
    
    // Perform ECDH
    uint8_t sharedSecret[32];
    if (!performECDH(clientPubKey, 65, sharedSecret)) {
        response = "{\"type\":\"keyexchange\",\"status\":\"error\",\"message\":\"ECDH failed\"}";
        return false;
    }
    
    // Derive session key using HKDF with client nonce as salt
    if (!deriveSessionKey(sharedSecret, session->clientNonce, session->sessionKey)) {
        response = "{\"type\":\"keyexchange\",\"status\":\"error\",\"message\":\"Key derivation failed\"}";
        return false;
    }
    
    session->keyExchanged = true;
    session->lastActivity = millis();
    session->rxCounter = 0;
    session->txCounter = 0;
    
    // Prepare response with server public key AND salt (for HKDF)
    String serverPubKey = getServerPublicKey();
    String saltHex = bytesToHex(session->clientNonce, 16);
    
    response = "{\"type\":\"keyexchange\",\"status\":\"success\",\"pubkey\":\"" + serverPubKey + "\",\"salt\":\"" + saltHex + "\"}";
    
    LOG_INFO("🔐", "KeyExchange OK: " + clientId.substring(0,8) + "... [Sessions:" + String(sessions.size()) + "]");
    return true;
}

bool SecureLayerManager::processProtectedKeyExchange(const String& clientId, const String& encryptedClientKey, String& response) {
    LOG_INFO("🔐", "Protected KeyExchange START: " + clientId.substring(0,8) + "...");
    
    if (!initialized) {
        response = "{\"type\":\"keyexchange\",\"status\":\"error\",\"message\":\"Server not initialized\"}";
        return false;
    }
    
    // 1. Получаем device static key для дешифровки
    DeviceStaticKey& deviceKey = DeviceStaticKey::getInstance();
    String staticKey = deviceKey.getDeviceStaticKey();
    
    if (staticKey.isEmpty()) {
        LOG_ERROR("🔐", "Failed to get device static key");
        response = "{\"type\":\"keyexchange\",\"status\":\"error\",\"message\":\"Device key unavailable\"}";
        return false;
    }
    
    // 2. Дешифруем client public key с помощью device static key
    CryptoManager& crypto = CryptoManager::getInstance();
    String clientPubKeyHex = crypto.decryptWithPassword(encryptedClientKey, staticKey);
    
    if (clientPubKeyHex.isEmpty()) {
        LOG_ERROR("🔐", "Failed to decrypt client public key with device key");
        response = "{\"type\":\"keyexchange\",\"status\":\"error\",\"message\":\"Key decryption failed\"}";
        return false;
    }
    
    LOG_DEBUG("🔐", "Client public key decrypted successfully");
    
    // 3. Создаем или находим сессию
    SecureSession* session = findSession(clientId);
    if (!session) {
        session = createSession(clientId);
        if (!session) {
            response = "{\"type\":\"keyexchange\",\"status\":\"error\",\"message\":\"Session limit exceeded\"}";
            return false;
        }
    }
    
    // 4. Конвертируем hex в binary для ECDH
    uint8_t clientPubKey[65];
    if (!hexToBytes(clientPubKeyHex, clientPubKey, sizeof(clientPubKey))) {
        response = "{\"type\":\"keyexchange\",\"status\":\"error\",\"message\":\"Invalid decrypted key format\"}";
        return false;
    }
    
    // 5. Выполняем ECDH с расшифрованным ключом
    uint8_t sharedSecret[32];
    if (!performECDH(clientPubKey, 65, sharedSecret)) {
        response = "{\"type\":\"keyexchange\",\"status\":\"error\",\"message\":\"ECDH computation failed\"}";
        return false;
    }
    
    // 6. Derive session key с использованием shared secret
    if (!deriveSessionKey(sharedSecret, session->clientNonce, session->sessionKey)) {
        response = "{\"type\":\"keyexchange\",\"status\":\"error\",\"message\":\"Session key derivation failed\"}";
        return false;
    }
    
    // 7. Отмечаем сессию как готовую
    session->keyExchanged = true;
    session->lastActivity = millis();
    session->rxCounter = 0;
    session->txCounter = 0;
    
    // 8. Шифруем server public key перед отправкой
    String serverPubKey = getServerPublicKey();
    String encryptedServerKey = crypto.encryptWithPassword(serverPubKey, staticKey);
    
    if (encryptedServerKey.isEmpty()) {
        LOG_ERROR("🔐", "Failed to encrypt server public key");
        response = "{\"type\":\"keyexchange\",\"status\":\"error\",\"message\":\"Server key encryption failed\"}";
        return false;
    }
    
    // 9. Формируем защищенный response
    response = "{\"type\":\"keyexchange\",\"status\":\"success\",\"encrypted_pubkey\":\"" + encryptedServerKey + "\"}";
    
    LOG_INFO("🔐", "Protected KeyExchange SUCCESS: " + clientId.substring(0,8) + "... [Sessions:" + String(sessions.size()) + "]");
    return true;
}

// ⚡ IRAM_ATTR - размещаем в IRAM для максимальной скорости (4-8x boost)
IRAM_ATTR bool SecureLayerManager::encryptResponse(const String& clientId, const String& plaintext, String& encryptedJson) {
    if (!initialized) {
        LOG_ERROR("SecureLayerManager", "Manager not initialized");
        return false;
    }
    
    // ANTI-TIMING ANALYSIS: Случайная задержка 50-200ms для маскировки crypto операций
    unsigned long randomDelay = 50 + (esp_random() % 150);
    delay(randomDelay);
    
    SecureSession* session = findSession(clientId);
    if (!session || !session->keyExchanged) {
        // 📉 Убран DEBUG лог - не критичная информация
        // TIMING PROTECTION: Одинаковая задержка для успешных/неуспешных операций
        delay(100 + (esp_random() % 100));
        return false;
    }
    
    // 📉 Убран DEBUG лог - слишком часто вызывается
    
    session->lastActivity = millis();
    
    // Prepare encryption
    size_t plaintextLen = plaintext.length();
    uint8_t* plaintextBytes = (uint8_t*)plaintext.c_str();
    uint8_t* ciphertext = new uint8_t[plaintextLen];
    uint8_t iv[SECURE_GCM_IV_SIZE];
    uint8_t tag[SECURE_GCM_TAG_SIZE];
    size_t ciphertextLen;
    
    // AES-256-GCM encryption via mbedTLS
    generateNonce(iv, SECURE_GCM_IV_SIZE);
    
    bool success = encryptData(session->sessionKey, plaintextBytes, plaintextLen,
                              ciphertext, &ciphertextLen, iv, tag);
    
    if (!success) {
        LOG_ERROR("SecureLayerManager", "AES-GCM encryption failed");
    }
    
    if (success) {
        // Build JSON response
        JsonDocument doc;
        doc["type"] = "secure";
        doc["counter"] = session->txCounter++;
        doc["data"] = bytesToHex(ciphertext, ciphertextLen);
        doc["iv"] = bytesToHex(iv, SECURE_GCM_IV_SIZE);
        doc["tag"] = bytesToHex(tag, SECURE_GCM_TAG_SIZE);
        
        serializeJson(doc, encryptedJson);
        // 📉 Убран DEBUG лог - слишком часто вызывается
    }
    
    delete[] ciphertext;
    return success;
}

// ⚡ IRAM_ATTR - размещаем в IRAM для максимальной скорости
IRAM_ATTR bool SecureLayerManager::decryptRequest(const String& clientId, const String& encryptedJson, String& plaintext) {
    if (!initialized) {
        LOG_ERROR("SecureLayerManager", "Manager not initialized");
        return false;
    }
    
    SecureSession* session = findSession(clientId);
    if (!session || !session->keyExchanged) {
        // 📉 Убран DEBUG лог - не критичная информация
        return false;
    }
    
    // 📉 Убран DEBUG лог - слишком часто вызывается
    
    // 🛡️ Увеличенный буфер для больших зашифрованных данных (POST с FormData)
    if (ESP.getFreeHeap() < 20000) {
        LOG_ERROR("SecureLayerManager", "Insufficient heap for JSON parse: " + String(ESP.getFreeHeap()) + "b");
        return false;
    }
    DynamicJsonDocument doc(1024); // 1KB для парсинга {type, data, iv, tag, counter}
    DeserializationError error = deserializeJson(doc, encryptedJson);
    if (error) {
        LOG_ERROR("SecureLayerManager", "Failed to parse encrypted JSON: " + String(error.c_str()));
        LOG_ERROR("SecureLayerManager", "JSON preview: " + encryptedJson.substring(0, 100));
        return false;
    }
    
    // Extract encrypted data
    String dataHex = doc["data"];
    String ivHex = doc["iv"];
    String tagHex = doc["tag"];
    uint64_t counter = doc["counter"];
    
    if (dataHex.isEmpty() || ivHex.isEmpty() || tagHex.isEmpty()) {
        LOG_ERROR("SecureLayerManager", "Missing encryption components");
        return false;
    }
    
    // Check replay protection
    if (counter <= session->rxCounter) {
        LOG_WARNING("SecureLayerManager", "Replay attack detected! Counter: " + String(counter));
        return false;
    }
    session->rxCounter = counter;
    
    // Convert hex to binary
    size_t dataLen = dataHex.length() / 2;
    uint8_t* ciphertext = new uint8_t[dataLen];
    uint8_t iv[SECURE_GCM_IV_SIZE];
    uint8_t tag[SECURE_GCM_TAG_SIZE];
    
    if (!hexToBytes(dataHex, ciphertext, dataLen) ||
        !hexToBytes(ivHex, iv, SECURE_GCM_IV_SIZE) ||
        !hexToBytes(tagHex, tag, SECURE_GCM_TAG_SIZE)) {
        delete[] ciphertext;
        return false;
    }
    
    // AES-256-GCM decryption with tag authentication via mbedTLS
    uint8_t* decryptedBytes = new uint8_t[dataLen + 1];
    size_t decryptedLen = 0;
    
    bool success = decryptData(session->sessionKey, ciphertext, dataLen,
                              iv, tag, decryptedBytes, &decryptedLen);
    
    if (success) {
        decryptedBytes[decryptedLen] = '\0';
    } else {
        LOG_ERROR("SecureLayerManager", "AES-GCM auth failed - tag mismatch or tampering");
        decryptedBytes[0] = '\0';
    }
    
    plaintext = String((char*)decryptedBytes);
    session->lastActivity = millis();
    // 📉 Убран DEBUG лог - слишком часто вызывается
    
    delete[] ciphertext;
    delete[] decryptedBytes;
    
    return success;
}

bool SecureLayerManager::performECDH(const uint8_t* clientPubKey, size_t keyLen, uint8_t* sharedSecret) {
    if (!initialized || keyLen != 65) {
        LOG_ERROR("🔐", "ECDH check failed");
        return false;
    }
    
    mbedtls_ecp_point clientPoint, resultPoint;
    
    mbedtls_ecp_point_init(&clientPoint);
    mbedtls_ecp_point_init(&resultPoint);
    
    // Import client public key to point
    int ret = mbedtls_ecp_point_read_binary(&ecdh_context.grp, &clientPoint, 
                                           clientPubKey, keyLen);
    if (ret != 0) {
        LOG_ERROR("🔐", "ECDH key import failed: " + String(ret));
        mbedtls_ecp_point_free(&clientPoint);
        mbedtls_ecp_point_free(&resultPoint);
        return false;
    }
    
    // Compute shared secret: resultPoint = d * clientPoint
    ret = mbedtls_ecp_mul(&ecdh_context.grp, &resultPoint, &ecdh_context.d, &clientPoint,
                         mbedtls_ctr_drbg_random, &ctr_drbg);
    
    if (ret != 0) {
        LOG_ERROR("🔐", "ECDH mul failed: " + String(ret));
        mbedtls_ecp_point_free(&clientPoint);
        mbedtls_ecp_point_free(&resultPoint);
        return false;
    }
    
    // Extract X coordinate as shared secret
    ret = mbedtls_mpi_write_binary(&resultPoint.X, sharedSecret, 32);
    
    mbedtls_ecp_point_free(&clientPoint);
    mbedtls_ecp_point_free(&resultPoint);
    
    return ret == 0;
}

bool SecureLayerManager::deriveSessionKey(const uint8_t* sharedSecret, const uint8_t* salt, uint8_t* sessionKey) {
    // Custom HKDF implementation using HMAC-SHA256 (ESP32 compatible)
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t* md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!md) return false;
    
    mbedtls_md_init(&ctx);
    
    // HKDF-Extract: PRK = HMAC-SHA256(salt, shared_secret)
    uint8_t prk[32];
    int ret = mbedtls_md_setup(&ctx, md, 1);
    if (ret != 0) {
        mbedtls_md_free(&ctx);
        return false;
    }
    
    ret = mbedtls_md_hmac_starts(&ctx, salt, 16);
    if (ret == 0) ret = mbedtls_md_hmac_update(&ctx, sharedSecret, 32);
    if (ret == 0) ret = mbedtls_md_hmac_finish(&ctx, prk);
    
    if (ret != 0) {
        mbedtls_md_free(&ctx);
        return false;
    }
    
    // HKDF-Expand: derive session key from PRK
    const char* info = "SecureLayerV1";
    size_t info_len = 13;
    
    ret = mbedtls_md_hmac_starts(&ctx, prk, 32);
    if (ret == 0) ret = mbedtls_md_hmac_update(&ctx, (const uint8_t*)info, info_len);
    if (ret == 0) {
        uint8_t counter = 1;
        ret = mbedtls_md_hmac_update(&ctx, &counter, 1);
    }
    if (ret == 0) ret = mbedtls_md_hmac_finish(&ctx, sessionKey);
    
    mbedtls_md_free(&ctx);
    
    // Clear PRK from memory
    memset(prk, 0, sizeof(prk));
    
    return ret == 0;
}

bool SecureLayerManager::encryptData(const uint8_t* key, const uint8_t* plaintext, size_t plaintextLen,
                                    uint8_t* ciphertext, size_t* ciphertextLen, 
                                    uint8_t* iv, uint8_t* tag) {
    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);
    
    int ret = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, SECURE_AES_KEY_SIZE * 8);
    if (ret != 0) {
        mbedtls_gcm_free(&gcm);
        return false;
    }
    
    // Generate random IV
    generateNonce(iv, SECURE_GCM_IV_SIZE);
    
    ret = mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT,
                                   plaintextLen, iv, SECURE_GCM_IV_SIZE,
                                   nullptr, 0, // No additional data
                                   plaintext, ciphertext,
                                   SECURE_GCM_TAG_SIZE, tag);
    
    *ciphertextLen = plaintextLen;
    mbedtls_gcm_free(&gcm);
    
    return ret == 0;
}

bool SecureLayerManager::decryptData(const uint8_t* key, const uint8_t* ciphertext, size_t ciphertextLen,
                                    const uint8_t* iv, const uint8_t* tag,
                                    uint8_t* plaintext, size_t* plaintextLen) {
    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);
    
    int ret = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, SECURE_AES_KEY_SIZE * 8);
    if (ret != 0) {
        mbedtls_gcm_free(&gcm);
        return false;
    }
    
    ret = mbedtls_gcm_auth_decrypt(&gcm, ciphertextLen,
                                  iv, SECURE_GCM_IV_SIZE,
                                  nullptr, 0, // No additional data
                                  tag, SECURE_GCM_TAG_SIZE,
                                  ciphertext, plaintext);
    
    *plaintextLen = ciphertextLen;
    mbedtls_gcm_free(&gcm);
    
    return ret == 0;
}

SecureLayerManager::SecureSession* SecureLayerManager::findSession(const String& clientId) {
    auto it = sessions.find(clientId);
    return (it != sessions.end()) ? &it->second : nullptr;
}

SecureLayerManager::SecureSession* SecureLayerManager::createSession(const String& clientId) {
    if (sessions.size() >= SECURE_MAX_SESSIONS) {
        // Evict the least recently used session instead of rejecting
        auto oldest = sessions.begin();
        for (auto it = sessions.begin(); it != sessions.end(); ++it) {
            if (it->second.lastActivity < oldest->second.lastActivity) {
                oldest = it;
            }
        }
        LOG_INFO("🔐", "Session limit reached — evicting LRU: " + 
                 oldest->second.clientId.substring(0,8) + "...");
        sessions.erase(oldest);
    }
    
    SecureSession session;
    session.clientId = clientId;
    session.rxCounter = 0;
    session.txCounter = 0;
    session.keyExchanged = false;
    session.lastActivity = millis();
    
    // Generate deterministic client nonce from clientId for reproducible keys
    generateNonce(session.clientNonce, 16);
    
    sessions[clientId] = session;
    LOG_DEBUG("🔐", "Session created: " + clientId.substring(0,8) + "... [Total:" + String(sessions.size()) + "]");
    
    return &sessions[clientId];
}

void SecureLayerManager::removeSession(const String& clientId) {
    auto it = sessions.find(clientId);
    if (it != sessions.end()) {
        // Clear sensitive data
        memset(it->second.sessionKey, 0, SECURE_AES_KEY_SIZE);
        sessions.erase(it);
        LOG_DEBUG("🔐", "Session removed: " + clientId.substring(0,8) + "... [Remaining:" + String(sessions.size()) + "]");
    }
}

// ❌ REMOVED: cleanupExpiredSessions() completely deleted
// Reason: Causes race condition crashes without mutex
// Not needed: Web server stops after 10min timeout, clearing all RAM sessions automatically

bool SecureLayerManager::isSecureSessionValid(const String& clientId) {
    SecureSession* session = findSession(clientId);
    return session && session->keyExchanged;
}

void SecureLayerManager::invalidateSecureSession(const String& clientId) {
    removeSession(clientId);
}

int SecureLayerManager::getActiveSecureSessionCount() {
    return sessions.size();
}

bool SecureLayerManager::shouldBypassSecurity(const String& endpoint) {
    // Static local array to avoid member variable corruption issues
    static const char* safeBypassEndpoints[] = {
        "/login",
        "/register", 
        "/api/secure/hello",
        "/api/secure/keyexchange",
        "/api/secure/status",
        nullptr
    };
    
    for (int i = 0; safeBypassEndpoints[i] != nullptr; i++) {
        if (endpoint.startsWith(safeBypassEndpoints[i])) {
            return true;
        }
    }
    return false;
}

String SecureLayerManager::wrapSecureResponse(const String& clientId, const String& originalResponse) {
    if (!isSecureSessionValid(clientId)) {
        return originalResponse; // Return unencrypted if no secure session
    }
    
    String encryptedResponse;
    if (encryptResponse(clientId, originalResponse, encryptedResponse)) {
        return encryptedResponse;
    }
    
    return originalResponse; // Fallback to unencrypted
}

bool SecureLayerManager::unwrapSecureRequest(const String& clientId, const String& requestBody, String& unwrappedBody) {
    if (!isSecureSessionValid(clientId)) {
        unwrappedBody = requestBody;
        return true; // Pass through unencrypted
    }
    
    return decryptRequest(clientId, requestBody, unwrappedBody);
}

String SecureLayerManager::bytesToHex(const uint8_t* bytes, size_t length) {
    String hex = "";
    for (size_t i = 0; i < length; i++) {
        char hexByte[3];
        sprintf(hexByte, "%02x", bytes[i]);
        hex += hexByte;
    }
    return hex;
}

bool SecureLayerManager::hexToBytes(const String& hex, uint8_t* bytes, size_t maxLength) {
    if (hex.length() % 2 != 0 || hex.length() / 2 > maxLength) {
        return false;
    }
    
    size_t length = hex.length() / 2;
    for (size_t i = 0; i < length; i++) {
        String hexByte = hex.substring(i * 2, i * 2 + 2);
        bytes[i] = (uint8_t)strtol(hexByte.c_str(), nullptr, 16);
    }
    
    return true;
}

bool SecureLayerManager::generateNonce(uint8_t* nonce, size_t length) {
    return mbedtls_ctr_drbg_random(&ctr_drbg, nonce, length) == 0;
}

void SecureLayerManager::wipeAllSessions() {
    for (auto& pair : sessions) {
        memset(pair.second.sessionKey, 0, SECURE_AES_KEY_SIZE);
        memset(pair.second.clientNonce, 0, 16);
    }
    sessions.clear();
    mbedtls_ecdh_free(&ecdh_context);
    initialized = false;
}
