#include "method_tunneling_manager.h"
#include <esp_system.h>

// Список endpoints, которые должны использовать method tunneling
const char* MethodTunnelingManager::tunnelingEnabledEndpoints[] = {
    "/api/keys",           // TOTP codes - КРИТИЧНО
    "/api/passwords",      // All passwords list - КРИТИЧНО
    "/api/passwords/get",  // Password data - КРИТИЧНО  
    "/api/passwords/add",  // Add passwords - КРИТИЧНО
    "/api/passwords/delete", // Delete passwords - КРИТИЧНО
    "/api/passwords/update", // Password updates - КРИТИЧНО
    "/api/passwords/reorder", // Password reordering - КРИТИЧНО
    // "/api/upload_splash" removed - custom splash upload disabled for security
    "/api/keys/add",       // Add TOTP keys
    "/api/keys/delete",    // Delete TOTP keys
    "/api/pincode_settings", // PIN settings - КРИТИЧНО (содержит настройки безопасности)
    "/api/hidden_space",   // Hidden Space management - КРИТИЧНО (скрытое пространство)
    "/api/theme",          // Theme settings - КРИТИЧНО (тема устройства)
    "/api/display_settings", // Display timeout - КРИТИЧНО (таймаут экрана)
    "/api/splash/mode",    // Splash screen selection - КРИТИЧНО (выбор изображений)
    "/api/secure/test",    // Secure test endpoint
    nullptr
};

MethodTunnelingManager& MethodTunnelingManager::getInstance() {
    static MethodTunnelingManager instance;
    return instance;
}

MethodTunnelingManager::MethodTunnelingManager() 
    : initialized(false), totalTunneledRequests(0) {
}

MethodTunnelingManager::~MethodTunnelingManager() {
    end();
}

bool MethodTunnelingManager::begin() {
    if (initialized) return true;
    
    LOG_INFO("MethodTunneling", "Initializing Method Tunneling Manager...");
    
    // Инициализация счетчиков
    tunneledEndpoints.clear();
    totalTunneledRequests = 0;
    
    initialized = true;
    LOG_INFO("MethodTunneling", "Method Tunneling Manager initialized successfully");
    return true;
}

void MethodTunnelingManager::end() {
    if (!initialized) return;
    
    LOG_INFO("MethodTunneling", "Shutting down Method Tunneling Manager");
    tunneledEndpoints.clear();
    initialized = false;
}

bool MethodTunnelingManager::isTunneledRequest(AsyncWebServerRequest* request) {
    if (!initialized || !request) return false;
    
    // Проверяем, что это POST запрос с заголовком X-Real-Method
    if (request->method() == HTTP_POST && request->hasHeader("X-Real-Method")) {
        return true;
    }
    
    return false;
}

String MethodTunnelingManager::extractRealMethod(AsyncWebServerRequest* request) {
    if (!isTunneledRequest(request)) {
        // Возвращаем обычный метод, если это не туннелированный запрос
        switch (request->method()) {
            case HTTP_GET: return "GET";
            case HTTP_POST: return "POST";
            case HTTP_PUT: return "PUT";
            case HTTP_DELETE: return "DELETE";
            case HTTP_PATCH: return "PATCH";
            default: return "UNKNOWN";
        }
    }
    
    String encryptedMethod = request->getHeader("X-Real-Method")->value();
    String clientId = "";
    
    // Извлекаем Client ID для дешифровки (поддержка Header Obfuscation)
    if (request->hasHeader("X-Client-ID")) {
        clientId = request->getHeader("X-Client-ID")->value();
    } else if (request->hasHeader("X-Req-UUID")) {
        clientId = request->getHeader("X-Req-UUID")->value();
    }
    
    LOG_DEBUG("MethodTunneling", "🔓 Extracting real method from encrypted header...");
    String realMethod = decryptMethodHeader(encryptedMethod, clientId);
    
    if (realMethod.isEmpty()) {
        LOG_WARNING("MethodTunneling", "Failed to decrypt method header, using POST fallback");
        return "POST";
    }
    
    LOG_DEBUG("MethodTunneling", "🔓 Real method extracted: " + realMethod);
    totalTunneledRequests++;
    
    return realMethod;
}

String MethodTunnelingManager::decryptMethodHeader(const String& encryptedMethod, const String& clientId) {
    if (encryptedMethod.isEmpty()) return "";
    
    // 📉 Убран DEBUG лог - слишком часто вызывается
    
    // Use XOR encryption for method headers (simple and effective for short strings)
    String fallbackKey = generateMethodEncryptionKey(clientId);
    String decrypted = xorDecryptMethod(encryptedMethod, fallbackKey);
    
    if (isValidHttpMethod(decrypted)) {
        // 📉 Убран DEBUG лог - слишком часто вызывается
        return decrypted;
    }
    
    LOG_ERROR("MethodTunneling", "❌ Method decryption failed - invalid result: " + decrypted);
    return "";
}

String MethodTunnelingManager::encryptMethodHeader(const String& method, const String& clientId) {
    if (method.isEmpty() || !isValidHttpMethod(method)) {
        LOG_ERROR("MethodTunneling", "Invalid HTTP method for encryption: " + method);
        return "";
    }
    
    LOG_DEBUG("MethodTunneling", "🔐 Encrypting method: " + method);
    
    // Use XOR encryption for method headers (simple and effective for short strings)
    String fallbackKey = generateMethodEncryptionKey(clientId);
    String encrypted = xorEncryptMethod(method, fallbackKey);
    
    LOG_DEBUG("MethodTunneling", "✅ XOR method encryption successful");
    return encrypted;
}

String MethodTunnelingManager::generateMethodEncryptionKey(const String& clientId) {
    // Generate deterministic key based on clientId and static secret
    String baseKey = "MT_ESP32_" + clientId + "_METHOD_KEY";
    
    // Limit key length for consistency
    if (baseKey.length() > 32) {
        baseKey = baseKey.substring(0, 32);
    }
    
    return baseKey;
}

String MethodTunnelingManager::xorEncryptMethod(const String& method, const String& key) {
    String encrypted = "";
    
    for (size_t i = 0; i < method.length(); i++) {
        char encryptedChar = method[i] ^ key[i % key.length()];
        encrypted += String(encryptedChar, HEX);
    }
    
    return encrypted;
}

String MethodTunnelingManager::xorDecryptMethod(const String& encrypted, const String& key) {
    if (encrypted.length() % 2 != 0) return ""; // Невалидный hex
    
    String decrypted = "";
    
    for (size_t i = 0; i < encrypted.length(); i += 2) {
        String hexByte = encrypted.substring(i, i + 2);
        char byte = (char)strtol(hexByte.c_str(), nullptr, 16);
        char decryptedChar = byte ^ key[(i/2) % key.length()];
        decrypted += decryptedChar;
    }
    
    return decrypted;
}

bool MethodTunnelingManager::isValidHttpMethod(const String& method) {
    return (method == "GET" || method == "POST" || method == "PUT" || method == "DELETE" || method == "PATCH");
}

bool MethodTunnelingManager::shouldProcessTunneling(const String& endpoint) {
    if (!initialized) return false;
    
    // Проверяем, включен ли tunneling для данного endpoint
    for (int i = 0; tunnelingEnabledEndpoints[i] != nullptr; i++) {
        if (endpoint.startsWith(tunnelingEnabledEndpoints[i])) {
            return true;
        }
    }
    
    return false;
}

void MethodTunnelingManager::registerTunneledEndpoint(AsyncWebServer& server, const String& path,
                                                     ArRequestHandlerFunction onRequest) {
    if (!initialized) {
        LOG_ERROR("MethodTunneling", "Cannot register endpoint - manager not initialized");
        return;
    }
    
    LOG_INFO("MethodTunneling", "🔒 Registering tunneled endpoint: " + path);
    
    // Регистрируем только POST endpoint для туннелирования
    server.on(path.c_str(), HTTP_POST, [this, onRequest, path](AsyncWebServerRequest *request) {
        if (isTunneledRequest(request)) {
            LOG_DEBUG("MethodTunneling", "🔓 Processing tunneled request for: " + path);
            
            // Обновляем статистику
            tunneledEndpoints[path]++;
            
            // Извлекаем реальный метод 
            String realMethod = extractRealMethod(request);
            LOG_DEBUG("MethodTunneling", "🔓 Tunneled " + path + " with real method: " + realMethod);
            
            // SECURITY: Проверяем наличие критичных заголовков для tunneled запросов (Header Obfuscation support)
            String clientId = "";
            if (request->hasHeader("X-Client-ID")) {
                clientId = request->getHeader("X-Client-ID")->value();
            } else if (request->hasHeader("X-Req-UUID")) {
                clientId = request->getHeader("X-Req-UUID")->value();
            }
            
            if (clientId.length() > 0) {
                LOG_DEBUG("MethodTunneling", "🔐 Tunneled request with clientId: " + clientId.substring(0,8) + "...");
            } else {
                LOG_DEBUG("MethodTunneling", "🔐 Tunneled request missing clientId header (normal for some automated requests)");
            }
            
            // КРИТИЧНО: Для GET запросов через tunneling обеспечиваем параметры
            if (realMethod == "GET" && path.indexOf("/api/passwords/get") >= 0) {
                // Для /api/passwords/get проверяем наличие параметра index
                if (!request->hasParam("index", true) && !request->hasParam("index", false)) {
                    LOG_INFO("MethodTunneling", "🔧 Tunneled GET request missing index parameter - handler will use default");
                    // Полагаемся на логику handler'а для установки default значения
                }
            }
            
            // Вызываем оригинальный handler (ВСЕ заголовки автоматически передаются)
            onRequest(request);
        } else {
            // Обычный POST запрос без туннелирования
            LOG_DEBUG("MethodTunneling", "📝 Processing regular POST request for: " + path);
            onRequest(request);
        }
    });
    
    LOG_INFO("MethodTunneling", "✅ Tunneled endpoint registered: " + path);
}

void MethodTunnelingManager::registerTunneledEndpointWithBody(AsyncWebServer& server, const String& path,
                                                             ArRequestHandlerFunction onRequest,
                                                             ArBodyHandlerFunction onBody) {
    if (!initialized) {
        LOG_ERROR("MethodTunneling", "Cannot register endpoint with body - manager not initialized");
        return;
    }
    
    LOG_INFO("MethodTunneling", "🔒 Registering tunneled endpoint with body handler: " + path);
    
    // Регистрируем POST endpoint с body handler
    server.on(path.c_str(), HTTP_POST, [this, onRequest, path](AsyncWebServerRequest *request) {
        if (isTunneledRequest(request)) {
            LOG_DEBUG("MethodTunneling", "🔓 Processing tunneled request with body for: " + path);
            tunneledEndpoints[path]++;
            String realMethod = extractRealMethod(request);
            LOG_DEBUG("MethodTunneling", "🔓 Tunneled " + path + " with real method: " + realMethod);
        }
        onRequest(request);
    }, NULL, onBody);
    
    LOG_INFO("MethodTunneling", "✅ Tunneled endpoint with body registered: " + path);
}

int MethodTunnelingManager::getTunneledRequestCount() {
    return totalTunneledRequests;
}

void MethodTunnelingManager::clearStatistics() {
    tunneledEndpoints.clear();
    totalTunneledRequests = 0;
    LOG_INFO("MethodTunneling", "Statistics cleared");
}
