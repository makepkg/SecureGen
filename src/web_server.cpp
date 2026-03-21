#include "web_server.h"
#include <ArduinoJson.h>
#include "config.h"
#include <FS.h>
#include "LittleFS.h"
#include "WiFi.h"
#include "totp_generator.h"
#include "crypto_manager.h"
#include "web_admin_manager.h" 
#include "web_pages/page_login.h"
#include <nvs_flash.h>  // Для очистки NVS partition при factory reset
#ifdef DEBUG_BUILD
#include "web_pages/page_test_encryption.h"
#endif
#include "web_pages/page_index.h"
#include "web_pages/page_register.h"
#include "web_pages/page_wifi_setup.h"
#include "web_pages/page_splash.h"
#include "ble_keyboard_manager.h"
#include "rtc_manager.h"
extern RTCManager rtcManager;

#ifdef SECURE_LAYER_ENABLED
#include "web_server_secure_integration.h"
#include "method_tunneling_manager.h"

// Helper function: URL decode
String urlDecode(const String& str) {
    String decoded = "";
    char temp[] = "0x00";
    unsigned int len = str.length();
    for (unsigned int i = 0; i < len; i++) {
        char c = str.charAt(i);
        if (c == '+') {
            decoded += ' ';
        } else if (c == '%' && i + 2 < len) {
            temp[2] = str.charAt(i + 1);
            temp[3] = str.charAt(i + 2);
            decoded += (char)strtol(temp, NULL, 16);
            i += 2;
        } else {
            decoded += c;
        }
    }
    return decoded;
}
#include "traffic_obfuscation_manager.h"
#include "header_obfuscation_manager.h"
#include "header_obfuscation_integration.h"
#endif

// WebServerManager Implementation

WebServerManager::WebServerManager(KeyManager& keyManager, SplashScreenManager& splashManager, DisplayManager& displayManager, PinManager& pinManager, ConfigManager& configManager, PasswordManager& passwordManager, TOTPGenerator& totpGenerator)
    : server(WEB_SERVER_PORT), keyManager(keyManager), splashManager(splashManager), displayManager(displayManager), pinManager(pinManager), configManager(configManager), passwordManager(passwordManager), totpGenerator(totpGenerator), _isRunning(false), _oneMinuteWarningShown(false) {}

void WebServerManager::setBleKeyboardManager(BleKeyboardManager* bleManager) {
    bleKeyboardManager = bleManager;
}

void WebServerManager::setWifiManager(WifiManager* manager) {
    wifiManager = manager;
}

void WebServerManager::setBatteryManager(BatteryManager* manager) {
    batteryManager = manager;
}

bool WebServerManager::isAuthenticated(AsyncWebServerRequest *request) {
    // НЕ сбрасываем таймер здесь - это вызывается для каждого запроса!
    if (session_id.length() > 0 && (millis() - session_created_time > SESSION_TIMEOUT)) {
        LOG_INFO("WebServer", "Session timeout reached - clearing session");
        clearSession();
    }
    
    if (request->hasHeader("Cookie")) {
        String cookie = request->getHeader("Cookie")->value();
        
        // CRITICAL FIX: Check if session_id is empty first!
        if (session_id.length() == 0) {
            return false;
        }
        
        String expectedCookie = "session=" + session_id;
        if (cookie.indexOf(expectedCookie) != -1) {
            return true;
        }
    }
    
    if (request->hasHeader("Authorization")) {
        String auth = request->getHeader("Authorization")->value();
        if (auth.startsWith("Bearer ") && auth.substring(7).equals(session_id)) {
            return true;
        }
    }
    return false;
}

bool WebServerManager::verifyCsrfToken(AsyncWebServerRequest *request) {
    if (session_csrf_token.isEmpty()) {
        LOG_WARNING("WebServer", "CSRF verification failed: no session token");
        return false;
    }
    
    // Проверяем заголовок X-CSRF-Token (приоритет)
    if (request->hasHeader("X-CSRF-Token")) {
        String token = request->getHeader("X-CSRF-Token")->value();
        return token.equals(session_csrf_token);
    }
    
    // Проверяем POST параметр csrf_token (fallback)
    if (request->hasParam("csrf_token", true)) {
        String token = request->getParam("csrf_token", true)->value();
        return token.equals(session_csrf_token);
    }
    
    LOG_WARNING("WebServer", "CSRF verification failed: no token provided");
    return false;
}

void WebServerManager::clearSession() {
    if (!session_id.isEmpty() || !session_csrf_token.isEmpty()) {
        LOG_INFO("WebServer", "Clearing active session and CSRF token");
    }
    
    // Clear protected secure session first
    clearSecureSession();
    
    // Clear from persistent storage
    CryptoManager::getInstance().clearSession();
    
    // Clear from memory with secure cleanup
    session_id.clear();
    session_csrf_token.clear();
    session_created_time = 0;
    
    // Re-initialize as empty strings
    session_id = "";
    session_csrf_token = "";
}

void WebServerManager::loadPersistentSession() {
    String persistentSessionId, persistentCsrfToken;
    unsigned long persistentCreatedTime;
    
    if (CryptoManager::getInstance().loadSession(persistentSessionId, persistentCsrfToken, persistentCreatedTime)) {
        // Session loaded successfully and is valid (CryptoManager уже проверил)
        session_id = persistentSessionId;
        session_csrf_token = persistentCsrfToken;
        session_created_time = millis(); // ИСПРАВЛЕНО: обновляем время для текущего запуска
        
        LOG_INFO("WebServer", "Restored valid persistent session from storage");
    } else {
    }
}

void WebServerManager::savePersistentSession() {
    if (session_id.isEmpty() || session_csrf_token.isEmpty()) {
        return;
    }
    
    if (CryptoManager::getInstance().saveSession(session_id, session_csrf_token, session_created_time)) {
    } else {
        LOG_ERROR("WebServer", "Failed to save session to persistent storage");
    }
}

void WebServerManager::regenerateCsrfTokenIfNeeded() {
    // ИСПРАВЛЕНО: CryptoManager единственный источник правды для валидации сессий
    // WebServerManager не должен проверять срок действия - это делает CryptoManager при loadSession()
    
    if (session_csrf_token.isEmpty()) {
        return; // No active session
    }
    
    // Никаких проверок срока действия - CryptoManager уже проверил при загрузке
    // Сессия валидна до тех пор пока не будет очищена извне или при следующем reboot
}

void WebServerManager::update() {
    if (!_isRunning) return;
    
    static unsigned long lastTimeoutCheck = 0;
    static unsigned long lastPersistentCleanup = 0;
    const unsigned long TIMEOUT_CHECK_INTERVAL = 20000; // Check every 20 seconds
    const unsigned long PERSISTENT_CLEANUP_INTERVAL = 300000; // Check persistent session every 5 minutes
    
    unsigned long currentTime = millis();
    
#ifdef SECURE_LAYER_ENABLED
    // ❌ DISABLED: RAM session cleanup causes race condition - crashes with StoreProhibited
    // secureLayer.update();
#endif
    
    // ✅ Cleanup persistent session from LittleFS if expired (safe, no race condition)
    if (currentTime - lastPersistentCleanup >= PERSISTENT_CLEANUP_INTERVAL) {
        lastPersistentCleanup = currentTime;
        
        // Проверяем persistent сессию и удаляем если истекла
        String dummyId, dummyCsrf;
        unsigned long dummyTime;
        if (!CryptoManager::getInstance().loadSession(dummyId, dummyCsrf, dummyTime)) {
            // loadSession уже удалил файл если сессия истекла
        }
    }
    
    if (currentTime - lastTimeoutCheck >= TIMEOUT_CHECK_INTERVAL) {
        lastTimeoutCheck = currentTime;
        
        // Проверяем валидность сессии (без ротации CSRF)
        regenerateCsrfTokenIfNeeded();
        
        // ⚠️ TIMEOUT = 0 → БЕСКОНЕЧНО (без автоотключения)
        if (_timeoutMinutes == 0) {
            return; // Не проверяем timeout
        }
        
        unsigned long elapsedTime = (currentTime - _lastActivityTimestamp) / 1000;
        unsigned long timeoutSeconds = _timeoutMinutes * 60;
        
        if (elapsedTime >= timeoutSeconds) {
            LOG_INFO("WebServer", "Web server timeout reached. Shutting down.");
            stop();
            return;
        }
        
        // Show warning at 1-minute mark if timeout is approaching
        unsigned long remainingTime = timeoutSeconds - elapsedTime;
        if (remainingTime <= 60 && !_oneMinuteWarningShown) {
            LOG_INFO("WebServer", "Web server will timeout in " + String(remainingTime) + " seconds");
            _oneMinuteWarningShown = true;
        }
        
        // Убрали частый INFO лог - оставляем только предупреждение выше
    }
}

void WebServerManager::start() {
    // 🛡️ КРИТИЧНО: Проверка памяти перед стартом
    uint32_t freeHeapBefore = ESP.getFreeHeap();
    uint32_t minFreeHeap = ESP.getMinFreeHeap();
    LOG_INFO("WebServer", "📡 Memory before start: Free=" + String(freeHeapBefore) + "b, Min=" + String(minFreeHeap) + "b");
    
    if (freeHeapBefore < 40000) {
        LOG_CRITICAL("WebServer", "❌ CRITICAL: Not enough memory to start web server! Free=" + String(freeHeapBefore));
        return;
    }
    
    _timeoutMinutes = configManager.getWebServerTimeout();
    LOG_INFO("WebServer", "Starting web server with timeout: " + String(_timeoutMinutes) + " minutes");
    resetActivityTimer();
    
    // КРИТИЧНО: Попытаться загрузить персистентную сессию при старте
    loadPersistentSession();

#ifdef SECURE_LAYER_ENABLED
    // Инициализация SecureLayerManager для HTTPS-like шифрования
    if (secureLayer.begin()) {
        LOG_INFO("WebServer", "🔐 SecureLayerManager initialized successfully");
        // Добавляем secure endpoints для key exchange и тестирования
        WebServerSecureIntegration::addSecureEndpoints(server, secureLayer, urlObfuscation);
        LOG_INFO("WebServer", "🔐 Secure endpoints added for encrypted testing");
    } else {
        LOG_ERROR("WebServer", "🔐 Failed to initialize SecureLayerManager");
    }
    
    // ✅ Инициализация TrafficObfuscation ТОЛЬКО при запуске веб-сервера!
    if (TrafficObfuscationManager::getInstance().begin()) {
        LOG_INFO("WebServer", "🎭 Traffic Obfuscation initialized successfully");
    } else {
        LOG_ERROR("WebServer", "🎭 Failed to initialize Traffic Obfuscation");
    }
#endif

    // Инициализация URL Obfuscation Manager
    if (urlObfuscation.begin()) {
        LOG_INFO("WebServer", "🔗 URLObfuscationManager initialized successfully");
        // Добавляем API endpoints для обфускации
        URLObfuscationIntegration::addObfuscationAPIEndpoints(server, urlObfuscation);
        LOG_INFO("WebServer", "🔗 URL obfuscation API endpoints added");
    } else {
        LOG_ERROR("WebServer", "🔗 Failed to initialize URLObfuscationManager");
    }

#ifdef SECURE_LAYER_ENABLED
    // Инициализация Header Obfuscation Manager
    if (HeaderObfuscationManager::getInstance().begin()) {
        LOG_INFO("WebServer", "🎭 HeaderObfuscationManager initialized successfully");
    } else {
        LOG_ERROR("WebServer", "🎭 Failed to initialize HeaderObfuscationManager");
    }
#endif

    // Middleware для сброса таймера активности при любом запросе
    server.onNotFound([this](AsyncWebServerRequest *request) {
        // НЕ сбрасываем таймер автоматически для всех запросов
        if (!WebAdminManager::getInstance().isRegistered()) {
            if (request->url() != "/register") {
                return request->redirect("/register");
            }
        } else if (!isAuthenticated(request)) {
            if (request->url() != "/login") {
                return request->redirect("/login");
            }
        }
        request->send(404, "text/plain", "Not found");
    });

    // --- Регистрация ---
    server.on("/register", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (WebAdminManager::getInstance().isRegistered()) {
            return request->redirect("/login");
        }
        request->send_P(200, "text/html", page_register);
    });

    server.on("/register", HTTP_POST, [this](AsyncWebServerRequest *request){
        if (WebAdminManager::getInstance().isRegistered()) {
            return request->redirect("/login");
        }
        
        // 🔐 Проверяем, зашифровано ли тело
        String username, password;
        bool isEncrypted = request->hasHeader("X-Encrypted-Body") && request->getHeader("X-Encrypted-Body")->value() == "true";
        
        if (isEncrypted) {
            // Тело уже обработано в bodyHandler
            if (request->_tempObject) {
                auto* registerData = (JsonDocument*)request->_tempObject;
                if (!(*registerData)["username"].isNull() && !(*registerData)["password"].isNull()) {
                    username = (*registerData)["username"].as<String>();
                    password = (*registerData)["password"].as<String>();
                    LOG_INFO("WebServer", "🔐 Register with encrypted body for user: [HIDDEN]");
                } else {
                    LOG_ERROR("WebServer", "🔐 Register: decrypted body missing fields");
                    request->send(400, "text/plain", "Invalid encrypted body");
                    return;
                }
            } else {
                LOG_ERROR("WebServer", "🔐 Register: encrypted body not processed");
                request->send(400, "text/plain", "Encrypted body processing failed");
                return;
            }
        } else if (request->hasParam("username", true) && request->hasParam("password", true)) {
            // Обычный FormData
            username = request->getParam("username", true)->value();
            password = request->getParam("password", true)->value();
            LOG_INFO("WebServer", "Register with plain body for user: [HIDDEN]");
        } else {
            LOG_ERROR("WebServer", "Register: no credentials provided");
            request->redirect("/register?error=1");
            return;
        }
        
        if (username.length() > 0 && password.length() >= 4) {
            WebAdminManager::getInstance().registerAdmin(username, password);
            request->redirect("/login");
            return;
        }
        request->redirect("/register?error=1");
    }, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        // 🔐 Body handler для расшифровки XOR зашифрованного тела
        if (index + len == total) {
            bool isEncrypted = request->hasHeader("X-Encrypted-Body") && request->getHeader("X-Encrypted-Body")->value() == "true";
            
            if (isEncrypted && request->hasHeader("X-Client-ID")) {
                String clientId = request->getHeader("X-Client-ID")->value();
                String body = String((char*)data, len);
                
                
                // Парсим JSON с зашифрованными данными
                JsonDocument doc;
                if (deserializeJson(doc, body) != DeserializationError::Ok || doc["encrypted"].isNull()) {
                    LOG_ERROR("WebServer", "🔐 Register: Failed to parse encrypted body JSON");
                    return;
                }
                
                String encryptedHex = doc["encrypted"].as<String>();
                
                // Генерируем тот же XOR ключ
                String encryptionKey = "MT_ESP32_" + clientId + "_METHOD_KEY";
                if (encryptionKey.length() > 32) encryptionKey = encryptionKey.substring(0, 32);
                
                // Расшифровываем HEX строку
                String decrypted = "";
                for (size_t i = 0; i < encryptedHex.length(); i += 2) {
                    String hexByte = encryptedHex.substring(i, i + 2);
                    char encryptedChar = (char)strtol(hexByte.c_str(), NULL, 16);
                    char decryptedChar = encryptedChar ^ encryptionKey[i / 2 % encryptionKey.length()];
                    decrypted += decryptedChar;
                }
                
                
                // Парсим расшифрованный JSON
                JsonDocument* registerData = new JsonDocument();
                if (deserializeJson(*registerData, decrypted) == DeserializationError::Ok) {
                    request->_tempObject = registerData;
                    LOG_INFO("WebServer", "🔐 Register: Body decrypted successfully");
                } else {
                    LOG_ERROR("WebServer", "🔐 Register: Failed to parse decrypted JSON");
                    delete registerData;
                }
            }
        }
    });

    // --- Логин ---
    server.on("/login", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (WebAdminManager::getInstance().isRegistered() && isAuthenticated(request)) {
            return request->redirect("/");
        }
        if (!WebAdminManager::getInstance().isRegistered()) {
            return request->redirect("/register");
        }
        
        // Auto-initialize secure session for login page
        if (shouldInitializeSecureSession(request)) {
            initializeSecureSession(request);
            
            String html = String(login_html);
            injectSecureInitScript(request, html);
            request->send(200, "text/html", html);
        } else {
            request->send_P(200, "text/html", login_html);
        }
    });

    server.on("/login", HTTP_POST, [this](AsyncWebServerRequest *request){
        auto& adminManager = WebAdminManager::getInstance();

        // Шаг 1: Проверяем, не активна ли уже блокировка
        unsigned long lockoutTime = adminManager.getLockoutTimeRemaining();
        if (lockoutTime > 0) {
            String url = "/login?error=2&time=" + String(lockoutTime);
            AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Found");
            response->addHeader("Location", url);
            request->send(response);
            return;
        }

        // 🔐 Проверяем, зашифровано ли тело
        String username, password;
        bool isEncrypted = request->hasHeader("X-Encrypted-Body") && request->getHeader("X-Encrypted-Body")->value() == "true";
        
        if (isEncrypted) {
            // Тело уже обработано в bodyHandler, данные в request->_tempObject
            if (request->_tempObject) {
                auto* loginData = (JsonDocument*)request->_tempObject;
                if (!(*loginData)["username"].isNull() && !(*loginData)["password"].isNull()) {
                    username = (*loginData)["username"].as<String>();
                    password = (*loginData)["password"].as<String>();
                    LOG_INFO("WebServer", "🔐 Login with encrypted body for user: [HIDDEN]");
                } else {
                    LOG_ERROR("WebServer", "🔐 Login: decrypted body missing fields");
                    request->send(400, "text/plain", "Invalid encrypted body");
                    return;
                }
            } else {
                LOG_ERROR("WebServer", "🔐 Login: encrypted body not processed");
                request->send(400, "text/plain", "Encrypted body processing failed");
                return;
            }
        } else if (request->hasParam("username", true) && request->hasParam("password", true)) {
            // Обычный FormData (не зашифрован)
            username = request->getParam("username", true)->value();
            password = request->getParam("password", true)->value();
            LOG_INFO("WebServer", "Login with plain body for user: [HIDDEN]");
        } else {
            LOG_ERROR("WebServer", "Login: no credentials provided");
            request->send(400, "text/plain", "No credentials provided");
            return;
        }

        // Шаг 2: Проверяем учетные данные
        if (username.length() > 0 && password.length() > 0) {
            
            if (adminManager.verifyCredentials(username, password)) {
                // Успех: сбрасываем счетчики и создаем сессию
                adminManager.resetLoginAttempts();
                session_id = CryptoManager::getInstance().generateSecureSessionId();
                session_csrf_token = CryptoManager::getInstance().generateCsrfToken();
                session_created_time = millis();
                
                // КРИТИЧНО: Сохраняем новую сессию в зашифрованное хранилище
                savePersistentSession();
                
                AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Found");
                response->addHeader("Location", "/");
                response->addHeader("Set-Cookie", "session=" + session_id + "; Path=/; HttpOnly; SameSite=Strict");
                // Security headers
                response->addHeader("X-Content-Type-Options", "nosniff");
                response->addHeader("X-Frame-Options", "DENY");
                response->addHeader("X-XSS-Protection", "1; mode=block");
                request->send(response);
                return;
            } else {
                // Неудача: регистрируем попытку и проверяем, не активировалась ли блокировка
                adminManager.handleFailedLoginAttempt();
                lockoutTime = adminManager.getLockoutTimeRemaining();
                if (lockoutTime > 0) {
                    // Блокировка только что активировалась
                    String url = "/login?error=2&time=" + String(lockoutTime);
                    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Found");
                    response->addHeader("Location", url);
                    request->send(response);
                    return;
                }
            }
        }

        // Если дошли сюда, значит была обычная неудачная попытка без блокировки
        AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Found");
        response->addHeader("Location", "/login?error=1");
        request->send(response);
    }, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        // 🔐 Body handler для расшифровки XOR зашифрованного тела
        if (index + len == total) {
            bool isEncrypted = request->hasHeader("X-Encrypted-Body") && request->getHeader("X-Encrypted-Body")->value() == "true";
            
            if (isEncrypted && request->hasHeader("X-Client-ID")) {
                String clientId = request->getHeader("X-Client-ID")->value();
                String body = String((char*)data, len);
                
                
                // Парсим JSON с зашифрованными данными
                JsonDocument doc;
                if (deserializeJson(doc, body) != DeserializationError::Ok || doc["encrypted"].isNull()) {
                    LOG_ERROR("WebServer", "🔐 Login: Failed to parse encrypted body JSON");
                    return;
                }
                
                String encryptedHex = doc["encrypted"].as<String>();
                
                // Генерируем тот же XOR ключ
                String encryptionKey = "MT_ESP32_" + clientId + "_METHOD_KEY";
                if (encryptionKey.length() > 32) encryptionKey = encryptionKey.substring(0, 32);
                
                // Расшифровываем HEX строку
                String decrypted = "";
                for (size_t i = 0; i < encryptedHex.length(); i += 2) {
                    String hexByte = encryptedHex.substring(i, i + 2);
                    char encryptedChar = (char)strtol(hexByte.c_str(), NULL, 16);
                    char decryptedChar = encryptedChar ^ encryptionKey[i / 2 % encryptionKey.length()];
                    decrypted += decryptedChar;
                }
                
                
                // Парсим расшифрованный JSON
                JsonDocument* loginData = new JsonDocument();
                if (deserializeJson(*loginData, decrypted) == DeserializationError::Ok) {
                    request->_tempObject = loginData;
                    LOG_INFO("WebServer", "🔐 Login: Body decrypted successfully");
                } else {
                    LOG_ERROR("WebServer", "🔐 Login: Failed to parse decrypted JSON");
                    delete loginData;
                }
            }
        }
    });

    // 🔗 Обфусцированный алиас для /login (GET)
    String obfuscatedLoginPath = urlObfuscation.obfuscateURL("/login");
    if (obfuscatedLoginPath.length() > 0 && obfuscatedLoginPath != "/login") {
        LOG_INFO("WebServer", "🔗 Registering obfuscated login GET: " + obfuscatedLoginPath);
        
        server.on(obfuscatedLoginPath.c_str(), HTTP_GET, [this](AsyncWebServerRequest *request){
            if (WebAdminManager::getInstance().isRegistered() && isAuthenticated(request)) {
                return request->redirect("/");
            }
            if (!WebAdminManager::getInstance().isRegistered()) {
                return request->redirect("/register");
            }
            
            // Auto-initialize secure session for login page
            if (shouldInitializeSecureSession(request)) {
                initializeSecureSession(request);
                
                String html = String(login_html);
                injectSecureInitScript(request, html);
                request->send(200, "text/html", html);
            } else {
                request->send_P(200, "text/html", login_html);
            }
        });
        
        // 🔗 Обфусцированный алиас для /login (POST)
        LOG_INFO("WebServer", "🔗 Registering obfuscated login POST: " + obfuscatedLoginPath);
        
        server.on(obfuscatedLoginPath.c_str(), HTTP_POST, [this](AsyncWebServerRequest *request){
            auto& adminManager = WebAdminManager::getInstance();

            // Шаг 1: Проверяем, не активна ли уже блокировка
            unsigned long lockoutTime = adminManager.getLockoutTimeRemaining();
            if (lockoutTime > 0) {
                String url = "/login?error=2&time=" + String(lockoutTime);
                AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Found");
                response->addHeader("Location", url);
                request->send(response);
                return;
            }

            // 🔐 Проверяем, зашифровано ли тело
            String username, password;
            bool isEncrypted = request->hasHeader("X-Encrypted-Body") && request->getHeader("X-Encrypted-Body")->value() == "true";
            
            if (isEncrypted) {
                if (request->_tempObject) {
                    auto* loginData = (JsonDocument*)request->_tempObject;
                    if (!(*loginData)["username"].isNull() && !(*loginData)["password"].isNull()) {
                        username = (*loginData)["username"].as<String>();
                        password = (*loginData)["password"].as<String>();
                        LOG_INFO("WebServer", "🔐 Login (obfuscated) with encrypted body for user: [HIDDEN]");
                    } else {
                        LOG_ERROR("WebServer", "🔐 Login (obfuscated): decrypted body missing fields");
                        request->send(400, "text/plain", "Invalid encrypted body");
                        return;
                    }
                } else {
                    LOG_ERROR("WebServer", "🔐 Login (obfuscated): encrypted body not processed");
                    request->send(400, "text/plain", "Encrypted body processing failed");
                    return;
                }
            } else if (request->hasParam("username", true) && request->hasParam("password", true)) {
                username = request->getParam("username", true)->value();
                password = request->getParam("password", true)->value();
                LOG_INFO("WebServer", "Login (obfuscated) with plain body for user: [HIDDEN]");
            } else {
                LOG_ERROR("WebServer", "Login (obfuscated): no credentials provided");
                request->send(400, "text/plain", "No credentials provided");
                return;
            }

            // Шаг 2: Проверяем учетные данные
            if (username.length() > 0 && password.length() > 0) {
                
                if (adminManager.verifyCredentials(username, password)) {
                    adminManager.resetLoginAttempts();
                    session_id = CryptoManager::getInstance().generateSecureSessionId();
                    session_csrf_token = CryptoManager::getInstance().generateCsrfToken();
                    session_created_time = millis();
                    
                    savePersistentSession();
                    
                    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Found");
                    response->addHeader("Location", "/");
                    response->addHeader("Set-Cookie", "session=" + session_id + "; Path=/; HttpOnly; SameSite=Strict");
                    response->addHeader("X-Content-Type-Options", "nosniff");
                    response->addHeader("X-Frame-Options", "DENY");
                    response->addHeader("X-XSS-Protection", "1; mode=block");
                    request->send(response);
                    return;
                } else {
                    adminManager.handleFailedLoginAttempt();
                    lockoutTime = adminManager.getLockoutTimeRemaining();
                    if (lockoutTime > 0) {
                        String url = "/login?error=2&time=" + String(lockoutTime);
                        AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Found");
                        response->addHeader("Location", url);
                        request->send(response);
                        return;
                    }
                }
            }

            AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Found");
            response->addHeader("Location", "/login?error=1");
            request->send(response);
        }, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            // 🔐 Body handler для расшифровки XOR зашифрованного тела
            if (index + len == total) {
                bool isEncrypted = request->hasHeader("X-Encrypted-Body") && request->getHeader("X-Encrypted-Body")->value() == "true";
                
                if (isEncrypted && request->hasHeader("X-Client-ID")) {
                    String clientId = request->getHeader("X-Client-ID")->value();
                    String body = String((char*)data, len);
                    
                    
                    JsonDocument doc;
                    if (deserializeJson(doc, body) != DeserializationError::Ok || doc["encrypted"].isNull()) {
                        LOG_ERROR("WebServer", "🔐 Login (obfuscated): Failed to parse encrypted body JSON");
                        return;
                    }
                    
                    String encryptedHex = doc["encrypted"].as<String>();
                    String encryptionKey = "MT_ESP32_" + clientId + "_METHOD_KEY";
                    if (encryptionKey.length() > 32) encryptionKey = encryptionKey.substring(0, 32);
                    
                    String decrypted = "";
                    for (size_t i = 0; i < encryptedHex.length(); i += 2) {
                        String hexByte = encryptedHex.substring(i, i + 2);
                        char encryptedChar = (char)strtol(hexByte.c_str(), NULL, 16);
                        char decryptedChar = encryptedChar ^ encryptionKey[i / 2 % encryptionKey.length()];
                        decrypted += decryptedChar;
                    }
                    
                    JsonDocument* loginData = new JsonDocument();
                    if (deserializeJson(*loginData, decrypted) == DeserializationError::Ok) {
                        request->_tempObject = loginData;
                        LOG_INFO("WebServer", "🔐 Login (obfuscated): Body decrypted successfully");
                    } else {
                        LOG_ERROR("WebServer", "🔐 Login (obfuscated): Failed to parse decrypted JSON");
                        delete loginData;
                    }
                }
            }
        });
    }

    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!WebAdminManager::getInstance().isRegistered()) return request->redirect("/register");
        if (!isAuthenticated(request)) return request->redirect("/login");
        resetActivityTimer(); // Пользователь заходит на главную страницу
        
        // Auto-initialize secure session for main page
        if (shouldInitializeSecureSession(request)) {
            initializeSecureSession(request);
            
            String html = String(PAGE_INDEX);
            injectSecureInitScript(request, html);
            request->send(200, "text/html", html);
        } else {
            request->send_P(200, "text/html", PAGE_INDEX);
        }
    });

    // 🖼️ Splash Screen Management Page
    server.on("/splash", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->redirect("/login");
        request->send_P(200, "text/html", page_splash_html);
    });

    // 🔒 Favicon handler - SVG lock icon for security theme
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
        // Inline SVG lock icon (blue gradient theme) - using single quotes to avoid C++ parsing issues
        const char* svg = "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none'>"
            "<defs><linearGradient id='lockGrad' x1='0%' y1='0%' x2='100%' y2='100%'>"
            "<stop offset='0%' style='stop-color:#4a90e2;stop-opacity:1'/>"
            "<stop offset='100%' style='stop-color:#2e5f8a;stop-opacity:1'/>"
            "</linearGradient></defs>"
            "<rect x='6' y='10' width='12' height='10' rx='2' fill='url(#lockGrad)' stroke='#1e3a5f' stroke-width='1.5'/>"
            "<path d='M8 10V7a4 4 0 0 1 8 0v3' stroke='#4a90e2' stroke-width='2' stroke-linecap='round' fill='none'/>"
            "<circle cx='12' cy='15' r='1.5' fill='#fff'/></svg>";
        request->send(200, "image/svg+xml", svg);
    });
    
    // Alternative: serve as .svg for browsers that prefer it
    server.on("/favicon.svg", HTTP_GET, [](AsyncWebServerRequest *request){
        const char* svg = "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none'>"
            "<defs><linearGradient id='lockGrad' x1='0%' y1='0%' x2='100%' y2='100%'>"
            "<stop offset='0%' style='stop-color:#4a90e2;stop-opacity:1'/>"
            "<stop offset='100%' style='stop-color:#2e5f8a;stop-opacity:1'/>"
            "</linearGradient></defs>"
            "<rect x='6' y='10' width='12' height='10' rx='2' fill='url(#lockGrad)' stroke='#1e3a5f' stroke-width='1.5'/>"
            "<path d='M8 10V7a4 4 0 0 1 8 0v3' stroke='#4a90e2' stroke-width='2' stroke-linecap='round' fill='none'/>"
            "<circle cx='12' cy='15' r='1.5' fill='#fff'/></svg>";
        request->send(200, "image/svg+xml", svg);
    });

#ifdef DEBUG_BUILD
    server.on("/test_secure_encryption.html", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!WebAdminManager::getInstance().isRegistered()) return request->redirect("/register");
        if (!isAuthenticated(request)) return request->redirect("/login");
        resetActivityTimer();
        request->send_P(200, "text/html", page_test_encryption_html);
    });
#endif

    // API: Logout (POST with encryption)
    server.on("/logout", HTTP_POST,
        [this](AsyncWebServerRequest *request){
            // Главный обработчик
            LOG_INFO("WebServer", "🔐 LOGOUT: Main handler called");
        },
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (!data || len == 0) {
                LOG_ERROR("WebServer", "🔐 LOGOUT: Invalid data");
                return request->send(400, "text/plain", "Invalid request data");
            }
            
            LOG_INFO("WebServer", "🔐 LOGOUT: onBody called - len=" + String(len));
            
            if (index + len == total) {
                if (!isAuthenticated(request)) {
                    return request->send(401, "text/plain", "Unauthorized");
                }
                if (!verifyCsrfToken(request)) {
                    return request->send(403, "text/plain", "CSRF token mismatch");
                }
                
#ifdef SECURE_LAYER_ENABLED
                String clientId = WebServerSecureIntegration::getClientId(request);
                
                if (clientId.length() > 0 && 
                    secureLayer.isSecureSessionValid(clientId) &&
                    (request->hasHeader("X-Secure-Request") || 
                     request->hasHeader("X-Security-Level"))) {
                    
                    LOG_INFO("WebServer", "🔐 LOGOUT: Processing encrypted request for " + clientId.substring(0,8) + "...");
                    
                    // Для logout не нужно дешифровать body (он пустой или minimal)
                    // Просто выполняем logout
                    
                    clearSession();
                    clearSecureSession();
                    
                    LOG_INFO("WebServer", "🔐 LOGOUT: Session cleared");
                    
                    // Шифруем ответ
                    String response = "{\"success\":true,\"message\":\"Logged out\"}";
                    String encryptedResponse;
                    
                    if (secureLayer.encryptResponse(clientId, response, encryptedResponse)) {
                        request->send(200, "application/json", encryptedResponse);
                    } else {
                        request->send(500, "text/plain", "Encryption failed");
                    }
                    return;
                }
#endif
                
                // Fallback: незашифрованный logout
                clearSession();
                clearSecureSession();
                request->send(200, "application/json", "{\"success\":true,\"message\":\"Logged out\"}");
            }
        });

#ifdef SECURE_LAYER_ENABLED
    // Serve AutoSecureInitializer JavaScript (embedded content)
    server.on("/secure/auto-init.js", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        
        // Serve the JavaScript content directly from PROGMEM instead of LittleFS
        // This avoids file system dependency and ensures availability
        String jsContent = R"(
/**
 * ESP32 Protected Handshake Auto-Initializer with Method Tunneling (Embedded Version)
 * Full security suite: Software fallback + Method Tunneling + Traffic Obfuscation
 */
(function() {
    'use strict';
    
    console.log('[ESP32-SecureInit] Loading embedded secure initializer with method tunneling...');
    
    // Enhanced SecureClient with Method Tunneling support
    class SecureClient {
        constructor() {
            this.sessionId = null;
            this.isSecureReady = false;
            this.methodTunnelingEnabled = true; // Auto-enabled for production
            this.tunnelingStats = { totalRequests: 0, tunneledRequests: 0 };
        }

        generateSessionId() {
            return Array.from(crypto.getRandomValues(new Uint8Array(16)))
                .map(b => b.toString(16).padStart(2, '0')).join('');
        }

        log(message, type = 'info') {
            console.log(`[SecureClient] ${message}`);
        }

        // XOR fallback encryption for method header (same as ESP32 implementation)
        xorEncrypt(data, key) {
            let result = '';
            for (let i = 0; i < data.length; i++) {
                const charCode = data.charCodeAt(i) ^ key.charCodeAt(i % key.length);
                result += charCode.toString(16).padStart(2, '0');
            }
            return result;
        }

        encryptMethod(method) {
            const clientId = this.sessionId || 'UNKNOWN';
            const encryptionKey = 'MT_ESP32_' + clientId + '_METHOD_KEY';
            const limitedKey = encryptionKey.substring(0, 32);
            return this.xorEncrypt(method, limitedKey);
        }

        shouldTunnelEndpoint(endpoint) {
            const tunneledEndpoints = [
                '/api/passwords',     // All passwords list
                '/api/passwords/get',
                '/api/keys', 
                '/api/add',
                '/api/passwords/add',
                '/api/passwords/delete',
                '/api/passwords/update',
                '/api/passwords/reorder',
                '/api/passwords/export',
                '/api/passwords/import',
                '/api/pincode_settings' // PIN settings (security configuration)
            ];
            return tunneledEndpoints.includes(endpoint);
        }

        // Header Obfuscation Functions (production version)
        processHeadersWithObfuscation(headers, embeddedData = {}) {
            let obfuscatedHeaders = { ...headers };

            // A) Header Mapping - replace sensitive headers
            if (obfuscatedHeaders['X-Client-ID']) {
                obfuscatedHeaders['X-Req-UUID'] = obfuscatedHeaders['X-Client-ID'];
                delete obfuscatedHeaders['X-Client-ID'];
            }
            if (obfuscatedHeaders['X-Secure-Request']) {
                obfuscatedHeaders['X-Security-Level'] = obfuscatedHeaders['X-Secure-Request'];
                delete obfuscatedHeaders['X-Secure-Request'];
            }

            // B) Fake Headers Injection - masquerade as regular browser
            const fakeHeaders = {
                'X-Browser-Engine': 'Mozilla/5.0 (compatible; MSIE 10.0)',
                'X-Request-Time': Date.now().toString(),
                'X-Client-Version': '2.4.1-stable',
                'X-Feature-Flags': 'analytics,tracking,ads',
                'X-Session-State': 'active'
            };
            Object.assign(obfuscatedHeaders, fakeHeaders);

            // C) Header Payload Embedding - hide data in User-Agent
            if (embeddedData && Object.keys(embeddedData).length > 0) {
                const encodedData = btoa(JSON.stringify(embeddedData));
                const baseUserAgent = 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36';
                obfuscatedHeaders['User-Agent'] = `${baseUserAgent} (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36 EdgeInsight/${encodedData}`;
            }

            this.log(`🏷️ Headers obfuscated: sensitive headers masked, fake headers added`);
            return obfuscatedHeaders;
        }

        async secureRequest(url, options = {}) {
            this.tunnelingStats.totalRequests++;
            
            let headers = options.headers || {};
            const shouldTunnel = this.methodTunnelingEnabled && this.shouldTunnelEndpoint(url);
            let actualMethod = options.method || 'GET';
            let actualUrl = url;

            if (shouldTunnel) {
                // Method Tunneling: Convert to POST with encrypted X-Real-Method
                const encryptedMethod = this.encryptMethod(actualMethod);
                headers['X-Real-Method'] = encryptedMethod;
                actualMethod = 'POST';
                actualUrl = '/api/tunnel';
                
                // Add original endpoint to body for tunneling
                const tunnelBody = {
                    endpoint: url,
                    data: options.body ? JSON.parse(options.body) : {}
                };
                options.body = JSON.stringify(tunnelBody);
                
                this.tunnelingStats.tunneledRequests++;
                this.log(`🚇 TUNNELING ${options.method || 'GET'} ${url} -> POST /api/tunnel`);
            }

            // Always add client ID for secure sessions
            if (this.sessionId) {
                headers['X-Client-ID'] = this.sessionId;
                headers['X-Secure-Request'] = 'true';
            }

            // Apply Header Obfuscation - critical for production security
            const embeddedData = { 
                endpoint: url, 
                method: actualMethod, 
                tunneled: shouldTunnel,
                timestamp: Date.now()
            };
            headers = this.processHeadersWithObfuscation(headers, embeddedData);

            return fetch(actualUrl, {
                ...options,
                method: actualMethod,
                headers: headers
            });
        }

        async init() {
            this.sessionId = this.generateSessionId();
            this.log('Initializing secure connection...');
            this.log('Checking browser crypto support...');
            
            if (!window.crypto || !window.crypto.subtle) {
                this.log('Web Crypto API not available (requires HTTPS or localhost)');
                this.log('Falling back to software-based encryption');
                this.log('Software-based encryption activated (Basic protection)');
                this.log('Note: For full security, use HTTPS or localhost access');
            } else {
                this.log('Establishing software-based secure connection...');
                this.log('Software-based encryption activated (Basic protection)');
                this.log('Note: For full security, use HTTPS or localhost access');
            }
            
            this.isSecureReady = true;
            
            // Override fetch for automatic tunneling
            const originalFetch = window.fetch;
            window.fetch = (url, options = {}) => {
                return this.secureRequest(url, options);
            };
            
            this.log('🔒 HTTPS-like encryption ACTIVATED! All API requests are now encrypted.');
        }
    }
    
    // Initialize secure client
    const secureClient = new SecureClient();
    secureClient.init();
    
    // Expose to global scope
    window.ESP32SecureSession = {
        initialized: true,
        mode: 'tunneling-enabled',
        clientId: secureClient.sessionId,
        client: secureClient
    };
    
})();
)";
        
        request->send(200, "application/javascript", jsContent);
    });
    
    // Protected handshake endpoint
    server.on("/secure/protected-handshake", HTTP_POST, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        
        LOG_INFO("WebServer", "🔐 Protected handshake request received");
        
        // Extract client ID from header or body
        String clientId;
        if (request->hasHeader("X-Client-ID")) {
            clientId = request->getHeader("X-Client-ID")->value();
        }
        
        if (clientId.isEmpty()) {
            LOG_ERROR("WebServer", "Protected handshake missing client ID");
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Client ID required\"}");
            return;
        }
        
        
        // Mark this as authenticated user activity
        resetActivityTimer();
        
        // Temporary response - will be handled by body parser
        request->send(200, "application/json", "{\"status\":\"processing\"}");
    }, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
        if (!isAuthenticated(request)) return;
        
        String body = String((char*)data).substring(0, len);
        
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            LOG_ERROR("WebServer", "Invalid JSON in protected handshake");
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        
        String clientId = doc["client_id"];
        String encryptedClientKey = doc["encrypted_pubkey"];
        
        if (clientId.isEmpty() || encryptedClientKey.isEmpty()) {
            LOG_ERROR("WebServer", "Missing required fields in protected handshake");
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing required fields\"}");
            return;
        }
        
        // Process protected key exchange using SecureLayerManager
        String response;
        if (secureLayer.processProtectedKeyExchange(clientId, encryptedClientKey, response)) {
            LOG_INFO("WebServer", "🔐 Protected handshake successful");
            request->send(200, "application/json", response);
        } else {
            LOG_ERROR("WebServer", "🔐 Protected handshake failed");
            request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Handshake processing failed\"}");
        }
    });
    
    // Protected session status endpoint
    server.on("/secure/status", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        
        JsonDocument doc;
        doc["secure_layer_active"] = true;
        doc["active_sessions"] = secureLayer.getActiveSecureSessionCount();
        doc["server_initialized"] = secureLayer.isSecureSessionValid("dummy"); // Quick check
        
        if (secureHandshakeActive) {
            doc["handshake_active"] = true;
            doc["client_id"] = currentSecureClientId.substring(0,8) + "...";
            doc["handshake_duration"] = millis() - handshakeStartTime;
        } else {
            doc["handshake_active"] = false;
        }
        
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });
#endif

    // API: Get all keys (SECURE TESTING ENABLED + URL OBFUSCATION)
    URLObfuscationIntegration::registerDualEndpoint(server, "/api/keys", HTTP_GET, 
        [this](AsyncWebServerRequest *request){
            if (!isAuthenticated(request)) return request->send(401);
            
            // НЕ сбрасываем таймер активности для автоматических обновлений TOTP
            // Проверяем, есть ли заголовок, указывающий на пользовательскую активность
            if (request->hasHeader("X-User-Activity")) {
                resetActivityTimer();
            }
            
            // Ограничиваем размер JSON для TOTP ключей (max ~50 ключей)
            JsonDocument doc;
            JsonArray keysArray = doc.to<JsonArray>();
            
            auto keys = keyManager.getAllKeys();
            
            // 🔐 Блокировка TOTP в AP/Offline режимах
            bool blockTOTP = !totpGenerator.isTimeSynced();
            
            for (size_t i = 0; i < keys.size(); i++) {
                JsonObject keyObj = keysArray.add<JsonObject>();
                
                // Основные поля (всегда присутствуют)
                keyObj["name"] = keys[i].name;
                // HOTP works without time sync, TOTP requires it
                if (keys[i].type == TOTPType::HOTP) {
                    keyObj["code"] = totpGenerator.generateCode(keys[i]);
                } else {
                    keyObj["code"] = blockTOTP ? "NOT SYNCED" : totpGenerator.generateCode(keys[i]);
                }
                
                // Время до следующего кода (для TOTP) или 0 для HOTP
                if (keys[i].type == TOTPType::TOTP) {
                    keyObj["timeLeft"] = totpGenerator.getTimeRemaining(keys[i].period);
                } else {
                    keyObj["timeLeft"] = 0; // HOTP не имеет таймера
                }
                
                // 🆕 РАСШИРЕННЫЕ МЕТАДАННЫЕ (компактный формат)
                // Всегда включаем метаданные для корректной работы фронтенда
                keyObj["t"] = (keys[i].type == TOTPType::HOTP) ? "H" : "T";
                keyObj["a"] = (keys[i].algorithm == TOTPAlgorithm::SHA256) ? "256" :
                              (keys[i].algorithm == TOTPAlgorithm::SHA512) ? "512" : "1";
                keyObj["d"] = keys[i].digits;
                keyObj["p"] = keys[i].period;
                
                if (keys[i].counter != 0) {
                    keyObj["c"] = keys[i].counter;
                }
            }
            
            String response;
            serializeJson(doc, response);
            
#ifdef SECURE_LAYER_ENABLED
            // 🎭 HEADER OBFUSCATION: Используем деобфускацию заголовков
            HeaderObfuscationManager& headerObf = HeaderObfuscationManager::getInstance();
            HeaderObfuscationIntegration::logObfuscatedRequest(request, "/api/keys", headerObf);
            
            // КРИТИЧНО: Принудительное шифрование TOTP данных
            // Поддержка обфусцированных заголовков (X-Req-UUID)
            String clientId = HeaderObfuscationIntegration::getClientId(request, headerObf);
            bool isTunneled = request->hasHeader("X-Real-Method");
            
            // УСИЛЕННАЯ ПРОВЕРКА: Ищем clientId в разных местах для tunneled запросов
            if (clientId.isEmpty() && isTunneled) {
                // Альтернативные источники clientId при tunneling
                if (request->hasHeader("Authorization")) {
                    String auth = request->getHeader("Authorization")->value();
                    if (auth.startsWith("Bearer ")) {
                        clientId = auth.substring(7);
                    }
                }
                LOG_DEBUG("WebServer", "🚇 TUNNELED clientId recovery attempt: " + 
                         (clientId.length() > 0 ? clientId.substring(0,8) + "..." : "FAILED"));
            }
            
            // Гарантированная проверка secure session для TOTP (прямых и tunneled запросов)
            if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId)) {
                LOG_INFO("WebServer", "🔐 TOTP ENCRYPTION: Securing keys data for client " + clientId.substring(0,8) + "..." + (isTunneled ? " [TUNNELED]" : " [DIRECT]"));
                WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", response, secureLayer);
                return;
            } else if (clientId.length() > 0) {
                LOG_WARNING("WebServer", "🔐 TOTP FALLBACK: No valid secure session for " + clientId.substring(0,8) + "..., sending plaintext");
            } else {
                // ОТЛАДКА ОТКЛЮЧЕНА: Избегаем LOG SPAM от автоматических TOTP обновлений
                if (isTunneled) {
                    LOG_DEBUG("WebServer", "🔐 TOTP FALLBACK: Missing clientId header [TUNNELED REQUEST]");
                }
                // Обычные запросы без clientId - тихо отправляем plaintext (автоматические TOTP updates)
            }
#endif
            
            request->send(200, "application/json", response);
        }, urlObfuscation);

    // API: Add key (🎭 HEADER OBFUSCATION + 🔗 URL OBFUSCATION + 🔐 XOR ENCRYPTION)
    auto keyAddHandler = [this](AsyncWebServerRequest *request){
        // Основной обработчик - пустой, вся логика в onBody callback
    };
    
    auto keyAddBodyHandler = [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        // onBody callback - обрабатывает тело запроса для расшифровки
        if (index + len == total) {
            if (!isAuthenticated(request)) return request->send(401);
            if (!verifyCsrfToken(request)) return request->send(403, "text/plain", "CSRF token mismatch");
            
            // 🛡️ SECURITY: Проверка размера входящего пакета (max 1024 байт)
            if (len > 1024) {
                LOG_ERROR("WebServer", "KEY ADD: Request body too large: " + String(len) + " bytes");
                return request->send(413, "application/json", "{\"error\":\"Request body too large\"}");
            }
            
            String bodyStr;
            bool isJsonFormat = false;
            
#ifdef SECURE_LAYER_ENABLED
            // 🎭 HEADER OBFUSCATION: Деобфускация заголовков
            HeaderObfuscationManager& headerObf = HeaderObfuscationManager::getInstance();
            HeaderObfuscationIntegration::logObfuscatedRequest(request, "/api/add", headerObf);
            
            // Получаем clientId с поддержкой обфусцированных заголовков
            String clientId = HeaderObfuscationIntegration::getClientId(request, headerObf);
            bool isSecureReq = HeaderObfuscationIntegration::isSecureRequest(request, headerObf);
            
            if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId) && isSecureReq) {
                LOG_INFO("WebServer", "🔐 KEY ADD: Decrypting request body for " + clientId.substring(0,8) + "...");
                
                // Расшифровываем тело запроса
                String encryptedBody = String((char*)data, len);
                String decryptedBody;
                
                if (secureLayer.decryptRequest(clientId, encryptedBody, decryptedBody)) {
                    LOG_DEBUG("WebServer", "🔐 Decrypted body length: " + String(decryptedBody.length()) + " bytes");
                    bodyStr = decryptedBody;
                    
                    // Проверяем формат (JSON или FormData)
                    isJsonFormat = bodyStr.startsWith("{");
                } else {
                    LOG_ERROR("WebServer", "🔐 Failed to decrypt request body");
                    return request->send(400, "text/plain", "Decryption failed");
                }
            } else 
#endif
            {
                // Обычный незашифрованный запрос
                bodyStr = String((char*)data, len);
                
                // Проверяем Content-Type для определения формата
                if (request->hasHeader("Content-Type")) {
                    String contentType = request->getHeader("Content-Type")->value();
                    isJsonFormat = contentType.indexOf("application/json") >= 0;
                } else {
                    // Автоопределение по первому символу
                    isJsonFormat = bodyStr.startsWith("{");
                }
            }
            
            // === ПАРСИНГ ДАННЫХ ===
            String name, secret;
            TOTPType type = TOTPType::TOTP;
            TOTPAlgorithm algorithm = TOTPAlgorithm::SHA1;
            uint8_t digits = 6;
            uint16_t period = 30;
            uint32_t counter = 0;
            
            if (isJsonFormat) {
                // 🆕 НОВЫЙ ФОРМАТ: Компактный JSON {n, s, t, a, d, p, c}
                LOG_INFO("WebServer", "KEY ADD: Parsing JSON format");
                
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, bodyStr);
                
                if (error) {
                    LOG_ERROR("WebServer", "KEY ADD: JSON parsing failed: " + String(error.c_str()));
                    return request->send(400, "application/json", "{\"error\":\"Invalid JSON format\"}");
                }
                
                // Обязательные поля
                if (doc["n"].isNull() || doc["s"].isNull()) {
                    LOG_ERROR("WebServer", "KEY ADD: Missing required fields (n, s)");
                    return request->send(400, "application/json", "{\"error\":\"Missing required fields: n (name), s (secret)\"}");
                }
                
                name = doc["n"].as<String>();
                secret = doc["s"].as<String>();
                
                // Опциональные поля с валидацией
                if (!doc["t"].isNull()) {
                    String typeStr = doc["t"].as<String>();
                    if (typeStr == "H") {
                        type = TOTPType::HOTP;
                    } else if (typeStr == "T") {
                        type = TOTPType::TOTP;
                    } else {
                        LOG_ERROR("WebServer", "KEY ADD: Invalid type value: " + typeStr);
                        return request->send(400, "application/json", "{\"error\":\"Invalid type (t): must be 'T' or 'H'\"}");
                    }
                }
                
                if (!doc["a"].isNull()) {
                    String algoStr = doc["a"].as<String>();
                    if (algoStr == "1") {
                        algorithm = TOTPAlgorithm::SHA1;
                    } else if (algoStr == "256") {
                        algorithm = TOTPAlgorithm::SHA256;
                    } else if (algoStr == "512") {
                        algorithm = TOTPAlgorithm::SHA512;
                    } else {
                        LOG_ERROR("WebServer", "KEY ADD: Invalid algorithm value: " + algoStr);
                        return request->send(400, "application/json", "{\"error\":\"Invalid algorithm (a): must be '1', '256', or '512'\"}");
                    }
                }
                
                if (!doc["d"].isNull()) {
                    digits = doc["d"].as<uint8_t>();
                    if (digits != 6 && digits != 8) {
                        LOG_ERROR("WebServer", "KEY ADD: Invalid digits value: " + String(digits));
                        return request->send(400, "application/json", "{\"error\":\"Invalid digits (d): must be 6 or 8\"}");
                    }
                }
                
                if (!doc["p"].isNull()) {
                    period = doc["p"].as<uint16_t>();
                    if (period != 30 && period != 60) {
                        LOG_ERROR("WebServer", "KEY ADD: Invalid period value: " + String(period));
                        return request->send(400, "application/json", "{\"error\":\"Invalid period (p): must be 30 or 60\"}");
                    }
                }
                
                if (!doc["c"].isNull()) {
                    counter = doc["c"].as<uint32_t>();
                }
                
            } else {
                // 🔙 ОБРАТНАЯ СОВМЕСТИМОСТЬ: Старый FormData формат
                LOG_INFO("WebServer", "KEY ADD: Parsing FormData format (legacy)");
                
                // Парсим FormData вручную
                int nameStart = bodyStr.indexOf("name=");
                int secretStart = bodyStr.indexOf("secret=");
                
                if (nameStart >= 0 && secretStart >= 0) {
                    int nameEnd = bodyStr.indexOf("&", nameStart);
                    if (nameEnd == -1) nameEnd = bodyStr.length();
                    
                    int secretEnd = bodyStr.indexOf("&", secretStart);
                    if (secretEnd == -1) secretEnd = bodyStr.length();
                    
                    name = bodyStr.substring(nameStart + 5, nameEnd);
                    secret = bodyStr.substring(secretStart + 7, secretEnd);
                    
                    // URL decode
                    name.replace("+", " ");
                    secret.replace("+", " ");
                    
                    // Используем значения по умолчанию для старого формата
                } else {
                    LOG_ERROR("WebServer", "KEY ADD: Invalid FormData format");
                    return request->send(400, "text/plain", "Invalid FormData format");
                }
            }
            
            // === ФИНАЛЬНАЯ ВАЛИДАЦИЯ ===
            if (name.isEmpty() || secret.isEmpty()) {
                LOG_ERROR("WebServer", "KEY ADD: Name or secret is empty");
                return request->send(400, "application/json", "{\"error\":\"Name and secret cannot be empty\"}");
            }
            
            // Валидация Base32 секрета
            secret.toUpperCase();
            secret.replace(" ", "");
            secret.replace("-", "");
            
            if (secret.length() < 16) {
                LOG_ERROR("WebServer", "KEY ADD: Secret too short: " + String(secret.length()));
                return request->send(400, "application/json", "{\"error\":\"Secret too short (minimum 16 characters)\"}");
            }
            
            // Проверка допустимых символов Base32 (A-Z, 2-7, =)
            for (size_t i = 0; i < secret.length(); i++) {
                char c = secret.charAt(i);
                if (!((c >= 'A' && c <= 'Z') || (c >= '2' && c <= '7') || c == '=')) {
                    LOG_ERROR("WebServer", "KEY ADD: Invalid Base32 character at position " + String(i));
                    return request->send(400, "application/json", "{\"error\":\"Invalid Base32 format (use A-Z, 2-7 only)\"}");
                }
            }
            
            // === ДОБАВЛЕНИЕ КЛЮЧА ===
            LOG_DEBUG("KeyManager", String("Adding key [Type=") + 
                     (type == TOTPType::HOTP ? "HOTP" : "TOTP") + 
                     ", Algo=" + (algorithm == TOTPAlgorithm::SHA1 ? "SHA1" : 
                                 algorithm == TOTPAlgorithm::SHA256 ? "SHA256" : "SHA512") +
                     ", Digits=" + String(digits) + 
                     ", Period=" + String(period) + 
                     ", Counter=" + String(counter) + "]");
            
            bool success;
            if (type == TOTPType::HOTP || algorithm != TOTPAlgorithm::SHA1 || 
                digits != 6 || period != 30) {
                // Используем расширенный метод для нестандартных параметров
                success = keyManager.addKeyExtended(name, secret, type, algorithm, digits, period);
            } else {
                // Используем простой метод для стандартных TOTP ключей (обратная совместимость)
                success = keyManager.addKey(name, secret);
            }
            
            // === ФОРМИРОВАНИЕ ОТВЕТА ===
            JsonDocument responseDoc;
            if (success) {
                responseDoc["status"] = "success";
                responseDoc["message"] = "Key added successfully";
                responseDoc["name"] = name;
                LOG_INFO("WebServer", "KEY ADD: Success");
            } else {
                responseDoc["status"] = "error";
                responseDoc["message"] = "Failed to add key (duplicate name?)";
                LOG_ERROR("WebServer", "KEY ADD: Failed");
            }
            
            String output;
            serializeJson(responseDoc, output);
            
#ifdef SECURE_LAYER_ENABLED
            // 🎭 Используем обфусцированные заголовки для response
            String clientId2 = HeaderObfuscationIntegration::getClientId(request, headerObf);
            if (clientId2.length() > 0 && secureLayer.isSecureSessionValid(clientId2)) {
                LOG_INFO("WebServer", "🔐 KEY ADD ENCRYPTION: Securing response");
                WebServerSecureIntegration::sendSecureResponse(request, success ? 200 : 400, "application/json", output, secureLayer);
                return;
            }
#endif
            
            request->send(success ? 200 : 400, "application/json", output);
        }
    };
    
    // Регистрируем два варианта: оригинальный и обфусцированный URL
    server.on("/api/add", HTTP_POST, keyAddHandler, NULL, keyAddBodyHandler);
    String obfuscatedAddPath = urlObfuscation.obfuscateURL("/api/add");
    if (obfuscatedAddPath.length() > 0 && obfuscatedAddPath != "/api/add") {
        server.on(obfuscatedAddPath.c_str(), HTTP_POST, keyAddHandler, NULL, keyAddBodyHandler);
        LOG_DEBUG("WebServer", "🔗 Registered obfuscated /api/add -> " + obfuscatedAddPath);
    }

    // ==================== API: Show QR Code on Display ====================
    // Показать QR код TOTP ключа на экране ESP32 для экспорта
    auto showQrHandler = [this](AsyncWebServerRequest *request) {
        // Основной обработчик - пустой, вся логика в onBody
    };
    
    auto showQrBodyHandler = [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (index + len == total) {
            // 🐛 DEBUG: Log raw body first
            LOG_DEBUG("WebServer", "SHOW_QR body: " + String(len) + " bytes: " + String((char*)data, len));
            
            // 1. Аутентификация и CSRF проверка
            if (!isAuthenticated(request)) return request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            if (!verifyCsrfToken(request)) return request->send(403, "application/json", "{\"error\":\"CSRF token mismatch\"}");
            
            int keyIndex = -1;
            
#ifdef SECURE_LAYER_ENABLED
            String clientId = WebServerSecureIntegration::getClientId(request);
            if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId) && 
                (request->hasHeader("X-Secure-Request") || request->hasHeader("X-Security-Level"))) {
                
                LOG_INFO("WebServer", "🔐 SHOW_QR: Decrypting request body for " + clientId.substring(0,8) + "...");
                
                // Расшифровываем тело запроса
                String encryptedBody = String((char*)data, len);
                String decryptedBody;
                
                if (secureLayer.decryptRequest(clientId, encryptedBody, decryptedBody)) {
                    LOG_DEBUG("WebServer", "🔐 Decrypted show_qr body: " + decryptedBody);
                    
                    // Парсим расшифрованные данные (формат: key_id=0)
                    int keyIdStart = decryptedBody.indexOf("key_id=");
                    if (keyIdStart >= 0) {
                        int keyIdEnd = decryptedBody.indexOf("&", keyIdStart);
                        if (keyIdEnd == -1) keyIdEnd = decryptedBody.length();
                        
                        String keyIdStr = decryptedBody.substring(keyIdStart + 7, keyIdEnd); // skip "key_id="
                        keyIndex = keyIdStr.toInt();
                        
                        LOG_DEBUG("WebServer", "🔐 Parsed key_id: " + String(keyIndex));
                    } else {
                        return request->send(400, "application/json", "{\"error\":\"Invalid decrypted format\"}");
                    }
                } else {
                    LOG_ERROR("WebServer", "🔐 Failed to decrypt show_qr request body");
                    return request->send(400, "application/json", "{\"error\":\"Decryption failed\"}");
                }
            } else 
#endif
            {
                // Обычный незашифрованный запрос - читаем параметры
                if (request->hasParam("key_id", true)) {
                    keyIndex = request->getParam("key_id", true)->value().toInt();
                } else {
                    return request->send(400, "application/json", "{\"error\":\"Missing key_id parameter\"}");
                }
            }
            
            if (keyIndex < 0) {
                return request->send(400, "application/json", "{\"error\":\"Invalid key index\"}");
            }
            
            // 4. Получить ключ из KeyManager
            auto keys = keyManager.getAllKeys();
            if (keyIndex < 0 || keyIndex >= keys.size()) {
                LOG_ERROR("WebServer", "SHOW_QR: Invalid key index: " + String(keyIndex));
                return request->send(404, "application/json", "{\"error\":\"Key not found\"}");
            }
            
            TOTPKey key = keys[keyIndex];
            
            // КРИТИЧНО: TOTP secret ДОЛЖЕН быть в uppercase (Base32 формат)
            String secretUpper = key.secret;
            secretUpper.toUpperCase();
            
            // Формируем полный TOTP/HOTP URI с расширенными параметрами
            String totpUri;
            if (key.type == TOTPType::HOTP) {
                // HOTP URI
                totpUri = "otpauth://hotp/" + key.name + "?secret=" + secretUpper;
                totpUri += "&counter=" + String(key.counter);
            } else {
                // TOTP URI
                totpUri = "otpauth://totp/" + key.name + "?secret=" + secretUpper;
                if (key.period != 30) {
                    totpUri += "&period=" + String(key.period);
                }
            }
            
            // Добавляем алгоритм (если не SHA1)
            if (key.algorithm == TOTPAlgorithm::SHA256) {
                totpUri += "&algorithm=SHA256";
            } else if (key.algorithm == TOTPAlgorithm::SHA512) {
                totpUri += "&algorithm=SHA512";
            }
            
            // Добавляем количество цифр (если не 6)
            if (key.digits != 6) {
                totpUri += "&digits=" + String(key.digits);
            }
            
            LOG_INFO("WebServer", "SHOW_QR: Displaying QR for key at index " + String(index) + " (type=" + 
                     (key.type == TOTPType::HOTP ? "HOTP" : "TOTP") + ", algo=SHA" + 
                     (key.algorithm == TOTPAlgorithm::SHA256 ? "256" : key.algorithm == TOTPAlgorithm::SHA512 ? "512" : "1") + 
                     ", digits=" + String(key.digits) + ")");
            
            // 6. Показать QR на экране (thread-safe request)
            displayManager.requestShowQRCode(totpUri, 30); // 30 секунд
            
            // 7. Отправить ответ
            JsonDocument responseDoc;
            responseDoc["success"] = true;
            responseDoc["message"] = "QR code displayed on device screen for 30 seconds";
            
            String response;
            serializeJson(responseDoc, response);
            
#ifdef SECURE_LAYER_ENABLED
            // Шифруем ответ если была защищенная сессия
            String clientId2 = WebServerSecureIntegration::getClientId(request);
            if (clientId2.length() > 0 && secureLayer.isSecureSessionValid(clientId2)) {
                LOG_INFO("WebServer", "🔐 SHOW_QR ENCRYPTION: Securing response");
                WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", response, secureLayer);
                return;
            }
#endif
            request->send(200, "application/json", response);
        }
    };
    
    // Регистрируем обработчик
    server.on("/api/show_qr", HTTP_POST, showQrHandler, NULL, showQrBodyHandler);
    String obfuscatedShowQrPath = urlObfuscation.obfuscateURL("/api/show_qr");
    if (obfuscatedShowQrPath.length() > 0 && obfuscatedShowQrPath != "/api/show_qr") {
        server.on(obfuscatedShowQrPath.c_str(), HTTP_POST, showQrHandler, NULL, showQrBodyHandler);
        LOG_DEBUG("WebServer", "🔗 Registered obfuscated /api/show_qr -> " + obfuscatedShowQrPath);
    }
    // ==================== END: Show QR Code ====================

    // API: Get server configuration (timeout, etc.) - SECURE LAYER ENABLED    // API: Get server configuration (timeout, etc.) - SECURE LAYER ENABLED
    server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        
        // Конфигурация - небольшой размер
        JsonDocument doc;
        // Всегда берем актуальное значение из конфигурации для синхронизации
        doc["web_server_timeout"] = configManager.getWebServerTimeout();
        
        String response;
        serializeJson(doc, response);
        
#ifdef SECURE_LAYER_ENABLED
        // 🔐 Шифруем ответ если активна защищенная сессия
        String clientId = WebServerSecureIntegration::getClientId(request);
        if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId)) {
            LOG_DEBUG("WebServer", "🔐 CONFIG: Encrypting response for " + clientId.substring(0,8) + "...");
            WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", response, secureLayer);
            return;
        }
#endif
        // Fallback: незашифрованный ответ
        request->send(200, "application/json", response);
    });

    // API: Get CSRF token
    server.on("/api/csrf_token", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        
        JsonDocument doc;
        doc["csrf_token"] = session_csrf_token;
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // API: Reset activity timer (keep session alive)
    server.on("/api/activity", HTTP_POST, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        resetActivityTimer();
        request->send(200, "application/json", "{\"status\":\"success\"}");
    });

    server.on("/api/remove", HTTP_POST, [this](AsyncWebServerRequest *request){
        // Основной обработчик - пустой, вся логика в onBody callback
    }, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        // onBody callback - обрабатывает тело запроса для расшифровки
        if (index + len == total) {
            if (!isAuthenticated(request)) return request->send(401);
            if (!verifyCsrfToken(request)) return request->send(403, "text/plain", "CSRF token mismatch");
            
            int keyIndex = -1;
            
#ifdef SECURE_LAYER_ENABLED
            String clientId = WebServerSecureIntegration::getClientId(request);
            if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId) && 
                (request->hasHeader("X-Secure-Request") || request->hasHeader("X-Security-Level"))) {
                
                LOG_INFO("WebServer", "🔐 KEY REMOVE: Decrypting request body for " + clientId.substring(0,8) + "...");
                
                // Расшифровываем тело запроса
                String encryptedBody = String((char*)data, len);
                String decryptedBody;
                
                if (secureLayer.decryptRequest(clientId, encryptedBody, decryptedBody)) {
                    LOG_DEBUG("WebServer", "🔐 Decrypted key remove body: " + decryptedBody);
                    
                    // Парсим расшифрованные данные (формат: index=0)
                    int indexStart = decryptedBody.indexOf("index=");
                    if (indexStart >= 0) {
                        int indexEnd = decryptedBody.indexOf("&", indexStart);
                        if (indexEnd == -1) indexEnd = decryptedBody.length();
                        
                        String indexStr = decryptedBody.substring(indexStart + 6, indexEnd); // skip "index="
                        keyIndex = indexStr.toInt();
                        
                        LOG_DEBUG("WebServer", "🔐 Parsed key index: " + String(keyIndex));
                    } else {
                        return request->send(400, "text/plain", "Invalid decrypted key remove format");
                    }
                } else {
                    LOG_ERROR("WebServer", "🔐 Failed to decrypt key remove request body");
                    return request->send(400, "text/plain", "Key remove decryption failed");
                }
            } else 
#endif
            {
                // Обычный незашифрованный запрос - читаем параметры
                if (request->hasParam("index", true)) {
                    keyIndex = request->getParam("index", true)->value().toInt();
                } else {
                    return request->send(400, "text/plain", "Missing index parameter");
                }
            }
            
            if (keyIndex < 0) {
                return request->send(400, "text/plain", "Invalid key index");
            }
            
            keyManager.removeKey(keyIndex);
            
            // Формируем JSON ответ с подтверждением
            JsonDocument doc;
            doc["status"] = "success";
            doc["message"] = "Key removed successfully";
            String response;
            serializeJson(doc, response);
            
#ifdef SECURE_LAYER_ENABLED
            String clientId2 = WebServerSecureIntegration::getClientId(request);
            if (clientId2.length() > 0 && secureLayer.isSecureSessionValid(clientId2)) {
                LOG_INFO("WebServer", "🔐 KEY REMOVE ENCRYPTION: Securing response");
                WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", response, secureLayer);
                return;
            }
#endif
            
            request->send(200, "application/json", response);
        }
    });

    // API: HOTP Generate (increment counter and return new code)
    auto hotpGenerateHandler = [this](AsyncWebServerRequest *request){
        // Основной обработчик - пустой, вся логика в onBody callback
    };
    
    auto hotpGenerateBodyHandler = [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        // onBody callback - обрабатывает тело запроса для расшифровки
        if (index + len == total) {
            if (!isAuthenticated(request)) return request->send(401);
            if (!verifyCsrfToken(request)) return request->send(403, "text/plain", "CSRF token mismatch");
            
            resetActivityTimer();
            
            int keyIndex = -1;
            
#ifdef SECURE_LAYER_ENABLED
            String clientId = WebServerSecureIntegration::getClientId(request);
            if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId) && 
                (request->hasHeader("X-Secure-Request") || request->hasHeader("X-Security-Level"))) {
                
                LOG_INFO("WebServer", "🔐 HOTP GENERATE: Decrypting request body for " + clientId.substring(0,8) + "...");
                
                // Расшифровываем тело запроса
                String encryptedBody = String((char*)data, len);
                String decryptedBody;
                
                if (secureLayer.decryptRequest(clientId, encryptedBody, decryptedBody)) {
                    LOG_DEBUG("WebServer", "🔐 Decrypted HOTP generate body: " + decryptedBody);
                    
                    // Парсим расшифрованные данные (формат: index=0)
                    int indexStart = decryptedBody.indexOf("index=");
                    if (indexStart >= 0) {
                        int indexEnd = decryptedBody.indexOf("&", indexStart);
                        if (indexEnd == -1) indexEnd = decryptedBody.length();
                        
                        String indexStr = decryptedBody.substring(indexStart + 6, indexEnd); // skip "index="
                        keyIndex = indexStr.toInt();
                        
                        LOG_DEBUG("WebServer", "🔐 Parsed key index: " + String(keyIndex));
                    } else {
                        return request->send(400, "text/plain", "Invalid decrypted HOTP generate format");
                    }
                } else {
                    LOG_ERROR("WebServer", "🔐 Failed to decrypt HOTP generate request body");
                    return request->send(400, "text/plain", "HOTP generate decryption failed");
                }
            } else 
#endif
            {
                // Обычный незашифрованный запрос - читаем параметры
                if (request->hasParam("index", true)) {
                    keyIndex = request->getParam("index", true)->value().toInt();
                } else {
                    return request->send(400, "text/plain", "Missing index parameter");
                }
            }
            
            if (keyIndex < 0) {
                return request->send(400, "text/plain", "Invalid key index");
            }
            
            auto keys = keyManager.getAllKeys();
            if (keyIndex >= keys.size()) {
                return request->send(400, "text/plain", "Key index out of range");
            }
            
            if (keys[keyIndex].type != TOTPType::HOTP) {
                return request->send(400, "text/plain", "Key is not HOTP type");
            }
            
            // Инкрементируем счетчик
            if (!keyManager.incrementHOTPCounter(keyIndex)) {
                return request->send(500, "text/plain", "Failed to increment HOTP counter");
            }
            
            // Получаем обновленный ключ и генерируем новый код
            auto updatedKeys = keyManager.getAllKeys();
            String newCode = totpGenerator.generateCode(updatedKeys[keyIndex]);
            
            // Формируем JSON ответ
            JsonDocument doc;
            doc["status"] = "success";
            doc["code"] = newCode;
            doc["counter"] = updatedKeys[keyIndex].counter;
            String response;
            serializeJson(doc, response);
            
#ifdef SECURE_LAYER_ENABLED
            String clientId2 = WebServerSecureIntegration::getClientId(request);
            if (clientId2.length() > 0 && secureLayer.isSecureSessionValid(clientId2)) {
                LOG_INFO("WebServer", "🔐 HOTP GENERATE ENCRYPTION: Securing response");
                WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", response, secureLayer);
                return;
            }
#endif
            
            request->send(200, "application/json", response);
        }
    };
    
    // Регистрируем обработчик
    server.on("/api/hotp/generate", HTTP_POST, hotpGenerateHandler, NULL, hotpGenerateBodyHandler);
    String obfuscatedHotpPath = urlObfuscation.obfuscateURL("/api/hotp/generate");
    if (obfuscatedHotpPath.length() > 0 && obfuscatedHotpPath != "/api/hotp/generate") {
        server.on(obfuscatedHotpPath.c_str(), HTTP_POST, hotpGenerateHandler, NULL, hotpGenerateBodyHandler);
        LOG_DEBUG("WebServer", "🔗 Registered obfuscated /api/hotp/generate -> " + obfuscatedHotpPath);
    }

    // Key update endpoint removed for security
    
    // API: Keys reordering (🎭 HEADER OBFUSCATION + 🔗 URL OBFUSCATION + 🔐 XOR ENCRYPTION)
    auto keysReorderHandler = [this](AsyncWebServerRequest *request){
        // Основной обработчик - пустой, вся логика в onBody callback
    };
    
    auto keysReorderBodyHandler = [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
        if (index + len == total) {
            if (!isAuthenticated(request)) return request->send(401);
            if (!verifyCsrfToken(request)) return request->send(403, "text/plain", "CSRF token mismatch");
            
            String body;
            
#ifdef SECURE_LAYER_ENABLED
            // 🎭 HEADER OBFUSCATION: Деобфускация заголовков
            HeaderObfuscationManager& headerObf = HeaderObfuscationManager::getInstance();
            HeaderObfuscationIntegration::logObfuscatedRequest(request, "/api/keys/reorder", headerObf);
            
            // Получаем clientId с поддержкой обфусцированных заголовков
            String clientId = HeaderObfuscationIntegration::getClientId(request, headerObf);
            bool isSecureReq = HeaderObfuscationIntegration::isSecureRequest(request, headerObf);
            
            if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId) && isSecureReq) {
                
                LOG_INFO("WebServer", "🔐 KEYS REORDER: Decrypting request body for " + clientId.substring(0,8) + "...");
                
                // Расшифровываем тело запроса
                String encryptedBody = String((char*)data, len);
                String decryptedBody;
                
                if (secureLayer.decryptRequest(clientId, encryptedBody, decryptedBody)) {
                    LOG_DEBUG("WebServer", "🔐 Decrypted reorder body length: " + String(decryptedBody.length()) + " bytes");
                    body = decryptedBody;
                } else {
                    LOG_ERROR("WebServer", "🔐 Failed to decrypt request body");
                    return request->send(400, "text/plain", "Decryption failed");
                }
            } else 
#endif
            {
                // Обычный незашифрованный запрос
                body = String((char*)data).substring(0, len);
            }
            
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, body);
            
            if (error) {
                return request->send(400, "text/plain", "Invalid JSON");
            }
            
            if (!doc["order"].is<JsonArray>()) {
                return request->send(400, "text/plain", "Missing or invalid 'order' field");
            }
            
            std::vector<std::pair<String, int>> newOrder;
            JsonArray orderArray = doc["order"];
            
            for (JsonObject item : orderArray) {
                String name = item["name"];
                int order = item["order"];
                newOrder.push_back(std::make_pair(name, order));
            }
            
            // Выполняем переупорядочивание
            bool success = keyManager.reorderKeys(newOrder);
            
            String response;
            int statusCode;
            if (success) {
                response = "Keys reordered successfully!";
                statusCode = 200;
                LOG_INFO("WebServer", "Keys reordered successfully");
            } else {
                response = "Failed to reorder keys";
                statusCode = 500;
                LOG_ERROR("WebServer", "Failed to reorder keys");
            }
            
#ifdef SECURE_LAYER_ENABLED
            // 🎭 Используем обфусцированные заголовки для response
            String clientId2 = HeaderObfuscationIntegration::getClientId(request, headerObf);
            if (clientId2.length() > 0 && secureLayer.isSecureSessionValid(clientId2)) {
                LOG_INFO("WebServer", "🔐 KEYS REORDER: Encrypting response");
                WebServerSecureIntegration::sendSecureResponse(request, statusCode, "text/plain", response, secureLayer);
                return;
            }
#endif
            // Fallback: незашифрованный ответ
            request->send(statusCode, "text/plain", response);
        }
    };
    
    // Регистрируем два варианта: оригинальный и обфусцированный URL
    server.on("/api/keys/reorder", HTTP_POST, keysReorderHandler, NULL, keysReorderBodyHandler);
    String obfuscatedReorderPath = urlObfuscation.obfuscateURL("/api/keys/reorder");
    if (obfuscatedReorderPath.length() > 0 && obfuscatedReorderPath != "/api/keys/reorder") {
        server.on(obfuscatedReorderPath.c_str(), HTTP_POST, keysReorderHandler, NULL, keysReorderBodyHandler);
        LOG_DEBUG("WebServer", "🔗 Registered obfuscated /api/keys/reorder -> " + obfuscatedReorderPath);
    }

    // API: Passwords (SECURE TESTING ENABLED + URL OBFUSCATION)
    URLObfuscationIntegration::registerDualEndpoint(server, "/api/passwords", HTTP_GET, 
        [this](AsyncWebServerRequest *request){
            if (!isAuthenticated(request)) return request->send(401);
            
            // Сбрасываем таймер активности для пользовательских запросов паролей
            if (request->hasHeader("X-User-Activity")) {
                resetActivityTimer();
            }
            
            auto passwords = passwordManager.getAllPasswords();
            // Список паролей - увеличенный размер для 50 длинных паролей (до 70 символов каждый)
            JsonDocument doc;
            JsonArray array = doc.to<JsonArray>();
            for (const auto& entry : passwords) {
                JsonObject obj = array.add<JsonObject>();
                obj["name"] = entry.name;
                obj["password"] = entry.password; // Добавляем пароли для корректного отображения в веб-интерфейсе
            }
            String output;
            serializeJson(doc, output);
            
#ifdef SECURE_LAYER_ENABLED
            // КРИТИЧНО: Принудительное шифрование паролей (аналогично TOTP)
            String clientId = WebServerSecureIntegration::getClientId(request);
            bool isTunneled = request->hasHeader("X-Real-Method");
            
            // УСИЛЕННАЯ ПРОВЕРКА: Ищем clientId в разных местах для tunneled запросов
            if (clientId.isEmpty() && isTunneled) {
                // Альтернативные источники clientId при tunneling
                if (request->hasHeader("Authorization")) {
                    String auth = request->getHeader("Authorization")->value();
                    if (auth.startsWith("Bearer ")) {
                        clientId = auth.substring(7);
                    }
                }
                LOG_DEBUG("WebServer", "🚇 TUNNELED clientId recovery attempt: " + 
                         (clientId.length() > 0 ? clientId.substring(0,8) + "..." : "FAILED"));
            }
            
            // Гарантированная проверка secure session для паролей (прямых и tunneled запросов)
            if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId)) {
                LOG_INFO("WebServer", "🔐 PASSWORD ENCRYPTION: Securing passwords data for client " + clientId.substring(0,8) + "..." + (isTunneled ? " [TUNNELED]" : " [DIRECT]"));
                WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                return;
            } else if (clientId.length() > 0) {
                LOG_WARNING("WebServer", "🔐 PASSWORD FALLBACK: No valid secure session for " + clientId.substring(0,8) + "..., sending plaintext");
            } else {
                // ОТЛАДКА: Для tunneled запросов без clientId
                if (isTunneled) {
                    LOG_DEBUG("WebServer", "🔐 PASSWORD FALLBACK: Missing clientId header [TUNNELED REQUEST]");
                }
            }
#endif
            
            request->send(200, "application/json", output);
        }, urlObfuscation);

    // API: Add password (SECURE TESTING ENABLED + URL OBFUSCATION + REQUEST DECRYPTION)
    auto passwordAddHandler = [this](AsyncWebServerRequest *request){
        // Основной обработчик - пустой, вся логика в onBody callback
    };
    
    auto passwordAddBodyHandler = [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (index + len == total) {
            if (!isAuthenticated(request)) return request->send(401);
            if (!verifyCsrfToken(request)) return request->send(403, "text/plain", "CSRF token mismatch");
            
            String name, password;
            
#ifdef SECURE_LAYER_ENABLED
            String clientId = WebServerSecureIntegration::getClientId(request);
            if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId) && 
                (request->hasHeader("X-Secure-Request") || request->hasHeader("X-Security-Level"))) {
                
                LOG_INFO("WebServer", "🔐 PASSWORD ADD: Decrypting request body for " + clientId.substring(0,8) + "...");
                
                // Расшифровываем тело запроса
                String encryptedBody = String((char*)data, len);
                String decryptedBody;
                
                if (secureLayer.decryptRequest(clientId, encryptedBody, decryptedBody)) {
                    LOG_DEBUG("WebServer", "🔐 Decrypted password add body length: " + String(decryptedBody.length()) + " bytes");
                    
                    // Парсим расшифрованные данные (формат: name=MyPassword&password=secret123)
                    int nameStart = decryptedBody.indexOf("name=");
                    int passwordStart = decryptedBody.indexOf("password=");
                    
                    if (nameStart >= 0 && passwordStart >= 0) {
                        // Извлекаем name
                        int nameEnd = decryptedBody.indexOf("&", nameStart);
                        if (nameEnd == -1) nameEnd = decryptedBody.length();
                        name = decryptedBody.substring(nameStart + 5, nameEnd); // skip "name="
                        
                        // Извлекаем password
                        int passwordEnd = decryptedBody.indexOf("&", passwordStart);
                        if (passwordEnd == -1) passwordEnd = decryptedBody.length();
                        password = decryptedBody.substring(passwordStart + 9, passwordEnd); // skip "password="
                        
                        // URL decode
                        name.replace("+", " ");
                        password.replace("+", " ");
                        
                        LOG_DEBUG("WebServer", "🔐 Parsed password add: name=" + name + ", password length=" + String(password.length()));
                    } else {
                        return request->send(400, "text/plain", "Invalid decrypted password add format");
                    }
                } else {
                    LOG_ERROR("WebServer", "🔐 Failed to decrypt password add request body");
                    return request->send(400, "text/plain", "Password add decryption failed");
                }
            } else 
#endif
            {
                // Обычный незашифрованный запрос - читаем параметры
                if (request->hasParam("name", true) && request->hasParam("password", true)) {
                    name = request->getParam("name", true)->value();
                    password = request->getParam("password", true)->value();
                } else {
                    return request->send(400, "text/plain", "Missing required parameters");
                }
            }
            
            if (name.isEmpty() || password.isEmpty()) {
                return request->send(400, "text/plain", "Name and password cannot be empty");
            }
            
            passwordManager.addPassword(name, password);
            
            String response = "Password added successfully!";
            
#ifdef SECURE_LAYER_ENABLED
            String clientId2 = WebServerSecureIntegration::getClientId(request);
            if (clientId2.length() > 0 && secureLayer.isSecureSessionValid(clientId2)) {
                LOG_INFO("WebServer", "🔐 PASSWORD ADD ENCRYPTION: Securing response");
                WebServerSecureIntegration::sendSecureResponse(request, 200, "text/plain", response, secureLayer);
                return;
            }
#endif
            
            request->send(200, "text/plain", response);
        }
    };
    
    // Регистрируем оба варианта: оригинальный и обфусцированный
    server.on("/api/passwords/add", HTTP_POST, passwordAddHandler, NULL, passwordAddBodyHandler);
    String obfuscatedPasswordAddPath = urlObfuscation.obfuscateURL("/api/passwords/add");
    if (obfuscatedPasswordAddPath.length() > 0 && obfuscatedPasswordAddPath != "/api/passwords/add") {
        server.on(obfuscatedPasswordAddPath.c_str(), HTTP_POST, passwordAddHandler, NULL, passwordAddBodyHandler);
    }

    // API: Delete password (SECURE TESTING ENABLED + URL OBFUSCATION + REQUEST DECRYPTION)
    auto passwordDeleteHandler = [this](AsyncWebServerRequest *request){
        // Основной обработчик - пустой, вся логика в onBody callback
    };
    
    auto passwordDeleteBodyHandler = [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (index + len == total) {
            if (!isAuthenticated(request)) return request->send(401);
            if (!verifyCsrfToken(request)) return request->send(403, "text/plain", "CSRF token mismatch");
            
            int passwordIndex = -1;
            
#ifdef SECURE_LAYER_ENABLED
            String clientId = WebServerSecureIntegration::getClientId(request);
            if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId) && 
                (request->hasHeader("X-Secure-Request") || request->hasHeader("X-Security-Level"))) {
                
                LOG_INFO("WebServer", "🔐 PASSWORD DELETE: Decrypting request body for " + clientId.substring(0,8) + "...");
                
                // Расшифровываем тело запроса
                String encryptedBody = String((char*)data, len);
                String decryptedBody;
                
                if (secureLayer.decryptRequest(clientId, encryptedBody, decryptedBody)) {
                    LOG_DEBUG("WebServer", "🔐 Decrypted password delete body length: " + String(decryptedBody.length()) + " bytes");
                    
                    // Парсим расшифрованные данные (формат: index=0)
                    int indexStart = decryptedBody.indexOf("index=");
                    if (indexStart >= 0) {
                        int indexEnd = decryptedBody.indexOf("&", indexStart);
                        if (indexEnd == -1) indexEnd = decryptedBody.length();
                        
                        String indexStr = decryptedBody.substring(indexStart + 6, indexEnd); // skip "index="
                        passwordIndex = indexStr.toInt();
                        
                        LOG_DEBUG("WebServer", "🔐 Parsed password index: " + String(passwordIndex));
                    } else {
                        return request->send(400, "text/plain", "Invalid decrypted password delete format");
                    }
                } else {
                    LOG_ERROR("WebServer", "🔐 Failed to decrypt password delete request body");
                    return request->send(400, "text/plain", "Password delete decryption failed");
                }
            } else 
#endif
            {
                // Обычный незашифрованный запрос - читаем параметры
                if (request->hasParam("index", true)) {
                    passwordIndex = request->getParam("index", true)->value().toInt();
                } else {
                    return request->send(400, "text/plain", "Missing index parameter");
                }
            }
            
            if (passwordIndex < 0) {
                return request->send(400, "text/plain", "Invalid password index");
            }
            
            passwordManager.deletePassword(passwordIndex);
            
            String response = "Password deleted successfully!";
            
#ifdef SECURE_LAYER_ENABLED
            String clientId2 = WebServerSecureIntegration::getClientId(request);
            if (clientId2.length() > 0 && secureLayer.isSecureSessionValid(clientId2)) {
                LOG_INFO("WebServer", "🔐 PASSWORD DELETE ENCRYPTION: Securing response");
                WebServerSecureIntegration::sendSecureResponse(request, 200, "text/plain", response, secureLayer);
                return;
            }
#endif
            
            request->send(200, "text/plain", response);
        }
    };
    
    // Регистрируем оба варианта: оригинальный и обфусцированный
    server.on("/api/passwords/delete", HTTP_POST, passwordDeleteHandler, NULL, passwordDeleteBodyHandler);
    String obfuscatedPasswordDeletePath = urlObfuscation.obfuscateURL("/api/passwords/delete");
    if (obfuscatedPasswordDeletePath.length() > 0 && obfuscatedPasswordDeletePath != "/api/passwords/delete") {
        server.on(obfuscatedPasswordDeletePath.c_str(), HTTP_POST, passwordDeleteHandler, NULL, passwordDeleteBodyHandler);
    }

    // API: Get single password for editing (SECURE TESTING ENABLED + URL OBFUSCATION)
    URLObfuscationIntegration::registerDualEndpoint(server, "/api/passwords/get", HTTP_POST, 
        [this](AsyncWebServerRequest *request){
            if (!isAuthenticated(request)) return request->send(401);
            
            // КРИТИЧНО: Поддержка tunneled запросов без параметров
            int index = 0; // Default index для tunneled запросов
            bool isTunneled = request->hasHeader("X-Real-Method");
            bool hasSecureSession = false;
            
#ifdef SECURE_LAYER_ENABLED
            String clientId = WebServerSecureIntegration::getClientId(request);
            hasSecureSession = (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId));
#endif
            
            if (request->hasParam("index", true)) {
                index = request->getParam("index", true)->value().toInt();
            } else if (request->hasParam("index", false)) {
                index = request->getParam("index", false)->value().toInt();
            } else if (isTunneled || hasSecureSession) {
                // Default index=0 для tunneled или secure запросов без параметров
                String logMessage = "🔧 Secure/Tunneled request missing index parameter, using default index=0";
                if (isTunneled) logMessage += " [TUNNELED]";
                if (hasSecureSession) logMessage += " [SECURE]";
                LOG_INFO("WebServer", logMessage);
                index = 0;
            } else {
                LOG_WARNING("WebServer", "Password get failed: missing index parameter");
                request->send(400, "text/plain", "Index parameter required");
                return;
            }
            
            auto passwords = passwordManager.getAllPasswords();
            if (index >= 0 && index < passwords.size()) {
                    LOG_INFO("WebServer", "🔐 Password retrieved for editing: index " + String(index));
                    
                    // Формируем ответ с чувствительными данными
                    JsonDocument doc;
                    doc["name"] = passwords[index].name;
                    doc["password"] = passwords[index].password;
                    String output;
                    serializeJson(doc, output);
                    
#ifdef SECURE_LAYER_ENABLED
                    // КРИТИЧНО: Принудительное шифрование паролей для tunneled и direct запросов
                    String clientId = WebServerSecureIntegration::getClientId(request);
                    bool isTunneled = request->hasHeader("X-Real-Method");
                    
                    if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId)) {
                        LOG_INFO("WebServer", "🔐 PASSWORD ENCRYPTION: Securing password data for client " + clientId.substring(0,8) + "..." + (isTunneled ? " [TUNNELED]" : " [DIRECT]"));
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        return;
                    } else if (clientId.length() > 0) {
                        LOG_WARNING("WebServer", "🔐 PASSWORD FALLBACK: No valid secure session for " + clientId.substring(0,8) + "..., sending plaintext");
                    }
#endif
                    
                    request->send(200, "application/json", output);
            } else {
                LOG_WARNING("WebServer", "Password get failed: invalid index " + String(index));
                request->send(404, "text/plain", "Password not found");
            }
        }, urlObfuscation);

    // API: Update password (SECURE TESTING ENABLED + URL OBFUSCATION + REQUEST DECRYPTION)
    auto passwordUpdateHandler = [this](AsyncWebServerRequest *request){
        // Основной обработчик - пустой, вся логика в onBody callback
    };
    
    auto passwordUpdateBodyHandler = [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (index + len == total) {
            if (!isAuthenticated(request)) return request->send(401);
            if (!verifyCsrfToken(request)) return request->send(403, "text/plain", "CSRF token mismatch");
            
            int indexVal;
            String name, password;
            
#ifdef SECURE_LAYER_ENABLED
            String clientId = WebServerSecureIntegration::getClientId(request);
            if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId) && 
                (request->hasHeader("X-Secure-Request") || request->hasHeader("X-Security-Level"))) {
                
                LOG_INFO("WebServer", "🔐 PASSWORD UPDATE: Decrypting request body for " + clientId.substring(0,8) + "...");
                
                // Расшифровываем тело запроса
                String encryptedBody = String((char*)data, len);
                String decryptedBody;
                
                if (secureLayer.decryptRequest(clientId, encryptedBody, decryptedBody)) {
                    LOG_DEBUG("WebServer", "🔐 Decrypted password update body length: " + String(decryptedBody.length()) + " bytes");
                    
                    // Парсим расшифрованные данные (формат: index=0&name=test&password=pass)
                    int indexStart = decryptedBody.indexOf("index=");
                    int nameStart = decryptedBody.indexOf("name=");
                    int passwordStart = decryptedBody.indexOf("password=");
                    
                    if (indexStart >= 0 && nameStart >= 0 && passwordStart >= 0) {
                        // Извлекаем index
                        int indexEnd = decryptedBody.indexOf("&", indexStart);
                        if (indexEnd == -1) indexEnd = decryptedBody.length();
                        String indexStr = decryptedBody.substring(indexStart + 6, indexEnd);
                        indexVal = indexStr.toInt();
                        
                        // Извлекаем name
                        int nameEnd = decryptedBody.indexOf("&", nameStart);
                        if (nameEnd == -1) nameEnd = decryptedBody.length();
                        name = decryptedBody.substring(nameStart + 5, nameEnd);
                        
                        // Извлекаем password
                        int passwordEnd = decryptedBody.indexOf("&", passwordStart);
                        if (passwordEnd == -1) passwordEnd = decryptedBody.length();
                        password = decryptedBody.substring(passwordStart + 9, passwordEnd);
                        
                        // URL decode
                        name.replace("+", " ");
                        password.replace("+", " ");
                        
                        LOG_DEBUG("WebServer", "🔐 Parsed: index=" + String(indexVal) + ", name=" + name + ", password=" + password.substring(0, 8) + "...");
                    } else {
                        return request->send(400, "text/plain", "Invalid decrypted password data format");
                    }
                } else {
                    LOG_ERROR("WebServer", "🔐 Failed to decrypt password update request body");
                    return request->send(400, "text/plain", "Password data decryption failed");
                }
            } else 
#endif
            {
                // Обычный незашифрованный запрос
                if (request->hasParam("index", true) && request->hasParam("name", true) && request->hasParam("password", true)) {
                    indexVal = request->getParam("index", true)->value().toInt();
                    name = request->getParam("name", true)->value();
                    password = request->getParam("password", true)->value();
                } else {
                    return request->send(400, "text/plain", "Missing required parameters");
                }
            }
            
            LOG_INFO("WebServer", "Password update requested for entry at index " + String(indexVal));
            
            String response;
            int statusCode;
            
            if (passwordManager.updatePassword(indexVal, name, password)) {
                LOG_INFO("WebServer", "Password entry updated successfully");
                response = "Password updated successfully!";
                statusCode = 200;
            } else {
                LOG_ERROR("WebServer", "Failed to update password entry");
                response = "Failed to update password";
                statusCode = 500;
            }
            
#ifdef SECURE_LAYER_ENABLED
            String clientId2 = WebServerSecureIntegration::getClientId(request);
            if (clientId2.length() > 0 && secureLayer.isSecureSessionValid(clientId2)) {
                LOG_INFO("WebServer", "🔐 PASSWORD UPDATE ENCRYPTION: Securing response");
                WebServerSecureIntegration::sendSecureResponse(request, statusCode, "text/plain", response, secureLayer);
                return;
            }
#endif
            
            request->send(statusCode, "text/plain", response);
        }
    };
    
    // Регистрируем оба варианта: оригинальный и обфусцированный
    server.on("/api/passwords/update", HTTP_POST, passwordUpdateHandler, NULL, passwordUpdateBodyHandler);
    String obfuscatedPasswordUpdatePath = urlObfuscation.obfuscateURL("/api/passwords/update");
    if (obfuscatedPasswordUpdatePath.length() > 0 && obfuscatedPasswordUpdatePath != "/api/passwords/update") {
        server.on(obfuscatedPasswordUpdatePath.c_str(), HTTP_POST, passwordUpdateHandler, NULL, passwordUpdateBodyHandler);
    }
    
    // API: Passwords reordering (SECURE TESTING ENABLED + URL OBFUSCATION)
    URLObfuscationIntegration::registerDualEndpointWithBody(server, "/api/passwords/reorder", HTTP_POST, 
        [this](AsyncWebServerRequest *request){
            if (!isAuthenticated(request)) return request->send(401);
            if (!verifyCsrfToken(request)) return request->send(403, "text/plain", "CSRF token mismatch");
            
            String response = "Passwords reordered successfully!";
            
#ifdef SECURE_LAYER_ENABLED
            // КРИТИЧНО: Принудительное шифрование ответов для password операций
            String clientId = WebServerSecureIntegration::getClientId(request);
            bool isTunneled = request->hasHeader("X-Real-Method");
            
            if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId)) {
                LOG_INFO("WebServer", "🔐 PASSWORD REORDER ENCRYPTION: Securing response for client " + clientId.substring(0,8) + "..." + (isTunneled ? " [TUNNELED]" : " [DIRECT]"));
                WebServerSecureIntegration::sendSecureResponse(request, 200, "text/plain", response, secureLayer);
                return;
            } else if (clientId.length() > 0) {
                LOG_WARNING("WebServer", "🔐 PASSWORD REORDER FALLBACK: No valid secure session for " + clientId.substring(0,8) + "..., sending plaintext");
            }
#endif
            
            request->send(200, "text/plain", response);
        }, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
            if (!isAuthenticated(request)) return;
            
            String body = String((char*)data).substring(0, len);
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, body);
            
            if (error) {
                request->send(400, "text/plain", "Invalid JSON");
                return;
            }
            
            if (!doc["order"].is<JsonArray>()) {
                request->send(400, "text/plain", "Missing or invalid 'order' field");
                return;
            }
            
            std::vector<std::pair<String, int>> newOrder;
            JsonArray orderArray = doc["order"];
            
            for (JsonObject item : orderArray) {
                String name = item["name"];
                int order = item["order"];
                newOrder.push_back(std::make_pair(name, order));
            }
            
            if (!passwordManager.reorderPasswords(newOrder)) {
                String errorResponse = "Failed to reorder passwords";
                
#ifdef SECURE_LAYER_ENABLED
                // КРИТИЧНО: Принудительное шифрование ошибок для password операций
                String clientId = WebServerSecureIntegration::getClientId(request);
                bool isTunneled = request->hasHeader("X-Real-Method");
                
                if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId)) {
                    LOG_INFO("WebServer", "🔐 PASSWORD REORDER ERROR ENCRYPTION: Securing error response for client " + clientId.substring(0,8) + "..." + (isTunneled ? " [TUNNELED]" : " [DIRECT]"));
                    WebServerSecureIntegration::sendSecureResponse(request, 500, "text/plain", errorResponse, secureLayer);
                    return;
                }
#endif
                
                request->send(500, "text/plain", errorResponse);
            }
        }, urlObfuscation);

    // API: Export passwords (SECURE TESTING ENABLED + URL OBFUSCATION + REQUEST DECRYPTION)
    auto passwordExportHandler = [this](AsyncWebServerRequest *request){
        // Основной обработчик - пустой, вся логика в onBody callback
    };
    
    auto passwordExportBodyHandler = [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (index + len == total) {
            if (!isAuthenticated(request)) return request->send(401);
            if (!verifyCsrfToken(request)) return request->send(403, "text/plain", "CSRF token mismatch");
            if (!WebAdminManager::getInstance().isApiEnabled()) {
                LOG_WARNING("WebServer", "Blocked unauthorized attempt to export passwords (API disabled).");
                return request->send(403, "text/plain", "API access for import/export is disabled.");
            }
            
            String password;
            
#ifdef SECURE_LAYER_ENABLED
            String clientId = WebServerSecureIntegration::getClientId(request);
            if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId) && 
                (request->hasHeader("X-Secure-Request") || request->hasHeader("X-Security-Level"))) {
                
                LOG_INFO("WebServer", "🔐 PASSWORD EXPORT: Decrypting request body for " + clientId.substring(0,8) + "...");
                
                // Расшифровываем тело запроса
                String encryptedBody = String((char*)data, len);
                String decryptedBody;
                
                if (secureLayer.decryptRequest(clientId, encryptedBody, decryptedBody)) {
                    LOG_DEBUG("WebServer", "🔐 Decrypted password export body length: " + String(decryptedBody.length()) + " bytes");
                    
                    // Парсим расшифрованные данные (формат: password=adminpass)
                    int passwordStart = decryptedBody.indexOf("password=");
                    if (passwordStart >= 0) {
                        int passwordEnd = decryptedBody.indexOf("&", passwordStart);
                        if (passwordEnd == -1) passwordEnd = decryptedBody.length();
                        
                        password = decryptedBody.substring(passwordStart + 9, passwordEnd); // skip "password="
                        
                        // URL decode пароля (заменяем %XX на символы)
                        password.replace("+", " ");
                        password.replace("%21", "!");
                        password.replace("%40", "@");
                        password.replace("%23", "#");
                        password.replace("%24", "$");
                        password.replace("%25", "%");
                        password.replace("%5E", "^");
                        password.replace("%26", "&");
                        password.replace("%2A", "*");
                        password.replace("%28", "(");
                        password.replace("%29", ")");
                        password.replace("%2D", "-");
                        password.replace("%5F", "_");
                        password.replace("%3D", "=");
                        password.replace("%2B", "+");
                        password.replace("%5B", "[");
                        password.replace("%5D", "]");
                        password.replace("%7B", "{");
                        password.replace("%7D", "}");
                        password.replace("%5C", "\\");
                        password.replace("%7C", "|");
                        password.replace("%3B", ";");
                        password.replace("%3A", ":");
                        password.replace("%27", "'");
                        password.replace("%22", "\"");
                        password.replace("%3C", "<");
                        password.replace("%3E", ">");
                        password.replace("%2C", ",");
                        password.replace("%2E", ".");
                        password.replace("%3F", "?");
                        password.replace("%2F", "/");
                        
                        LOG_DEBUG("WebServer", "🔐 Parsed and URL-decoded admin password for password export");
                    } else {
                        return request->send(400, "text/plain", "Invalid decrypted password export format");
                    }
                } else {
                    LOG_ERROR("WebServer", "🔐 Failed to decrypt password export request body");
                    return request->send(400, "text/plain", "Password export decryption failed");
                }
            } else 
#endif
            {
                // Обычный незашифрованный запрос - читаем параметры
                if (!request->hasParam("password", true)) {
                    return request->send(400, "text/plain", "Password is required for export.");
                }
                password = request->getParam("password", true)->value();
            }
            
            if (password.isEmpty()) {
                return request->send(400, "text/plain", "Password cannot be empty");
            }
            
            if (!WebAdminManager::getInstance().verifyCredentials(WebAdminManager::getInstance().getUsername(), password)) {
                LOG_WARNING("WebServer", "Password export failed: Invalid admin password provided.");
                return request->send(401, "text/plain", "Invalid admin password.");
            }

            LOG_INFO("WebServer", "Password verified. Starting password export process.");
            auto passwords = passwordManager.getAllPasswordsForExport();
            
            JsonDocument doc;
            JsonArray array = doc.to<JsonArray>();
            for (const auto& entry : passwords) {
                JsonObject obj = array.add<JsonObject>();
                obj["name"] = entry.name;
                obj["password"] = entry.password;
            }
            String plaintext;
            serializeJson(doc, plaintext);

            String encryptedContent = CryptoManager::getInstance().encryptWithPassword(plaintext, password);

            // 🔐 ВАЖНО: Не используем sendSecureResponse для файлов - контент уже зашифрован CryptoManager
            LOG_INFO("WebServer", "🔐 PASSWORD EXPORT: Sending encrypted file (pre-encrypted by CryptoManager)");
            AsyncWebServerResponse *response = request->beginResponse(200, "application/json", encryptedContent);
            response->addHeader("Content-Disposition", "attachment; filename=\"encrypted_passwords_backup.json\"");
            request->send(response);
        }
    };
    
    // Регистрируем оба варианта: оригинальный и обфусцированный
    server.on("/api/passwords/export", HTTP_POST, passwordExportHandler, NULL, passwordExportBodyHandler);
    String obfuscatedPasswordExportPath = urlObfuscation.obfuscateURL("/api/passwords/export");
    if (obfuscatedPasswordExportPath.length() > 0 && obfuscatedPasswordExportPath != "/api/passwords/export") {
        server.on(obfuscatedPasswordExportPath.c_str(), HTTP_POST, passwordExportHandler, NULL, passwordExportBodyHandler);
    }

    // API: Import passwords (SECURE TESTING ENABLED + URL OBFUSCATION)
    URLObfuscationIntegration::registerDualEndpointWithBody(server, "/api/passwords/import", HTTP_POST, 
        [this](AsyncWebServerRequest *request) {
            // Empty handler, body handler does the work
        }, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (!isAuthenticated(request)) {
                if (index == 0) request->send(401);
                return;
            }
            if (!WebAdminManager::getInstance().isApiEnabled()) {
                if (index == 0) {
                    LOG_WARNING("WebServer", "Blocked unauthorized attempt to import passwords (API disabled).");
                    request->send(403, "text/plain", "API access for import/export is disabled.");
                }
                return;
            }

            static String body;
            if (index == 0) body = "";
            body.concat((char*)data, len);

            if (index + len >= total) {
                LOG_INFO("WebServer", "Received passwords import data.");
                
                String finalBody = body;
                String clientId; // Объявляем заранее для использования в нескольких блоках
                
#ifdef SECURE_LAYER_ENABLED
                // 🔐 Проверяем XOR зашифрованный запрос
                clientId = WebServerSecureIntegration::getClientId(request);
                if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId) && 
                    (request->hasHeader("X-Secure-Request") || request->hasHeader("X-Security-Level"))) {
                    
                    LOG_INFO("WebServer", "🔐 PASSWORDS IMPORT: Decrypting XOR request body for " + clientId.substring(0,8) + "...");
                    
                    // Расшифровываем XOR тело запроса
                    String decryptedBody;
                    if (secureLayer.decryptRequest(clientId, body, decryptedBody)) {
                        LOG_DEBUG("WebServer", "🔐 XOR decrypted passwords import body length: " + String(decryptedBody.length()) + " bytes");
                        finalBody = decryptedBody;
                    } else {
                        LOG_ERROR("WebServer", "🔐 Failed to XOR decrypt passwords import request body");
                        request->send(400, "text/plain", "Passwords import XOR decryption failed");
                        return;
                    }
                }
#endif
                
                JsonDocument doc;
                if (deserializeJson(doc, finalBody) != DeserializationError::Ok) {
                    request->send(400, "text/plain", "Invalid JSON body.");
                    return;
                }

                String password = doc["password"];
                String fileContent = doc["data"];

                if (password.isEmpty() || fileContent.isEmpty()) {
                    request->send(400, "text/plain", "Missing password or file data.");
                    return;
                }

                String decryptedContent = CryptoManager::getInstance().decryptWithPassword(fileContent, password);

                if (decryptedContent.isEmpty()) {
                    LOG_WARNING("WebServer", "Failed to decrypt imported passwords. Wrong password or corrupt file.");
                    
                    String errorResponse = "Decryption failed. Wrong password or corrupt file.";
                    
#ifdef SECURE_LAYER_ENABLED
                    // КРИТИЧНО: Принудительное шифрование ошибок для password операций
                    String clientId = WebServerSecureIntegration::getClientId(request);
                    bool isTunneled = request->hasHeader("X-Real-Method");
                    
                    if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId)) {
                        LOG_INFO("WebServer", "🔐 PASSWORD IMPORT ERROR ENCRYPTION: Securing error response for client " + clientId.substring(0,8) + "..." + (isTunneled ? " [TUNNELED]" : " [DIRECT]"));
                        WebServerSecureIntegration::sendSecureResponse(request, 400, "text/plain", errorResponse, secureLayer);
                        return;
                    }
#endif
                    
                    request->send(400, "text/plain", errorResponse);
                    return;
                }

                String response;
                int statusCode;
                
                if (passwordManager.replaceAllPasswords(decryptedContent)) {
                    LOG_INFO("WebServer", "Passwords imported successfully.");
                    response = "Import successful!";
                    statusCode = 200;
                } else {
                    LOG_ERROR("WebServer", "Failed to process imported passwords after decryption.");
                    response = "Failed to process passwords after decryption.";
                    statusCode = 500;
                }
                
#ifdef SECURE_LAYER_ENABLED
                // 🔐 Переиспользуем clientId из блока расшифровки выше
                if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId)) {
                    LOG_INFO("WebServer", "🔐 PASSWORD IMPORT ENCRYPTION: Securing response");
                    WebServerSecureIntegration::sendSecureResponse(request, statusCode, "text/plain", response, secureLayer);
                    return;
                }
#endif
                
                request->send(statusCode, "text/plain", response);
            }
        }, urlObfuscation);

    // --- API для управления доступом к импорту/экспорту ---
    server.on("/api/enable_import_export", HTTP_POST, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        if (!verifyCsrfToken(request)) return request->send(403, "text/plain", "CSRF token mismatch");
        WebAdminManager::getInstance().enableApi();
        request->send(200, "text/plain", "API enabled for 5 minutes.");
    });

    server.on("/api/import_export_status", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        auto& adminManager = WebAdminManager::getInstance();
        // Статус API - небольшой размер
        JsonDocument doc;
        doc["enabled"] = adminManager.isApiEnabled();
        doc["timeLeft"] = adminManager.getApiTimeRemaining();
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });
    
    // API: Public client bootstrap config - minimal pre-auth mappings
    server.on("/api/client/config", HTTP_GET, [this](AsyncWebServerRequest *request){
        JsonDocument doc;
        doc["k"] = urlObfuscation.obfuscateURL("/api/secure/keyexchange");
        doc["t"] = urlObfuscation.obfuscateURL("/api/tunnel");
        doc["l"] = urlObfuscation.obfuscateURL("/login");
        
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // API: Change admin password (POST with encryption)
    server.on("/api/change_password", HTTP_POST,
        [this](AsyncWebServerRequest *request){
            // Главный обработчик - вызывается первым
            LOG_INFO("WebServer", "🔐 CHANGE_PASSWORD: Main handler called");
        },
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            // Защита от краша
            if (!data || len == 0) {
                LOG_ERROR("WebServer", "🔐 CHANGE_PASSWORD: Invalid data pointer or length");
                return request->send(400, "text/plain", "Invalid request data");
            }
            
            LOG_INFO("WebServer", "🔐 CHANGE_PASSWORD: onBody called - index=" + String(index) + " len=" + String(len) + " total=" + String(total));
            
            if (index + len == total) {
                if (!isAuthenticated(request)) {
                    return request->send(401, "text/plain", "Unauthorized");
                }
                if (!verifyCsrfToken(request)) {
                    return request->send(403, "text/plain", "CSRF token mismatch");
                }
                
                String newPassword;
                
#ifdef SECURE_LAYER_ENABLED
                String clientId = WebServerSecureIntegration::getClientId(request);
                
                if (clientId.length() > 0 && 
                    secureLayer.isSecureSessionValid(clientId) &&
                    (request->hasHeader("X-Secure-Request") || 
                     request->hasHeader("X-Security-Level"))) {
                    
                    LOG_INFO("WebServer", "🔐 CHANGE_PASSWORD: Decrypting " + String(len) + "b for " + clientId.substring(0,8) + "...");
                    
                    // Расшифровка запроса
                    String encryptedBody = String((char*)data, len);
                    String decryptedBody;
                    
                    if (secureLayer.decryptRequest(clientId, encryptedBody, decryptedBody)) {
                        LOG_DEBUG("WebServer", "🔐 CHANGE_PASSWORD: Decrypted successfully");
                        // Парсинг параметра password
                        int passwordStart = decryptedBody.indexOf("password=");
                        if (passwordStart >= 0) {
                            int passwordEnd = decryptedBody.indexOf("&", passwordStart);
                            if (passwordEnd < 0) passwordEnd = decryptedBody.length();
                            newPassword = decryptedBody.substring(passwordStart + 9, passwordEnd);
                            
                            // URL decode (полный decode всех спецсимволов)
                            newPassword = urlDecode(newPassword);
                            LOG_DEBUG("WebServer", "🔐 Decoded password length: " + String(newPassword.length()));
                        }
                    } else {
                        return request->send(400, "text/plain", "Decryption failed");
                    }
                } else
#endif
                {
                    // Fallback: незашифрованный запрос
                    if (request->hasParam("password", true)) {
                        newPassword = request->getParam("password", true)->value();
                    }
                }
                
                // Валидация пароля
                if (newPassword.length() == 0) {
                    return request->send(400, "text/plain", "Password parameter missing.");
                }
                if (newPassword.length() < 4) {
                    return request->send(400, "text/plain", "Password must be at least 4 characters long.");
                }
                
                // Смена пароля
                String response;
                if (WebAdminManager::getInstance().changePassword(newPassword)) {
                    response = "Password changed successfully!";
                } else {
                    return request->send(500, "text/plain", "Failed to save new password.");
                }
                
                // Отправка зашифрованного ответа
#ifdef SECURE_LAYER_ENABLED
                String clientId2 = WebServerSecureIntegration::getClientId(request);
                if (clientId2.length() > 0 && 
                    secureLayer.isSecureSessionValid(clientId2)) {
                    WebServerSecureIntegration::sendSecureResponse(
                        request, 200, "text/plain", response, secureLayer);
                    return;
                }
#endif
                // Fallback: незашифрованный ответ
                request->send(200, "text/plain", response);
            }
        });

    server.on("/api/wifi_credentials", HTTP_POST,
        [this](AsyncWebServerRequest *request){
            // Main handler — always sends response
            if (!isAuthenticated(request)) return request->send(401);
            if (!verifyCsrfToken(request)) return request->send(403);
            
            auto* parsed = (JsonDocument*)request->_tempObject;
            if (!parsed) {
                return request->send(400, "application/json",
                    "{\"success\":false,\"message\":\"No body\"}");
            }
            
            String ssid = (*parsed)["ssid"].as<String>();
            String password = (*parsed)["password"].as<String>();
            String confirm = (*parsed)["confirm_password"].as<String>();
            
            delete parsed; request->_tempObject = nullptr;
            
            if (ssid.length() == 0) {
                return request->send(400, "application/json",
                    "{\"success\":false,\"message\":\"SSID is required\"}");
            }
            if (password != confirm) {
                return request->send(400, "application/json",
                    "{\"success\":false,\"message\":\"Passwords do not match\"}");
            }
            if (password.length() > 0 && password.length() < 8) {
                return request->send(400, "application/json",
                    "{\"success\":false,\"message\":\"Password must be at least 8 characters\"}");
            }
            
            if (wifiManager && wifiManager->saveCredentials(ssid, password)) {
                request->send(200, "application/json",
                    "{\"success\":true,\"message\":\"WiFi credentials saved. Reboot to apply.\"}");
            } else {
                request->send(500, "application/json",
                    "{\"success\":false,\"message\":\"Failed to save WiFi credentials\"}");
            }
        },
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
            // Body handler — NEVER calls send()
            if (index + len < total) return;
            
            JsonDocument* doc = new JsonDocument();
            if (deserializeJson(*doc, data, len) == DeserializationError::Ok) {
                request->_tempObject = doc;
            } else {
                delete doc;
            }
        });

    // API: Change WiFi AP password (POST with encryption)
    server.on("/api/change_ap_password", HTTP_POST,
        [this](AsyncWebServerRequest *request){
            // Главный обработчик - вызывается первым
            LOG_INFO("WebServer", "🔐 CHANGE_AP_PASSWORD: Main handler called");
        },
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            // Защита от краша
            if (!data || len == 0) {
                LOG_ERROR("WebServer", "🔐 CHANGE_AP_PASSWORD: Invalid data pointer or length");
                return request->send(400, "text/plain", "Invalid request data");
            }
            
            LOG_INFO("WebServer", "🔐 CHANGE_AP_PASSWORD: onBody called - index=" + String(index) + " len=" + String(len) + " total=" + String(total));
            
            if (index + len == total) {
                if (!isAuthenticated(request)) {
                    return request->send(401, "text/plain", "Unauthorized");
                }
                if (!verifyCsrfToken(request)) {
                    return request->send(403, "text/plain", "CSRF token mismatch");
                }
                
                String newPassword;
                
#ifdef SECURE_LAYER_ENABLED
                String clientId = WebServerSecureIntegration::getClientId(request);
                
                if (clientId.length() > 0 && 
                    secureLayer.isSecureSessionValid(clientId) &&
                    (request->hasHeader("X-Secure-Request") || 
                     request->hasHeader("X-Security-Level"))) {
                    
                    LOG_INFO("WebServer", "🔐 CHANGE_AP_PASSWORD: Decrypting " + String(len) + "b for " + clientId.substring(0,8) + "...");
                    
                    // Расшифровка запроса
                    String encryptedBody = String((char*)data, len);
                    String decryptedBody;
                    
                    if (secureLayer.decryptRequest(clientId, encryptedBody, decryptedBody)) {
                        LOG_DEBUG("WebServer", "🔐 CHANGE_AP_PASSWORD: Decrypted successfully");
                        // Парсинг параметра password
                        int passwordStart = decryptedBody.indexOf("password=");
                        if (passwordStart >= 0) {
                            int passwordEnd = decryptedBody.indexOf("&", passwordStart);
                            if (passwordEnd < 0) passwordEnd = decryptedBody.length();
                            newPassword = decryptedBody.substring(passwordStart + 9, passwordEnd);
                            
                            // URL decode (полный decode всех спецсимволов)
                            newPassword = urlDecode(newPassword);
                            LOG_DEBUG("WebServer", "🔐 Decoded password length: " + String(newPassword.length()));
                        }
                    } else {
                        return request->send(400, "text/plain", "Decryption failed");
                    }
                } else
#endif
                {
                    // Fallback: незашифрованный запрос
                    if (request->hasParam("password", true)) {
                        newPassword = request->getParam("password", true)->value();
                    }
                }
                
                // Валидация пароля
                if (newPassword.length() == 0) {
                    return request->send(400, "text/plain", "Password parameter missing.");
                }
                if (newPassword.length() < 8) {
                    return request->send(400, "text/plain", "WiFi password must be at least 8 characters long.");
                }
                
                // Смена AP пароля
                String response;
                if (configManager.saveApPassword(newPassword)) {
                    response = "{\"success\":true,\"message\":\"WiFi AP password changed successfully!\"}";
                } else {
                    return request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save new AP password.\"}");
                }
                
                // Отправка зашифрованного ответа
#ifdef SECURE_LAYER_ENABLED
                String clientId2 = WebServerSecureIntegration::getClientId(request);
                if (clientId2.length() > 0 && 
                    secureLayer.isSecureSessionValid(clientId2)) {
                    WebServerSecureIntegration::sendSecureResponse(
                        request, 200, "text/plain", response, secureLayer);
                    return;
                }
#endif
                // Fallback: незашифрованный ответ
                request->send(200, "text/plain", response);
            }
        });

    // API: Get session duration settings (GET with encryption)
    server.on("/api/session_duration", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        
        ConfigManager::SessionDuration duration = configManager.getSessionDuration();
        JsonDocument doc;
        doc["duration"] = static_cast<int>(duration);
        
        String output;
        serializeJson(doc, output);
        
#ifdef SECURE_LAYER_ENABLED
        String clientId = WebServerSecureIntegration::getClientId(request);
        if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId)) {
            LOG_INFO("WebServer", "🔐 SESSION_DURATION GET: Securing response for " + clientId.substring(0,8) + "...");
            WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
            return;
        }
#endif
        request->send(200, "application/json", output);
    });

    // API: Set session duration settings (POST with encryption)
    server.on("/api/session_duration", HTTP_POST,
        [this](AsyncWebServerRequest *request){
            // Пустой основной обработчик
        },
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index + len == total) {
                if (!isAuthenticated(request)) {
                    return request->send(401, "text/plain", "Unauthorized");
                }
                if (!verifyCsrfToken(request)) {
                    return request->send(403, "text/plain", "CSRF token mismatch");
                }
                
                String durationStr;
                
#ifdef SECURE_LAYER_ENABLED
                String clientId = WebServerSecureIntegration::getClientId(request);
                
                if (clientId.length() > 0 && 
                    secureLayer.isSecureSessionValid(clientId) &&
                    (request->hasHeader("X-Secure-Request") || 
                     request->hasHeader("X-Security-Level"))) {
                    
                    // Расшифровка запроса
                    String encryptedBody = String((char*)data, len);
                    String decryptedBody;
                    
                    if (secureLayer.decryptRequest(clientId, encryptedBody, decryptedBody)) {
                        // Парсинг параметра duration
                        int durationStart = decryptedBody.indexOf("duration=");
                        if (durationStart >= 0) {
                            int durationEnd = decryptedBody.indexOf("&", durationStart);
                            if (durationEnd < 0) durationEnd = decryptedBody.length();
                            durationStr = decryptedBody.substring(durationStart + 9, durationEnd);
                        }
                    } else {
                        return request->send(400, "text/plain", "Decryption failed");
                    }
                } else
#endif
                {
                    // Fallback: незашифрованный запрос
                    if (request->hasParam("duration", true)) {
                        durationStr = request->getParam("duration", true)->value();
                    }
                }
                
                // Валидация
                if (durationStr.length() == 0) {
                    return request->send(400, "text/plain", "Duration parameter missing.");
                }
                
                int durationValue = durationStr.toInt();
                
                // Validate duration
                if (durationValue == 0 || durationValue == 1 || durationValue == 6 || 
                    durationValue == 24 || durationValue == 72) {
                    
                    ConfigManager::SessionDuration duration = static_cast<ConfigManager::SessionDuration>(durationValue);
                    configManager.setSessionDuration(duration);
                    
                    // Формирование JSON ответа
                    JsonDocument doc;
                    doc["success"] = true;
                    doc["message"] = "Session duration updated successfully!";
                    String response;
                    serializeJson(doc, response);
                    
                    // Отправка зашифрованного ответа
#ifdef SECURE_LAYER_ENABLED
                    String clientId2 = WebServerSecureIntegration::getClientId(request);
                    if (clientId2.length() > 0 && 
                        secureLayer.isSecureSessionValid(clientId2)) {
                        WebServerSecureIntegration::sendSecureResponse(
                            request, 200, "application/json", response, secureLayer);
                        return;
                    }
#endif
                    // Fallback
                    request->send(200, "application/json", response);
                } else {
                    request->send(400, "text/plain", "Invalid session duration value.");
                }
            }
        });

    // 🔒 SECURITY: OLD custom splash API endpoints /api/upload_splash and /api/delete_splash REMOVED
    // Custom splash upload feature disabled for security - only embedded splash screens supported

    server.on("/api/export", HTTP_POST, [this](AsyncWebServerRequest *request){
        // Основной обработчик - пустой, вся логика в onBody callback
    }, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        // onBody callback - обрабатывает тело запроса для расшифровки пароля
        if (index + len == total) {
            if (!isAuthenticated(request)) return request->send(401);
            if (!verifyCsrfToken(request)) return request->send(403, "text/plain", "CSRF token mismatch");
            if (!WebAdminManager::getInstance().isApiEnabled()) {
                LOG_WARNING("WebServer", "Blocked unauthorized attempt to export TOTP keys (API disabled).");
                return request->send(403, "text/plain", "API access for import/export is disabled.");
            }
            
            String password;
            
#ifdef SECURE_LAYER_ENABLED
            String clientId = WebServerSecureIntegration::getClientId(request);
            if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId) && 
                (request->hasHeader("X-Secure-Request") || request->hasHeader("X-Security-Level"))) {
                
                LOG_INFO("WebServer", "🔐 EXPORT: Decrypting request body for " + clientId.substring(0,8) + "...");
                
                // Расшифровываем тело запроса
                String encryptedBody = String((char*)data, len);
                String decryptedBody;
                
                if (secureLayer.decryptRequest(clientId, encryptedBody, decryptedBody)) {
                    LOG_DEBUG("WebServer", "🔐 Decrypted export body length: " + String(decryptedBody.length()) + " bytes");
                    
                    // Парсим расшифрованные данные (формат: password=adminpass)
                    int passwordStart = decryptedBody.indexOf("password=");
                    if (passwordStart >= 0) {
                        int passwordEnd = decryptedBody.indexOf("&", passwordStart);
                        if (passwordEnd == -1) passwordEnd = decryptedBody.length();
                        
                        password = decryptedBody.substring(passwordStart + 9, passwordEnd); // skip "password="
                        
                        // URL decode пароля (заменяем %XX на символы)
                        password.replace("+", " ");
                        password.replace("%21", "!");
                        password.replace("%40", "@");
                        password.replace("%23", "#");
                        password.replace("%24", "$");
                        password.replace("%25", "%");
                        password.replace("%5E", "^");
                        password.replace("%26", "&");
                        password.replace("%2A", "*");
                        password.replace("%28", "(");
                        password.replace("%29", ")");
                        password.replace("%2D", "-");
                        password.replace("%5F", "_");
                        password.replace("%3D", "=");
                        password.replace("%2B", "+");
                        password.replace("%5B", "[");
                        password.replace("%5D", "]");
                        password.replace("%7B", "{");
                        password.replace("%7D", "}");
                        password.replace("%5C", "\\");
                        password.replace("%7C", "|");
                        password.replace("%3B", ";");
                        password.replace("%3A", ":");
                        password.replace("%27", "'");
                        password.replace("%22", "\"");
                        password.replace("%3C", "<");
                        password.replace("%3E", ">");
                        password.replace("%2C", ",");
                        password.replace("%2E", ".");
                        password.replace("%3F", "?");
                        password.replace("%2F", "/");
                        
                        LOG_DEBUG("WebServer", "🔐 Parsed and URL-decoded admin password for TOTP export");
                    } else {
                        return request->send(400, "text/plain", "Invalid decrypted export format");
                    }
                } else {
                    LOG_ERROR("WebServer", "🔐 Failed to decrypt export request body");
                    return request->send(400, "text/plain", "Export decryption failed");
                }
            } else 
#endif
            {
                // Обычный незашифрованный запрос - читаем параметры
                if (!request->hasParam("password", true)) {
                    return request->send(400, "text/plain", "Password is required for export.");
                }
                password = request->getParam("password", true)->value();
            }
            
            if (password.isEmpty()) {
                return request->send(400, "text/plain", "Password cannot be empty");
            }
            
            if (!WebAdminManager::getInstance().verifyCredentials(WebAdminManager::getInstance().getUsername(), password)) {
                LOG_WARNING("WebServer", "Export failed: Invalid admin password provided.");
                return request->send(401, "text/plain", "Invalid admin password.");
            }

            LOG_INFO("WebServer", "Password verified. Starting TOTP keys export process.");
            auto keys = keyManager.getAllKeys();
            
            JsonDocument doc;
            JsonArray array = doc.to<JsonArray>();
            for (const auto& key : keys) {
                JsonObject obj = array.add<JsonObject>();
                obj["name"] = key.name;
                obj["secret"] = key.secret;
                // Добавляем расширенные метаданные
                obj["type"] = (key.type == TOTPType::HOTP) ? "H" : "T";
                obj["algorithm"] = (key.algorithm == TOTPAlgorithm::SHA256) ? "256" :
                                   (key.algorithm == TOTPAlgorithm::SHA512) ? "512" : "1";
                obj["digits"] = key.digits;
                obj["period"] = key.period;
                if (key.counter != 0) {
                    obj["counter"] = key.counter;
                }
            }
            String plaintext;
            serializeJson(doc, plaintext);

            String encryptedContent = CryptoManager::getInstance().encryptWithPassword(plaintext, password);
            
            // 🔐 ВАЖНО: Не используем sendSecureResponse для файлов - контент уже зашифрован CryptoManager
            LOG_INFO("WebServer", "🔐 TOTP EXPORT: Sending encrypted file (pre-encrypted by CryptoManager)");
            AsyncWebServerResponse *response = request->beginResponse(200, "application/json", encryptedContent);
            response->addHeader("Content-Disposition", "attachment; filename=\"encrypted_keys_backup.json\"");
            request->send(response);
        }
    });

    server.on("/api/import", HTTP_POST, [this](AsyncWebServerRequest *request) {
        // This handler is intentionally left empty because the body handler below does all the work.
        // We just need to ensure the endpoint exists.
    }, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (!isAuthenticated(request)) {
            if (index == 0) request->send(401);
            return;
        }
        if (!WebAdminManager::getInstance().isApiEnabled()) {
            if (index == 0) {
                LOG_WARNING("WebServer", "Blocked unauthorized attempt to import TOTP keys (API disabled).");
                request->send(403, "text/plain", "API access for import/export is disabled.");
            }
            return;
        }

        static String body;
        if (index == 0) body = "";
        body.concat((char*)data, len);

        if (index + len >= total) {
            LOG_INFO("WebServer", "Received TOTP keys import data.");
            
            String finalBody = body;
            
#ifdef SECURE_LAYER_ENABLED
            // 🔐 Проверяем XOR зашифрованный запрос
            String clientId = WebServerSecureIntegration::getClientId(request);
            if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId) && 
                (request->hasHeader("X-Secure-Request") || request->hasHeader("X-Security-Level"))) {
                
                LOG_INFO("WebServer", "🔐 IMPORT: Decrypting XOR request body for " + clientId.substring(0,8) + "...");
                
                // Расшифровываем XOR тело запроса
                String decryptedBody;
                if (secureLayer.decryptRequest(clientId, body, decryptedBody)) {
                    LOG_DEBUG("WebServer", "🔐 XOR decrypted import body length: " + String(decryptedBody.length()) + " bytes");
                    finalBody = decryptedBody;
                } else {
                    LOG_ERROR("WebServer", "🔐 Failed to XOR decrypt import request body");
                    request->send(400, "text/plain", "Import XOR decryption failed");
                    return;
                }
            }
#endif
            
            JsonDocument doc;
            if (deserializeJson(doc, finalBody) != DeserializationError::Ok) {
                request->send(400, "text/plain", "Invalid JSON body.");
                return;
            }

            String password = doc["password"];
            String fileContent = doc["data"];

            if (password.isEmpty() || fileContent.isEmpty()) {
                request->send(400, "text/plain", "Missing password or file data.");
                return;
            }

            String decryptedContent = CryptoManager::getInstance().decryptWithPassword(fileContent, password);

            if (decryptedContent.isEmpty()) {
                LOG_WARNING("WebServer", "Failed to decrypt imported TOTP keys. Wrong password or corrupt file.");
                request->send(400, "text/plain", "Decryption failed. Wrong password or corrupt file.");
                return;
            }

            if (keyManager.replaceAllKeys(decryptedContent)) {
                LOG_INFO("WebServer", "TOTP keys imported successfully.");
                
                // Формируем JSON ответ с подтверждением
                JsonDocument responseDoc;
                responseDoc["status"] = "success";
                responseDoc["message"] = "Import successful!";
                String jsonResponse;
                serializeJson(responseDoc, jsonResponse);
                
#ifdef SECURE_LAYER_ENABLED
                // КРИТИЧНО: Принудительное шифрование для import операций
                String clientId = WebServerSecureIntegration::getClientId(request);
                bool isTunneled = request->hasHeader("X-Real-Method");
                
                if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId)) {
                    LOG_INFO("WebServer", "🔐 IMPORT ENCRYPTION: Securing import response for client " + clientId.substring(0,8) + "..." + (isTunneled ? " [TUNNELED]" : " [DIRECT]"));
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", jsonResponse, secureLayer);
                    return;
                } else if (clientId.length() > 0) {
                    LOG_WARNING("WebServer", "🔐 IMPORT FALLBACK: No valid secure session for " + clientId.substring(0,8) + "..., sending plaintext");
                } else if (isTunneled) {
                    LOG_DEBUG("WebServer", "🔐 IMPORT FALLBACK: Missing clientId header [TUNNELED REQUEST]");
                }
#endif
                
                request->send(200, "application/json", jsonResponse);
            } else {
                LOG_ERROR("WebServer", "Failed to process imported TOTP keys after decryption.");
                
                // Формируем JSON ответ с ошибкой
                JsonDocument errorDoc;
                errorDoc["status"] = "error";
                errorDoc["message"] = "Failed to process keys after decryption.";
                String errorResponse;
                serializeJson(errorDoc, errorResponse);
                
#ifdef SECURE_LAYER_ENABLED
                // Шифруем также ответы с ошибками
                String clientId = WebServerSecureIntegration::getClientId(request);
                if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId)) {
                    WebServerSecureIntegration::sendSecureResponse(request, 500, "application/json", errorResponse, secureLayer);
                    return;
                }
#endif
                
                request->send(500, "application/json", errorResponse);
            }
        }
    });

    server.on("/api/pincode_settings", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        
        // Сбрасываем таймер активности для пользовательских запросов PIN настроек
        if (request->hasHeader("X-User-Activity")) {
            resetActivityTimer();
        }
        
        pinManager.loadPinConfig();
        
        // Проверяем текущее состояние device key
        bool pinProtectionEnabled = CryptoManager::getInstance().isDeviceKeyEncrypted();
        bool deviceKeyExists = CryptoManager::getInstance().isDeviceKeyFileExists();
        
        // Проверяем статус Device BLE PIN
        bool deviceBlePinEnabled = CryptoManager::getInstance().isDeviceBlePinEnabled();
        bool deviceBlePinConfigured = CryptoManager::getInstance().isDeviceBlePinConfigured();
        
        LOG_INFO("WebServer", "GET /api/pincode_settings: deviceBlePinConfigured=" + String(deviceBlePinConfigured));
        
        JsonDocument doc;
        doc["pinProtectionEnabled"] = pinProtectionEnabled;
        doc["deviceKeyExists"] = deviceKeyExists;
        doc["length"] = pinManager.getPinLength();
        
        // Device BLE PIN status
        doc["deviceBlePinEnabled"] = deviceBlePinEnabled;
        doc["deviceBlePinConfigured"] = deviceBlePinConfigured;
        
        // Legacy fields для обратной совместимости
        doc["enabled"] = pinProtectionEnabled;
        doc["enabledForDevice"] = pinProtectionEnabled;
        
        String response;
        serializeJson(doc, response);
        
#ifdef SECURE_LAYER_ENABLED
        // 🔐 КРИТИЧНО: Принудительное шифрование PIN настроек (чувствительные данные)
        String clientId = WebServerSecureIntegration::getClientId(request);
        bool isTunneled = request->hasHeader("X-Real-Method");
        
        // УСИЛЕННАЯ ПРОВЕРКА: Ищем clientId в разных местах для tunneled запросов
        if (clientId.isEmpty() && isTunneled) {
            // Альтернативные источники clientId при tunneling
            if (request->hasHeader("Authorization")) {
                String auth = request->getHeader("Authorization")->value();
                if (auth.startsWith("Bearer ")) {
                    clientId = auth.substring(7);
                }
            }
            LOG_DEBUG("WebServer", "🚇 TUNNELED clientId recovery attempt: " + 
                     (clientId.length() > 0 ? clientId.substring(0,8) + "..." : "FAILED"));
        }
        
        // Гарантированная проверка secure session для PIN настроек (прямых и tunneled запросов)
        if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId)) {
            LOG_INFO("WebServer", "🔐 PIN SETTINGS ENCRYPTION: Securing settings data for client " + clientId.substring(0,8) + "..." + (isTunneled ? " [TUNNELED]" : " [DIRECT]"));
            WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", response, secureLayer);
            return;
        } else if (clientId.length() > 0) {
            LOG_WARNING("WebServer", "🔐 PIN SETTINGS FALLBACK: No valid secure session for " + clientId.substring(0,8) + "..., sending plaintext");
        }
#endif
        
        // Fallback: отправка без шифрования если secure layer не активен
        request->send(200, "application/json", response);
    });

    // PIN settings POST endpoint с onBody callback (как passwords/update)
    auto pinSettingsHandler = [this](AsyncWebServerRequest *request){
        // Основной обработчик - пустой, вся логика в onBody callback
        LOG_DEBUG("WebServer", "🔐 PIN SETTINGS: Main handler called, method=" + String(request->methodToString()));
    };
    
    auto pinSettingsBodyHandler = [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        LOG_DEBUG("WebServer", "🔐 PIN SETTINGS BODY: len=" + String(len) + ", index=" + String(index) + ", total=" + String(total));
        
        if (index + len == total) {
            LOG_INFO("WebServer", "🔐 PIN SETTINGS: Processing complete body, size=" + String(total));
            if (!isAuthenticated(request)) return request->send(401);
            if (!verifyCsrfToken(request)) return request->send(403, "text/plain", "CSRF token mismatch");
            
            // 🔐 ОБРАБОТКА ЗАШИФРОВАННЫХ И ОБЫЧНЫХ ПАРАМЕТРОВ
            bool enablePinProtection = false;
            int pinLength = DEFAULT_PIN_LENGTH;
            String newPin = "";
            String confirmPin = "";
            String currentPin = ""; // Для отключения PIN
            bool factoryResetConfirmed = false; // Для подтверждения factory reset
            bool isEncrypted = false;
            
            String clientId = WebServerSecureIntegration::getClientId(request);
            
            // Попытка расшифровать входящие данные из тела запроса
            if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId) && len > 0) {
                String encryptedBody = String((char*)data, len);
                String decryptedBody;
                
                if (secureLayer.decryptRequest(clientId, encryptedBody, decryptedBody)) {
                    LOG_DEBUG("WebServer", "🔐 Decrypted PIN settings body length: " + String(decryptedBody.length()) + " bytes");
                    isEncrypted = true;
                    
                    // Парсим расшифрованные данные
                    int pos = 0;
                    while (pos < decryptedBody.length()) {
                        int eqPos = decryptedBody.indexOf('=', pos);
                        if (eqPos == -1) break;
                        
                        int ampPos = decryptedBody.indexOf('&', eqPos);
                        if (ampPos == -1) ampPos = decryptedBody.length();
                        
                        String key = decryptedBody.substring(pos, eqPos);
                        String value = decryptedBody.substring(eqPos + 1, ampPos);
                        
                        // URL decode значения
                        value.replace("%3D", "=");
                        value.replace("%26", "&");
                        value.replace("+", " ");
                        
                        if (key == "enablePinProtection") enablePinProtection = (value == "true");
                        else if (key == "enabledForDevice") enablePinProtection = (value == "true"); // LEGACY
                        else if (key == "enabled") enablePinProtection = (value == "true"); // LEGACY
                        else if (key == "length") pinLength = value.toInt();
                        else if (key == "pin") newPin = value;
                        else if (key == "pin_confirm") confirmPin = value;
                        else if (key == "current_pin") currentPin = value;
                        else if (key == "factory_reset_confirmed") {
                            factoryResetConfirmed = (value == "true");
                        }
                        
                        pos = ampPos + 1;
                    }
                    
                    LOG_INFO("WebServer", "🔐 PIN SETTINGS DECRYPT: enable=" + String(enablePinProtection) + ", len=" + String(pinLength));
                } else {
                    LOG_WARNING("WebServer", "🔐 Failed to decrypt PIN settings data, trying fallback parameters");
                }
            }
            
            // Fallback: обработка обычных параметров если расшифровка не удалась
            if (!isEncrypted) {
                enablePinProtection = request->hasParam("enablePinProtection", true) && (request->getParam("enablePinProtection", true)->value() == "true");
                
                // LEGACY: Поддержка старого параметра enabledForDevice
                if (request->hasParam("enabledForDevice", true)) {
                    enablePinProtection = request->getParam("enabledForDevice", true)->value() == "true";
                    LOG_INFO("WebServer", "🔐 PIN SETTINGS: Using legacy parameter enabledForDevice=" + String(enablePinProtection));
                }
                
                // LEGACY: Поддержка старого параметра enabled
                if (request->hasParam("enabled", true) && !request->hasParam("enablePinProtection", true) && !request->hasParam("enabledForDevice", true)) {
                    enablePinProtection = request->getParam("enabled", true)->value() == "true";
                    LOG_INFO("WebServer", "🔐 PIN SETTINGS: Using legacy parameter enabled=" + String(enablePinProtection));
                }
                
                if (request->hasParam("length", true)) {
                    pinLength = request->getParam("length", true)->value().toInt();
                }
                
                if (request->hasParam("pin", true)) {
                    newPin = request->getParam("pin", true)->value();
                }
                
                if (request->hasParam("pin_confirm", true)) {
                    confirmPin = request->getParam("pin_confirm", true)->value();
                }
                
                if (request->hasParam("current_pin", true)) {
                    currentPin = request->getParam("current_pin", true)->value();
                }
                
                if (request->hasParam("factory_reset_confirmed", true)) {
                    factoryResetConfirmed = (request->getParam("factory_reset_confirmed", true)->value() == "true");
                }
                
                LOG_INFO("WebServer", "🔐 PIN SETTINGS FALLBACK: enable=" + String(enablePinProtection) + ", len=" + String(pinLength));
            }
            
            String message;
            int statusCode;
            bool success;
            bool requiresReboot = false;
            
            // Проверяем текущее состояние device key
            bool currentlyEncrypted = CryptoManager::getInstance().isDeviceKeyEncrypted();
            
            if (enablePinProtection) {
                // ВКЛЮЧЕНИЕ PIN ЗАЩИТЫ
                
                if (newPin.length() == 0) {
                    message = "PIN is required to enable protection.";
                    statusCode = 400;
                    success = false;
                } else if (newPin != confirmPin) {
                    message = "PINs do not match.";
                    statusCode = 400;
                    success = false;
                } else if (newPin.length() < 4 || newPin.length() > 10) {
                    message = "PIN must be 4-10 digits.";
                    statusCode = 400;
                    success = false;
                } else {
                    if (currentlyEncrypted) {
                        // Device key уже зашифрован - меняем PIN
                        LOG_INFO("WebServer", "Changing PIN encryption...");
                        
                        if (currentPin.length() == 0) {
                            message = "Current PIN is required to change PIN.";
                            statusCode = 400;
                            success = false;
                        } else if (CryptoManager::getInstance().changePinEncryption(currentPin, newPin)) {
                            message = "PIN changed successfully! Device will reboot.";
                            statusCode = 200;
                            success = true;
                            requiresReboot = true;
                            LOG_INFO("WebServer", "PIN changed successfully");
                        } else {
                            message = "Failed to change PIN. Current PIN may be incorrect.";
                            statusCode = 400;
                            success = false;
                            LOG_ERROR("WebServer", "Failed to change PIN");
                        }
                    } else {
                        // Device key не зашифрован - включаем защиту через factory reset
                        LOG_WARNING("WebServer", "Enabling PIN protection requires factory reset!");
                        
                        if (!factoryResetConfirmed) {
                            message = "WARNING: Enabling PIN protection will perform FACTORY RESET and delete ALL data (TOTP keys, passwords, WiFi). Export your data first! Send 'factory_reset_confirmed=true' to proceed.";
                            statusCode = 400;
                            success = false;
                            LOG_WARNING("WebServer", "Factory reset not confirmed - aborting PIN enable");
                        } else {
                            // Удаляем все данные (полная очистка как Hardware Reset)
                            LOG_INFO("WebServer", "Starting full factory reset...");
                            
                            LittleFS.remove(KEYS_FILE);
                            LittleFS.remove(PASSWORD_FILE);
                            LittleFS.remove("/wifi_config.json");
                            LittleFS.remove("/wifi_config.json.enc");
                            LittleFS.remove("/splash_config.json");
                            LittleFS.remove(PIN_FILE);
                            LittleFS.remove(CONFIG_FILE);
                            LittleFS.remove(DEVICE_KEY_FILE);
                            LittleFS.remove(BLE_CONFIG_FILE);
                            LittleFS.remove(WEB_ADMIN_FILE);
                            LittleFS.remove(MDNS_CONFIG_FILE);
                            LittleFS.remove(LOGIN_STATE_FILE);
                            LittleFS.remove("/rtc_config.json");
                            LittleFS.remove("/ble_pin.json.enc");
                            LittleFS.remove("/session.json.enc");
                            
                            // URL Obfuscation: Удаление boot counter и всех mappings
                            LOG_INFO("WebServer", "Clearing URL obfuscation data...");
                            LittleFS.remove("/boot_counter.txt");
                            
                            // Удаляем все url_mappings_*.json файлы
                            fs::File root = LittleFS.open("/", "r");
                            if (root) {
                                fs::File file = root.openNextFile();
                                while (file) {
                                    String filename = String(file.name());
                                    if (filename.startsWith("/url_mappings_") && filename.endsWith(".json")) {
                                        LOG_DEBUG("WebServer", "Removing URL mapping file: " + filename);
                                        LittleFS.remove(filename);
                                    }
                                    file = root.openNextFile();
                                }
                            }
                            
                            // Очистка NVS partition (BLE bonding keys)
                            LOG_INFO("WebServer", "Clearing NVS partition...");
                            nvs_flash_erase_partition("nvs");
                            
                            LOG_INFO("WebServer", "All data files deleted for factory reset");
                            
                            // Длина PIN выбирается на устройстве при первой загрузке
                            
                            message = "Factory reset completed! Device will reboot. On next boot, choose PIN length and create PIN on device screen.";
                            statusCode = 200;
                            success = true;
                            requiresReboot = true;
                            LOG_INFO("WebServer", "Factory reset completed - device will prompt for PIN length and PIN on next boot");
                        }
                    }
                }
            } else {
                // ОТКЛЮЧЕНИЕ PIN ЗАЩИТЫ
                
                if (!currentlyEncrypted) {
                    message = "PIN protection is already disabled.";
                    statusCode = 200;
                    success = false;
                    LOG_INFO("WebServer", "PIN protection already disabled");
                } else {
                    // Новая логика: устанавливаем глобальный флаг для запроса PIN на устройстве
                    LOG_INFO("WebServer", "PIN disable requested - will prompt on device");
                    
                    extern bool shouldPromptPinDisable;
                    shouldPromptPinDisable = true;
                    
                    message = "Please enter PIN on device to confirm. Check device screen.";
                    statusCode = 200;
                    success = true;
                    requiresReboot = false; // Не перезагружаем сразу
                    LOG_INFO("WebServer", "PIN disable flag set - device will prompt for PIN");
                }
            }
            
            // Формируем JSON ответ
            JsonDocument doc;
            doc["success"] = success;
            doc["message"] = message;
            doc["requiresReboot"] = requiresReboot;
            String response;
            serializeJson(doc, response);
            
            // Если требуется перезагрузка, планируем её
            if (requiresReboot && success) {
                extern bool shouldRestart;
                shouldRestart = true;
            }

#ifdef SECURE_LAYER_ENABLED
            // 🔐 ЗАШИФРОВАННЫЙ ОТВЕТ для PIN настроек
            bool isTunneled = request->hasHeader("X-Real-Method");
            
            if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId)) {
                LOG_INFO("WebServer", "🔐 PIN SETTINGS ENCRYPTION: Securing response for client " + clientId.substring(0,8) + "..." + (isTunneled ? " [TUNNELED]" : " [DIRECT]") + (isEncrypted ? " [DATA_DECRYPTED]" : " [DATA_PLAIN]"));
                WebServerSecureIntegration::sendSecureResponse(request, statusCode, "application/json", response, secureLayer);
                return;
            } else if (clientId.length() > 0) {
                LOG_WARNING("WebServer", "🔐 PIN SETTINGS FALLBACK: No valid secure session for " + clientId.substring(0,8) + "..., sending plaintext");
            }
#endif
            
            request->send(statusCode, "application/json", response);
        }
    };
    
    // Регистрируем оба варианта: оригинальный и обфусцированный (как у других endpoints)
    server.on("/api/pincode_settings", HTTP_POST, pinSettingsHandler, NULL, pinSettingsBodyHandler);
    String obfuscatedPinSettingsPath = urlObfuscation.obfuscateURL("/api/pincode_settings");
    if (obfuscatedPinSettingsPath.length() > 0 && obfuscatedPinSettingsPath != "/api/pincode_settings") {
        server.on(obfuscatedPinSettingsPath.c_str(), HTTP_POST, pinSettingsHandler, NULL, pinSettingsBodyHandler);
    }

    // BLE PIN API endpoints - PIN viewing removed for security
    // PIN is only displayed on device screen during BLE pairing

    // BLE PIN Update POST endpoint с onBody callback (с поддержкой шифрования)
    auto blePinUpdateHandler = [this](AsyncWebServerRequest *request){
        // Основной обработчик - пустой, вся логика в onBody callback
        LOG_DEBUG("WebServer", "🔐 BLE PIN UPDATE: Main handler called, method=" + String(request->methodToString()));
    };
    
    auto blePinUpdateBodyHandler = [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        LOG_DEBUG("WebServer", "🔐 BLE PIN UPDATE BODY: len=" + String(len) + ", index=" + String(index) + ", total=" + String(total));
        
        if (index + len == total) {
            LOG_INFO("WebServer", "🔐 BLE PIN UPDATE: Processing complete body, size=" + String(total));
            if (!isAuthenticated(request)) return request->send(401);
            if (!verifyCsrfToken(request)) return request->send(403, "text/plain", "CSRF token mismatch");
            
            // 🔐 ОБРАБОТКА ЗАШИФРОВАННЫХ И ОБЫЧНЫХ ПАРАМЕТРОВ
            String bleBondingPinStr = "";
            String deviceBlePinStr = "";
            bool deviceBlePinEnabled = false;
            bool deviceBlePinEnabledProvided = false; // Флаг что параметр был передан
            bool isEncrypted = false;
            bool hasBondingPin = false;
            bool hasDevicePin = false;
            
            String clientId = WebServerSecureIntegration::getClientId(request);
            
            // Попытка расшифровать входящие данные из тела запроса
            if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId) && len > 0) {
                String encryptedBody = String((char*)data, len);
                String decryptedBody;
                
                if (secureLayer.decryptRequest(clientId, encryptedBody, decryptedBody)) {
                    LOG_DEBUG("WebServer", "🔐 [MAIN] Decrypted BLE PIN body: " + decryptedBody.substring(0, 100));
                    isEncrypted = true;
                    
                    // Парсим расшифрованные данные
                    int pos = 0;
                    while (pos < decryptedBody.length()) {
                        int eqPos = decryptedBody.indexOf('=', pos);
                        if (eqPos == -1) break;
                        
                        int ampPos = decryptedBody.indexOf('&', eqPos);
                        if (ampPos == -1) ampPos = decryptedBody.length();
                        
                        String key = decryptedBody.substring(pos, eqPos);
                        String value = decryptedBody.substring(eqPos + 1, ampPos);
                        
                        // URL decode значения
                        value.replace("%3D", "=");
                        value.replace("%26", "&");
                        value.replace("+", " ");
                        
                        if (key == "ble_bonding_pin") {
                            bleBondingPinStr = value;
                            hasBondingPin = (value.length() > 0 && value != "null");
                        } else if (key == "device_ble_pin") {
                            deviceBlePinStr = value;
                            hasDevicePin = (value.length() > 0 && value != "null");
                        } else if (key == "device_ble_pin_enabled") {
                            deviceBlePinEnabled = (value == "true");
                            deviceBlePinEnabledProvided = (value.length() > 0 && value != "null"); // Параметр был передан
                        }
                        
                        pos = ampPos + 1;
                    }
                    
                    LOG_INFO("WebServer", "🔐 BLE PIN DECRYPT: bonding=" + String(hasBondingPin) + ", device=" + String(hasDevicePin));
                } else {
                    LOG_WARNING("WebServer", "🔐 Failed to decrypt BLE PIN data, trying fallback parameters");
                }
            }
            
            // Fallback: обработка обычных параметров если расшифровка не удалась
            if (!isEncrypted) {
                if (request->hasParam("ble_bonding_pin", true)) {
                    bleBondingPinStr = request->getParam("ble_bonding_pin", true)->value();
                    hasBondingPin = (bleBondingPinStr.length() > 0 && bleBondingPinStr != "null");
                    LOG_DEBUG("WebServer", "BLE Bonding PIN received, length=" + String(bleBondingPinStr.length()));
                }
                if (request->hasParam("device_ble_pin", true)) {
                    deviceBlePinStr = request->getParam("device_ble_pin", true)->value();
                    hasDevicePin = (deviceBlePinStr.length() > 0 && deviceBlePinStr != "null");
                }
                if (request->hasParam("device_ble_pin_enabled", true)) {
                    String deviceBlePinEnabledValue = request->getParam("device_ble_pin_enabled", true)->value();
                    deviceBlePinEnabled = (deviceBlePinEnabledValue == "true");
                    deviceBlePinEnabledProvided = (deviceBlePinEnabledValue.length() > 0 && deviceBlePinEnabledValue != "null"); // Параметр был передан
                }
                LOG_INFO("WebServer", "🔐 BLE PIN FALLBACK: bonding=" + String(hasBondingPin) + ", device=" + String(hasDevicePin));
            }
            
            String message;
            int statusCode = 200;
            bool success = true;
            
            LOG_INFO("WebServer", "🔐 [MAIN] BLE PIN UPDATE: hasBondingPin=" + String(hasBondingPin) + ", hasDevicePin=" + String(hasDevicePin));
            
            // Обработка BLE Bonding PIN
            if (hasBondingPin) {
                if (bleBondingPinStr.length() != 6) {
                    message = "BLE Bonding PIN must be exactly 6 digits";
                    statusCode = 400;
                    success = false;
                } else {
                    bool validDigits = true;
                    for (char c : bleBondingPinStr) {
                        if (!isdigit(c)) {
                            validDigits = false;
                            break;
                        }
                    }
                    
                    if (!validDigits) {
                        message = "BLE Bonding PIN must contain only digits";
                        statusCode = 400;
                        success = false;
                    } else {
                        uint32_t blePin = bleBondingPinStr.toInt();
                        
                        if (CryptoManager::getInstance().saveBlePin(blePin)) {
                            LOG_INFO("WebServer", "BLE Bonding PIN updated successfully");
                            
                            // Clear all BLE bonding keys when PIN changes for security
                            if (bleKeyboardManager) {
                                bleKeyboardManager->clearBondingKeys();
                                LOG_INFO("WebServer", "BLE bonding keys cleared due to PIN change");
                            }
                            
                            message = "BLE Bonding PIN updated successfully! All BLE clients cleared.";
                        } else {
                            LOG_ERROR("WebServer", "Failed to save BLE Bonding PIN");
                            message = "Failed to save BLE Bonding PIN";
                            statusCode = 500;
                            success = false;
                        }
                    }
                }
            }
            
            // Обработка Device BLE PIN
            if (success && hasDevicePin) {
                if (deviceBlePinStr.length() != 6) {
                    message = "Device BLE PIN must be exactly 6 digits";
                    statusCode = 400;
                    success = false;
                } else {
                    bool validDigits = true;
                    for (char c : deviceBlePinStr) {
                        if (!isdigit(c)) {
                            validDigits = false;
                            break;
                        }
                    }
                    
                    if (!validDigits) {
                        message = "Device BLE PIN must contain only digits";
                        statusCode = 400;
                        success = false;
                    } else {
                        uint32_t devicePin = deviceBlePinStr.toInt();
                        
                        // Валидация: PIN не должен быть меньше 0 (но 000000 это валидный PIN = 0)
                        // Проверяем что строка действительно была "000000" а не пустая
                        if (deviceBlePinStr == "000000") {
                            devicePin = 0; // Явно устанавливаем 0 для PIN 000000
                        }
                        
                        LOG_INFO("WebServer", "Device BLE PIN validated, saving (value protected)");
                        
                        if (CryptoManager::getInstance().saveDeviceBlePin(devicePin)) {
                            LOG_INFO("WebServer", "Device BLE PIN updated successfully");
                            
                            if (hasBondingPin) {
                                message += " Device BLE PIN also updated.";
                            } else {
                                message = "Device BLE PIN updated successfully!";
                            }
                        } else {
                            LOG_ERROR("WebServer", "Failed to save Device BLE PIN");
                            message = "Failed to save Device BLE PIN";
                            statusCode = 500;
                            success = false;
                        }
                    }
                }
            } else if (success && !hasDevicePin && deviceBlePinEnabledProvided && deviceBlePinEnabled == false) {
                // Выключение Device BLE PIN (только enabled=false без PIN, и параметр ЯВНО передан)
                if (CryptoManager::getInstance().isDeviceBlePinConfigured()) {
                    CryptoManager::getInstance().setDeviceBlePinEnabled(false);
                    LOG_INFO("WebServer", "Device BLE PIN disabled successfully");
                    message = "Device BLE PIN disabled successfully!";
                    hasDevicePin = true; // Помечаем что операция выполнена
                } else {
                    message = "Device BLE PIN not configured";
                    statusCode = 400;
                    success = false;
                }
            }
            
            if (!hasBondingPin && !hasDevicePin) {
                message = "No PIN parameters provided";
                statusCode = 400;
                success = false;
            }
            
            // Формируем JSON ответ
            JsonDocument doc;
            doc["success"] = success;
            doc["message"] = message;
            String response;
            serializeJson(doc, response);

#ifdef SECURE_LAYER_ENABLED
            // 🔐 ЗАШИФРОВАННЫЙ ОТВЕТ для BLE PIN настроек
            bool isTunneled = request->hasHeader("X-Real-Method");
            
            if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId)) {
                LOG_INFO("WebServer", "🔐 BLE PIN ENCRYPTION: Securing response for client " + clientId.substring(0,8) + "..." + (isTunneled ? " [TUNNELED]" : " [DIRECT]") + (isEncrypted ? " [DATA_DECRYPTED]" : " [DATA_PLAIN]"));
                WebServerSecureIntegration::sendSecureResponse(request, statusCode, "application/json", response, secureLayer);
                return;
            } else if (clientId.length() > 0) {
                LOG_WARNING("WebServer", "🔐 BLE PIN FALLBACK: No valid secure session for " + clientId.substring(0,8) + "..., sending plaintext");
            }
#endif
            
            request->send(statusCode, "application/json", response);
        }
    };
    
    // Регистрируем оба варианта: оригинальный и обфусцированный (как у других endpoints)
    server.on("/api/ble_pin_update", HTTP_POST, blePinUpdateHandler, NULL, blePinUpdateBodyHandler);
    String obfuscatedBlePinUpdatePath = urlObfuscation.obfuscateURL("/api/ble_pin_update");
    if (obfuscatedBlePinUpdatePath.length() > 0 && obfuscatedBlePinUpdatePath != "/api/ble_pin_update") {
        server.on(obfuscatedBlePinUpdatePath.c_str(), HTTP_POST, blePinUpdateHandler, NULL, blePinUpdateBodyHandler);
    }

    // DS3231 RTC endpoints
    server.on("/api/rtc", HTTP_GET,[this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        if (!verifyCsrfToken(request)) return request->send(403);
        RTCConfig cfg = rtcManager.getConfig();
        JsonDocument doc;
        doc["enabled"]        = cfg.enabled;
        doc["sda_pin"]        = cfg.sda_pin;
        doc["scl_pin"]        = cfg.scl_pin;
        doc["available"]      = rtcManager.isAvailable();
        doc["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
        if (rtcManager.isAvailable()) {
            DateTime now = rtcManager.getCurrentTime();
            char iso[25];
            snprintf(iso, sizeof(iso), "%04d-%02d-%02dT%02d:%02d:%02dZ",
                     now.year(), now.month(), now.day(),
                     now.hour(), now.minute(), now.second());
            doc["rtc_time"] = iso;
        } else {
            doc["rtc_time"] = nullptr;
        }
        String out; serializeJson(doc, out);
        request->send(200, "application/json", out);
    });

    server.on("/api/rtc", HTTP_POST,[this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        if (!verifyCsrfToken(request)) return request->send(403);
        auto* parsed = (JsonDocument*)request->_tempObject;
        if (!parsed) return request->send(400, "text/plain", "No body");
        RTCConfig cfg;
        cfg.enabled = (*parsed)["enabled"] | false;
        cfg.sda_pin = (*parsed)["sda_pin"]  | 21;
        cfg.scl_pin = (*parsed)["scl_pin"]  | 22;
        time_t ts   = (time_t)(long)(*parsed)["timestamp"];
        delete parsed; request->_tempObject = nullptr;
        rtcManager.saveConfig(cfg);
        bool time_synced = false, available = false;
        String message = "Config saved";
        if (cfg.enabled) {
            if (!rtcManager.isInitialized()) {
                rtcManager.init();
            } else if (cfg.sda_pin != rtcManager.getConfig().sda_pin ||
                       cfg.scl_pin != rtcManager.getConfig().scl_pin) {
                rtcManager.reinit();
            }
            available = rtcManager.isAvailable();
            if (available && ts > 1609459200) {
                struct timeval tv = { ts, 0 };
                settimeofday(&tv, nullptr);
                rtcManager.syncToRTC(ts);
                time_synced = true;
                message = "Config saved and time synchronized";
            } else if (!available) {
                message = "Config saved but DS3231 not found on I2C bus";
            } else {
                message = "Config saved but timestamp invalid";
            }
        }
        JsonDocument resp;
        resp["success"]     = true;
        resp["time_synced"] = time_synced;
        resp["available"]   = available;
        resp["message"]     = message;
        String out; serializeJson(resp, out);
        request->send(200, "application/json", out);
    },NULL,[this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
        if (index + len < total) return;
        JsonDocument* doc = new JsonDocument();
        if (deserializeJson(*doc, data, len) == DeserializationError::Ok) {
            request->_tempObject = doc;
        } else { delete doc; }
    });

    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request){
        int n = WiFi.scanComplete();
        if (n == WIFI_SCAN_RUNNING) {
            request->send(202, "application/json", "[]");
            return;
        }
        if (n == WIFI_SCAN_FAILED || n == 0) {
            WiFi.scanNetworks(true);
            request->send(200, "application/json", "[]");
            return;
        }
        JsonDocument doc;
        JsonArray array = doc.to<JsonArray>();
        for (int i = 0; i < n; ++i) {
            JsonObject net = array.add<JsonObject>();
            net["ssid"] = WiFi.SSID(i);
            net["rssi"] = WiFi.RSSI(i);
        }
        WiFi.scanDelete();
        WiFi.scanNetworks(true);
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    server.on("/api/battery", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        if (!verifyCsrfToken(request)) return request->send(403);
        if (!batteryManager) {
            return request->send(503, "application/json", 
                "{\"error\":\"Battery manager not available\"}");
        }
        JsonDocument doc;
        doc["level"] = batteryManager->getPercentage();
        doc["charging"] = (batteryManager->getVoltage() > 4.15f);
        String out;
        serializeJson(doc, out);
        request->send(200, "application/json", out);
    });

    // Boot Mode endpoint
    server.on("/api/boot-mode", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        if (!verifyCsrfToken(request)) return request->send(403);
        String mode = configManager.getBootMode();
        JsonDocument resp;
        resp["boot_mode"] = mode;
        String out; serializeJson(resp, out);
        request->send(200, "application/json", out);
    });

    server.on("/api/boot-mode", HTTP_POST,[this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        if (!verifyCsrfToken(request)) return request->send(403);
        auto* parsed = (JsonDocument*)request->_tempObject;
        if (!parsed) return request->send(400, "text/plain", "No body");
        String mode = (*parsed)["boot_mode"] | "wifi";
        delete parsed; request->_tempObject = nullptr;
        if (mode != "wifi" && mode != "ap" && mode != "offline") {
            return request->send(400, "application/json", "{\"error\":\"Invalid boot_mode\"}");
        }
        bool ok = configManager.saveBootMode(mode);
        JsonDocument resp;
        resp["success"] = ok;
        resp["boot_mode"] = mode;
        String out; serializeJson(resp, out);
        request->send(200, "application/json", out);
    },NULL,[this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
        if (index + len < total) return;
        JsonDocument* doc = new JsonDocument();
        if (deserializeJson(*doc, data, len) == DeserializationError::Ok) {
            request->_tempObject = doc;
        } else { delete doc; }
    });

    // Clock Settings endpoints
    server.on("/api/clock_settings", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        if (!verifyCsrfToken(request)) return request->send(403);
        
        JsonDocument doc;
        doc["timezone"] = configManager.getTimezone();
        String response;
        serializeJson(doc, response);
        
#ifdef SECURE_LAYER_ENABLED
        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", response, secureLayer);
#else
        request->send(200, "application/json", response);
#endif
    });

    server.on("/api/clock_settings", HTTP_POST, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        if (!verifyCsrfToken(request)) return request->send(403);
        if (request->hasHeader("X-User-Activity")) resetActivityTimer();
        
        String tz = "";
        if (request->hasParam("timezone", true)) {
            tz = request->getParam("timezone", true)->value();
        }
        
        JsonDocument doc;
        String response;
        int statusCode = 200;
        
        if (tz.length() == 0) {
            doc["success"] = false;
            doc["message"] = "Missing timezone parameter!";
            statusCode = 400;
        } else if (tz.length() > 64) {
            doc["success"] = false;
            doc["message"] = "Invalid timezone value!";
            statusCode = 400;
        } else if (configManager.saveTimezone(tz)) {
            setenv("TZ", tz.c_str(), 1);
            tzset();
            LOG_INFO("WebServer", "🕐 Timezone changed to: " + tz);
            doc["success"] = true;
            doc["message"] = "Timezone saved successfully!";
            doc["timezone"] = tz;
        } else {
            doc["success"] = false;
            doc["message"] = "Failed to save timezone!";
            statusCode = 500;
        }
        
        serializeJson(doc, response);
#ifdef SECURE_LAYER_ENABLED
        WebServerSecureIntegration::sendSecureResponse(request, statusCode, "application/json", response, secureLayer);
#else
        request->send(statusCode, "application/json", response);
#endif
    });

    // Display Settings endpoints
    server.on("/api/display_settings", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        
        uint16_t displayTimeout = configManager.getDisplayTimeout();
        uint32_t autoLockTimeout = configManager.getAutoLockTimeout();
        JsonDocument doc;
        doc["display_timeout"] = displayTimeout;
        doc["auto_lock_timeout"] = autoLockTimeout;
        String output;
        serializeJson(doc, output);
        
#ifdef SECURE_LAYER_ENABLED
        String clientId = WebServerSecureIntegration::getClientId(request);
        if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId)) {
            LOG_INFO("WebServer", "🔐 DISPLAY_SETTINGS GET: Securing response for " + clientId.substring(0,8) + "...");
            WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
            return;
        }
#endif
        request->send(200, "application/json", output);
    });

    server.on("/api/display_settings", HTTP_POST,
        [this](AsyncWebServerRequest *request){
            // Пустой основной обработчик - вся логика в onBody callback
        },
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index + len == total) {
                // Проверка аутентификации
                if (!isAuthenticated(request)) {
                    return request->send(401, "text/plain", "Unauthorized");
                }
                
                // Проверка CSRF токена
                if (!verifyCsrfToken(request)) {
                    return request->send(403, "text/plain", "CSRF token mismatch");
                }
                
                String timeoutStr;
                
#ifdef SECURE_LAYER_ENABLED
                String clientId = WebServerSecureIntegration::getClientId(request);
                
                if (clientId.length() > 0 && 
                    secureLayer.isSecureSessionValid(clientId) &&
                    (request->hasHeader("X-Secure-Request") || 
                     request->hasHeader("X-Security-Level"))) {
                    
                    LOG_INFO("WebServer", "🔐 DISPLAY_SETTINGS: Decrypting request for " + clientId.substring(0,8) + "...");
                    
                    // Расшифровка запроса
                    String encryptedBody = String((char*)data, len);
                    String decryptedBody;
                    
                    if (secureLayer.decryptRequest(clientId, encryptedBody, decryptedBody)) {
                        LOG_DEBUG("WebServer", "🔐 DISPLAY_SETTINGS: Decrypted body: " + decryptedBody);
                        
                        // Парсинг параметра display_timeout
                        int timeoutStart = decryptedBody.indexOf("display_timeout=");
                        if (timeoutStart >= 0) {
                            int timeoutEnd = decryptedBody.indexOf("&", timeoutStart);
                            if (timeoutEnd < 0) timeoutEnd = decryptedBody.length();
                            timeoutStr = decryptedBody.substring(timeoutStart + 16, timeoutEnd); // skip "display_timeout="
                            
                            // URL decode (если нужно)
                            timeoutStr = urlDecode(timeoutStr);
                            LOG_DEBUG("WebServer", "🔐 DISPLAY_SETTINGS: Parsed timeout=" + timeoutStr);
                        }
                    } else {
                        LOG_ERROR("WebServer", "🔐 DISPLAY_SETTINGS: Decryption failed");
                        return request->send(400, "text/plain", "Decryption failed");
                    }
                } else
#endif
                {
                    // Fallback: незашифрованный запрос
                    if (request->hasParam("display_timeout", true)) {
                        timeoutStr = request->getParam("display_timeout", true)->value();
                    }
                }
                
                // Валидация display_timeout
                if (timeoutStr.length() == 0) {
                    return request->send(400, "text/plain", "Missing display_timeout parameter!");
                }
                
                uint16_t timeout = timeoutStr.toInt();
                
                // Validate screen timeout values
                if (timeout != 0 && timeout != 15 && timeout != 30 && timeout != 60 && 
                    timeout != 300 && timeout != 1800) {
                    return request->send(400, "text/plain", "Invalid timeout value!");
                }
                
                // Parse and validate auto_lock_timeout
                String autoLockStr;
#ifdef SECURE_LAYER_ENABLED
                {
                    String clientIdAl = WebServerSecureIntegration::getClientId(request);
                    if (clientIdAl.length() > 0 && secureLayer.isSecureSessionValid(clientIdAl) &&
                        (request->hasHeader("X-Secure-Request") || request->hasHeader("X-Security-Level"))) {
                        // already decrypted into timeoutStr path above — re-parse for auto_lock
                        String encBody2 = String((char*)data, len);
                        String decBody2;
                        if (secureLayer.decryptRequest(clientIdAl, encBody2, decBody2)) {
                            int alStart = decBody2.indexOf("auto_lock_timeout=");
                            if (alStart >= 0) {
                                int alEnd = decBody2.indexOf("&", alStart);
                                if (alEnd < 0) alEnd = decBody2.length();
                                autoLockStr = urlDecode(decBody2.substring(alStart + 18, alEnd));
                            }
                        }
                    } else {
                        if (request->hasParam("auto_lock_timeout", true))
                            autoLockStr = request->getParam("auto_lock_timeout", true)->value();
                    }
                }
#else
                if (request->hasParam("auto_lock_timeout", true))
                    autoLockStr = request->getParam("auto_lock_timeout", true)->value();
#endif
                
                uint32_t autoLockTimeout = autoLockStr.length() > 0 ? (uint32_t)autoLockStr.toInt() : configManager.getAutoLockTimeout();
                
                // Validate auto_lock_timeout values
                if (autoLockTimeout != 0 && autoLockTimeout != 300 && autoLockTimeout != 900 &&
                    autoLockTimeout != 1800 && autoLockTimeout != 3600 && autoLockTimeout != 14400) {
                    return request->send(400, "text/plain", "Invalid auto lock timeout value!");
                }
                
                // Cross-validation: auto lock must be > screen timeout (when both non-zero)
                if (timeout > 0 && autoLockTimeout > 0 && autoLockTimeout <= timeout) {
                    return request->send(400, "text/plain", "Auto lock timeout must be greater than screen timeout!");
                }
                
                // Сохранение таймаутов
                String response;
                int statusCode;
                
                JsonDocument doc;
                bool screenSaved = configManager.saveDisplayTimeout(timeout);
                bool autoLockSaved = configManager.saveAutoLockTimeout(autoLockTimeout);
                
                if (screenSaved && autoLockSaved) {
                    doc["success"] = true;
                    doc["message"] = "Display settings saved successfully!";
                    doc["timeout"] = timeout;
                    doc["auto_lock_timeout"] = autoLockTimeout;
                    statusCode = 200;
                    LOG_INFO("WebServer", "Display timeout changed to: " + String(timeout) + "s, auto lock: " + String(autoLockTimeout) + "s");
                } else {
                    doc["success"] = false;
                    doc["message"] = "Failed to save display settings!";
                    statusCode = 500;
                    LOG_ERROR("WebServer", "Failed to save display settings");
                }
                
                serializeJson(doc, response);
                
                // Отправка зашифрованного ответа
#ifdef SECURE_LAYER_ENABLED
                String clientId2 = WebServerSecureIntegration::getClientId(request);
                if (clientId2.length() > 0 && 
                    secureLayer.isSecureSessionValid(clientId2)) {
                    WebServerSecureIntegration::sendSecureResponse(
                        request, statusCode, "application/json", response, secureLayer);
                    return;
                }
#endif
                // Fallback: незашифрованный ответ
                request->send(statusCode, "application/json", response);
            }
        });

    // API: Clear BLE Clients (POST)
    server.on("/api/clear_ble_clients", HTTP_POST,
        [this](AsyncWebServerRequest *request){
            LOG_INFO("WebServer", "🔐 CLEAR_BLE: Main handler called");
            if (!isAuthenticated(request)) return request->send(401);
            if (!verifyCsrfToken(request)) return request->send(403, "text/plain", "CSRF mismatch");
            
            if (bleKeyboardManager) {
                bleKeyboardManager->clearBondingKeys();
                LOG_INFO("WebServer", "BLE bonding keys cleared");
                request->send(200, "application/json", "{\"success\":true,\"message\":\"BLE clients cleared\"}");
            } else {
                request->send(500, "application/json", "{\"success\":false,\"message\":\"BLE manager unavailable\"}");
            }
        },
        NULL,
        NULL
    );

    server.on("/api/theme", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        
        Theme currentTheme = configManager.loadTheme();
        String themeName = (currentTheme == Theme::LIGHT) ? "light" : "dark";
        JsonDocument doc;
        doc["theme"] = themeName;
        String output;
        serializeJson(doc, output);
        
#ifdef SECURE_LAYER_ENABLED
        String clientId = WebServerSecureIntegration::getClientId(request);
        if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId)) {
            LOG_INFO("WebServer", "🔐 THEME GET: Securing response for " + clientId.substring(0,8) + "...");
            WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
            return;
        }
#endif
        request->send(200, "application/json", output);
    });

    server.on("/api/theme", HTTP_POST,
        [this](AsyncWebServerRequest *request){
            // Пустой основной обработчик - вся логика в onBody callback
        },
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index + len == total) {
                // Проверка аутентификации
                if (!isAuthenticated(request)) {
                    return request->send(401, "text/plain", "Unauthorized");
                }
                
                // Проверка CSRF токена
                if (!verifyCsrfToken(request)) {
                    return request->send(403, "text/plain", "CSRF token mismatch");
                }
                
                String theme;
                
#ifdef SECURE_LAYER_ENABLED
                String clientId = WebServerSecureIntegration::getClientId(request);
                
                if (clientId.length() > 0 && 
                    secureLayer.isSecureSessionValid(clientId) &&
                    (request->hasHeader("X-Secure-Request") || 
                     request->hasHeader("X-Security-Level"))) {
                    
                    LOG_INFO("WebServer", "🔐 THEME: Decrypting request for " + clientId.substring(0,8) + "...");
                    
                    // Расшифровка запроса
                    String encryptedBody = String((char*)data, len);
                    String decryptedBody;
                    
                    if (secureLayer.decryptRequest(clientId, encryptedBody, decryptedBody)) {
                        LOG_DEBUG("WebServer", "🔐 THEME: Decrypted body: " + decryptedBody);
                        
                        // Парсинг параметра theme
                        int themeStart = decryptedBody.indexOf("theme=");
                        if (themeStart >= 0) {
                            int themeEnd = decryptedBody.indexOf("&", themeStart);
                            if (themeEnd < 0) themeEnd = decryptedBody.length();
                            theme = decryptedBody.substring(themeStart + 6, themeEnd);
                            
                            // URL decode (если нужно)
                            theme = urlDecode(theme);
                            LOG_DEBUG("WebServer", "🔐 THEME: Parsed theme=" + theme);
                        }
                    } else {
                        LOG_ERROR("WebServer", "🔐 THEME: Decryption failed");
                        return request->send(400, "text/plain", "Decryption failed");
                    }
                } else
#endif
                {
                    // Fallback: незашифрованный запрос
                    if (request->hasParam("theme", true)) {
                        theme = request->getParam("theme", true)->value();
                    }
                }
                
                // Валидация theme
                if (theme.length() == 0) {
                    return request->send(400, "text/plain", "Theme parameter missing.");
                }
                if (theme != "light" && theme != "dark") {
                    return request->send(400, "text/plain", "Invalid theme. Must be 'light' or 'dark'.");
                }
                
                // Применение темы
                Theme newTheme = (theme == "light") ? Theme::LIGHT : Theme::DARK;
                configManager.saveTheme(newTheme);
                
                extern bool pendingThemeChange; extern Theme pendingTheme;
                pendingThemeChange = true; pendingTheme = newTheme;
                
                LOG_INFO("WebServer", "Theme changed to: " + theme);
                
                // Формируем JSON ответ
                JsonDocument doc;
                doc["success"] = true;
                doc["message"] = "Theme updated successfully!";
                doc["theme"] = theme;
                String response;
                serializeJson(doc, response);
                
                // Отправка зашифрованного ответа
#ifdef SECURE_LAYER_ENABLED
                String clientId2 = WebServerSecureIntegration::getClientId(request);
                if (clientId2.length() > 0 && 
                    secureLayer.isSecureSessionValid(clientId2)) {
                    WebServerSecureIntegration::sendSecureResponse(
                        request, 200, "application/json", response, secureLayer);
                    return;
                }
#endif
                // Fallback: незашифрованный ответ
                request->send(200, "application/json", response);
            }
        });

    // API: BLE Device Name Settings (GET with encryption)
    server.on("/api/ble_settings", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        
        JsonDocument doc;
        doc["device_name"] = configManager.loadBleDeviceName();
        String output;
        serializeJson(doc, output);
        
#ifdef SECURE_LAYER_ENABLED
        String clientId = WebServerSecureIntegration::getClientId(request);
        if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId)) {
            LOG_INFO("WebServer", "🔐 BLE_SETTINGS GET: Securing response for " + clientId.substring(0,8) + "...");
            WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
            return;
        }
#endif
        request->send(200, "application/json", output);
    });

    server.on("/api/ble_settings", HTTP_POST,
        [this](AsyncWebServerRequest *request){
            // Пустой основной обработчик
        },
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index + len == total) {
                // Проверка аутентификации
                if (!isAuthenticated(request)) {
                    return request->send(401, "text/plain", "Unauthorized");
                }
                
                // Проверка CSRF токена
                if (!verifyCsrfToken(request)) {
                    return request->send(403, "text/plain", "CSRF token mismatch");
                }
                
                String deviceName;
                
#ifdef SECURE_LAYER_ENABLED
                String clientId = WebServerSecureIntegration::getClientId(request);
                
                if (clientId.length() > 0 && 
                    secureLayer.isSecureSessionValid(clientId) &&
                    (request->hasHeader("X-Secure-Request") || 
                     request->hasHeader("X-Security-Level"))) {
                    
                    // Расшифровка запроса
                    String encryptedBody = String((char*)data, len);
                    String decryptedBody;
                    
                    if (secureLayer.decryptRequest(clientId, encryptedBody, decryptedBody)) {
                        // Парсинг параметра device_name
                        int deviceNameStart = decryptedBody.indexOf("device_name=");
                        if (deviceNameStart >= 0) {
                            int deviceNameEnd = decryptedBody.indexOf("&", deviceNameStart);
                            if (deviceNameEnd < 0) deviceNameEnd = decryptedBody.length();
                            deviceName = decryptedBody.substring(deviceNameStart + 12, deviceNameEnd);
                            
                            // URL decode (полный)
                            deviceName = urlDecode(deviceName);
                        }
                    } else {
                        return request->send(400, "text/plain", "Decryption failed");
                    }
                } else
#endif
                {
                    // Fallback: незашифрованный запрос
                    if (request->hasParam("device_name", true)) {
                        deviceName = request->getParam("device_name", true)->value();
                    }
                }
                
                // Валидация device name
                if (deviceName.length() == 0) {
                    return request->send(400, "text/plain", "Device name parameter missing.");
                }
                if (deviceName.length() > 15) {
                    return request->send(400, "text/plain", "Device name too long (max 15 characters)");
                }
                
                // Сохранение и применение
                configManager.saveBleDeviceName(deviceName);
                if (bleKeyboardManager) {
                    bleKeyboardManager->setDeviceName(deviceName);
                }
                
                // Формирование JSON ответа
                JsonDocument doc;
                doc["success"] = true;
                doc["message"] = "BLE device name updated successfully!";
                String response;
                serializeJson(doc, response);
                
                // Отправка зашифрованного ответа
#ifdef SECURE_LAYER_ENABLED
                String clientId2 = WebServerSecureIntegration::getClientId(request);
                if (clientId2.length() > 0 && 
                    secureLayer.isSecureSessionValid(clientId2)) {
                    WebServerSecureIntegration::sendSecureResponse(
                        request, 200, "application/json", response, secureLayer);
                    return;
                }
#endif
                // Fallback: незашифрованный ответ
                request->send(200, "application/json", response);
            }
        });

    // API: mDNS Hostname Settings (GET with encryption)
    server.on("/api/mdns_settings", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        
        JsonDocument doc;
        doc["hostname"] = configManager.loadMdnsHostname();
        String output;
        serializeJson(doc, output);
        
#ifdef SECURE_LAYER_ENABLED
        String clientId = WebServerSecureIntegration::getClientId(request);
        if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId)) {
            LOG_INFO("WebServer", "🔐 MDNS_SETTINGS GET: Securing response for " + clientId.substring(0,8) + "...");
            WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
            return;
        }
#endif
        request->send(200, "application/json", output);
    });

    server.on("/api/mdns_settings", HTTP_POST,
        [this](AsyncWebServerRequest *request){
            // Пустой основной обработчик
        },
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index + len == total) {
                // Проверка аутентификации
                if (!isAuthenticated(request)) {
                    return request->send(401, "text/plain", "Unauthorized");
                }
                
                // Проверка CSRF токена
                if (!verifyCsrfToken(request)) {
                    return request->send(403, "text/plain", "CSRF token mismatch");
                }
                
                String hostname;
                
#ifdef SECURE_LAYER_ENABLED
                String clientId = WebServerSecureIntegration::getClientId(request);
                
                if (clientId.length() > 0 && 
                    secureLayer.isSecureSessionValid(clientId) &&
                    (request->hasHeader("X-Secure-Request") || 
                     request->hasHeader("X-Security-Level"))) {
                    
                    // Расшифровка запроса
                    String encryptedBody = String((char*)data, len);
                    String decryptedBody;
                    
                    if (secureLayer.decryptRequest(clientId, encryptedBody, decryptedBody)) {
                        // Парсинг параметра hostname
                        int hostnameStart = decryptedBody.indexOf("hostname=");
                        if (hostnameStart >= 0) {
                            int hostnameEnd = decryptedBody.indexOf("&", hostnameStart);
                            if (hostnameEnd < 0) hostnameEnd = decryptedBody.length();
                            hostname = decryptedBody.substring(hostnameStart + 9, hostnameEnd);
                            
                            // URL decode (полный)
                            hostname = urlDecode(hostname);
                        }
                    } else {
                        return request->send(400, "text/plain", "Decryption failed");
                    }
                } else
#endif
                {
                    // Fallback: незашифрованный запрос
                    if (request->hasParam("hostname", true)) {
                        hostname = request->getParam("hostname", true)->value();
                    }
                }
                
                // Валидация hostname
                if (hostname.length() == 0) {
                    return request->send(400, "text/plain", "Hostname parameter missing.");
                }
                if (hostname.length() > 63) {
                    return request->send(400, "text/plain", "Invalid hostname length (1-63 characters)");
                }
                
                // Сохранение и применение
                configManager.saveMdnsHostname(hostname);
                if (wifiManager) {
                    wifiManager->updateMdnsHostname();
                }
                
                // Формирование JSON ответа
                JsonDocument doc;
                doc["success"] = true;
                doc["message"] = "mDNS hostname updated successfully!";
                String response;
                serializeJson(doc, response);
                
                // Отправка зашифрованного ответа
#ifdef SECURE_LAYER_ENABLED
                String clientId2 = WebServerSecureIntegration::getClientId(request);
                if (clientId2.length() > 0 && 
                    secureLayer.isSecureSessionValid(clientId2)) {
                    WebServerSecureIntegration::sendSecureResponse(
                        request, 200, "application/json", response, secureLayer);
                    return;
                }
#endif
                // Fallback: незашифрованный ответ
                request->send(200, "application/json", response);
            }
        });


    // --- Startup Mode API (GET with encryption) ---
    server.on("/api/startup_mode", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        
        String mode = configManager.getStartupMode();
        JsonDocument doc;
        doc["mode"] = mode;
        String response;
        serializeJson(doc, response);
        
#ifdef SECURE_LAYER_ENABLED
        String clientId = WebServerSecureIntegration::getClientId(request);
        if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId)) {
            LOG_INFO("WebServer", "🔐 STARTUP_MODE GET: Securing response for " + clientId.substring(0,8) + "...");
            WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", response, secureLayer);
            return;
        }
#endif
        request->send(200, "application/json", response);
    });

    server.on("/api/startup_mode", HTTP_POST,
        [this](AsyncWebServerRequest *request){
            // Пустой основной обработчик
        },
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index + len == total) {
                // Проверка аутентификации
                if (!isAuthenticated(request)) {
                    return request->send(401, "text/plain", "Unauthorized");
                }
                
                // Проверка CSRF токена
                if (!verifyCsrfToken(request)) {
                    return request->send(403, "text/plain", "CSRF token mismatch");
                }
                
                String mode;
                
#ifdef SECURE_LAYER_ENABLED
                String clientId = WebServerSecureIntegration::getClientId(request);
                
                if (clientId.length() > 0 && 
                    secureLayer.isSecureSessionValid(clientId) &&
                    (request->hasHeader("X-Secure-Request") || 
                     request->hasHeader("X-Security-Level"))) {
                    
                    // Расшифровка запроса
                    String encryptedBody = String((char*)data, len);
                    String decryptedBody;
                    
                    if (secureLayer.decryptRequest(clientId, encryptedBody, decryptedBody)) {
                        // Парсинг параметра mode
                        int modeStart = decryptedBody.indexOf("mode=");
                        if (modeStart >= 0) {
                            int modeEnd = decryptedBody.indexOf("&", modeStart);
                            if (modeEnd < 0) modeEnd = decryptedBody.length();
                            mode = decryptedBody.substring(modeStart + 5, modeEnd);
                            
                            // URL decode (полный)
                            mode = urlDecode(mode);
                        }
                    } else {
                        return request->send(400, "text/plain", "Decryption failed");
                    }
                } else
#endif
                {
                    // Fallback: незашифрованный запрос
                    if (request->hasParam("mode", true)) {
                        mode = request->getParam("mode", true)->value();
                    }
                }
                
                // Валидация mode
                if (mode.length() == 0) {
                    return request->send(400, "text/plain", "Missing mode parameter.");
                }
                if (mode != "totp" && mode != "password") {
                    return request->send(400, "text/plain", "Invalid startup mode. Must be 'totp' or 'password'.");
                }
                
                // Сохранение
                bool success = configManager.saveStartupMode(mode);
                String message;
                int statusCode;
                
                if (success) {
                    LogManager::getInstance().logInfo("WebServer", "Startup mode changed to: " + mode);
                    message = "Startup mode saved successfully!";
                    statusCode = 200;
                } else {
                    LogManager::getInstance().logError("WebServer", "Failed to save startup mode: " + mode);
                    message = "Failed to save startup mode.";
                    statusCode = 500;
                }
                
                // Формирование JSON ответа
                JsonDocument doc;
                doc["success"] = success;
                doc["message"] = message;
                String response;
                serializeJson(doc, response);
                
                // Отправка зашифрованного ответа
#ifdef SECURE_LAYER_ENABLED
                String clientId2 = WebServerSecureIntegration::getClientId(request);
                if (clientId2.length() > 0 && 
                    secureLayer.isSecureSessionValid(clientId2)) {
                    WebServerSecureIntegration::sendSecureResponse(
                        request, statusCode, "application/json", response, secureLayer);
                    return;
                }
#endif
                // Fallback: незашифрованный ответ
                request->send(statusCode, "application/json", response);
            }
        });

    // API: Reboot (POST with encryption)
    server.on("/api/reboot", HTTP_POST,
        [this](AsyncWebServerRequest *request){
            LOG_INFO("WebServer", "🔐 REBOOT: Main handler called");
        },
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (!data || len == 0) {
                LOG_ERROR("WebServer", "🔐 REBOOT: Invalid data");
                return request->send(400, "text/plain", "Invalid request data");
            }
            
            if (index + len == total) {
                if (!isAuthenticated(request)) {
                    return request->send(401, "text/plain", "Unauthorized");
                }
                if (!verifyCsrfToken(request)) {
                    return request->send(403, "text/plain", "CSRF token mismatch");
                }
                
#ifdef SECURE_LAYER_ENABLED
                String clientId = WebServerSecureIntegration::getClientId(request);
                
                if (clientId.length() > 0 && 
                    secureLayer.isSecureSessionValid(clientId) &&
                    (request->hasHeader("X-Secure-Request") || 
                     request->hasHeader("X-Security-Level"))) {
                    
                    LOG_INFO("WebServer", "🔐 REBOOT: Processing encrypted request");
                    
                    // Шифруем ответ
                    String response = "{\"success\":true,\"message\":\"Rebooting\"}";
                    String encryptedResponse;
                    
                    if (secureLayer.encryptResponse(clientId, response, encryptedResponse)) {
                        request->send(200, "application/json", encryptedResponse);
                        delay(1000);
                        ESP.restart();
                    } else {
                        request->send(500, "text/plain", "Encryption failed");
                    }
                    return;
                }
#endif
                
                // Fallback
                LOG_INFO("WebServer", "System reboot requested");
                request->send(200, "text/plain", "Rebooting...");
                delay(1000);
                ESP.restart();
            }
        });

    // API: Reboot with Web Server (POST with encryption)
    server.on("/api/reboot_with_web", HTTP_POST,
        [this](AsyncWebServerRequest *request){
            LOG_INFO("WebServer", "🔐 REBOOT_WEB: Main handler called");
        },
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (!data || len == 0) {
                LOG_ERROR("WebServer", "🔐 REBOOT_WEB: Invalid data");
                return request->send(400, "text/plain", "Invalid request data");
            }
            
            if (index + len == total) {
                if (!isAuthenticated(request)) {
                    return request->send(401, "text/plain", "Unauthorized");
                }
                if (!verifyCsrfToken(request)) {
                    return request->send(403, "text/plain", "CSRF token mismatch");
                }
                
#ifdef SECURE_LAYER_ENABLED
                String clientId = WebServerSecureIntegration::getClientId(request);
                
                if (clientId.length() > 0 && 
                    secureLayer.isSecureSessionValid(clientId) &&
                    (request->hasHeader("X-Secure-Request") || 
                     request->hasHeader("X-Security-Level"))) {
                    
                    LOG_INFO("WebServer", "🔐 REBOOT_WEB: Processing encrypted request");
                    
                    // Set auto-start flag
                    configManager.setWebServerAutoStart(true);
                    LOG_INFO("WebServer", "Web server auto-start flag set successfully");
                    
                    // Шифруем ответ
                    String response = "{\"success\":true,\"message\":\"Rebooting with web server\"}";
                    String encryptedResponse;
                    
                    if (secureLayer.encryptResponse(clientId, response, encryptedResponse)) {
                        request->send(200, "application/json", encryptedResponse);
                        delay(1000);
                        ESP.restart();
                    } else {
                        request->send(500, "text/plain", "Encryption failed");
                    }
                    return;
                }
#endif
                
                // Fallback
                LOG_INFO("WebServer", "System reboot with web server auto-start requested");
                configManager.setWebServerAutoStart(true);
                request->send(200, "text/plain", "Rebooting with web server enabled...");
                LOG_INFO("WebServer", "Web server auto-start flag set successfully");
                delay(1000);
                ESP.restart();
            }
        });

    // API: Device Settings GET (SECURE TESTING ENABLED + URL OBFUSCATION)
    URLObfuscationIntegration::registerDualEndpoint(server, "/api/settings", HTTP_GET, 
        [this](AsyncWebServerRequest *request){
            if (!isAuthenticated(request)) return request->send(401);
            
            JsonDocument doc;
            doc["web_server_timeout"] = configManager.getWebServerTimeout();
            doc["admin_login"] = WebAdminManager::getInstance().getUsername();
            String output;
            serializeJson(doc, output);
            
            LOG_INFO("WebServer", "🔐 SETTINGS GET: Returning device settings");
            
#ifdef SECURE_LAYER_ENABLED
            String clientId = WebServerSecureIntegration::getClientId(request);
            bool isTunneled = request->hasHeader("X-Real-Method");
            
            if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId)) {
                LOG_INFO("WebServer", "🔐 SETTINGS ENCRYPTION: Securing response for client " + clientId.substring(0,8) + "..." + (isTunneled ? " [TUNNELED]" : " [DIRECT]"));
                WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                return;
            } else if (clientId.length() > 0) {
                LOG_WARNING("WebServer", "🔐 SETTINGS FALLBACK: No valid secure session for " + clientId.substring(0,8) + "..., sending plaintext");
            }
#endif
            
            request->send(200, "application/json", output);
        }, urlObfuscation);

    server.on("/api/settings", HTTP_POST,
        [this](AsyncWebServerRequest *request){
            // Пустой основной обработчик
        },
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index + len == total) {
                // Проверка аутентификации
                if (!isAuthenticated(request)) {
                    return request->send(401, "text/plain", "Unauthorized");
                }
                
                // Проверка CSRF токена
                if (!verifyCsrfToken(request)) {
                    return request->send(403, "text/plain", "CSRF token mismatch");
                }
                
                String timeoutStr;
                
#ifdef SECURE_LAYER_ENABLED
                String clientId = WebServerSecureIntegration::getClientId(request);
                
                if (clientId.length() > 0 && 
                    secureLayer.isSecureSessionValid(clientId) &&
                    (request->hasHeader("X-Secure-Request") || 
                     request->hasHeader("X-Security-Level"))) {
                    
                    // Расшифровка запроса
                    String encryptedBody = String((char*)data, len);
                    String decryptedBody;
                    
                    if (secureLayer.decryptRequest(clientId, encryptedBody, decryptedBody)) {
                        // Парсинг параметра web_server_timeout
                        int timeoutStart = decryptedBody.indexOf("web_server_timeout=");
                        if (timeoutStart >= 0) {
                            int timeoutEnd = decryptedBody.indexOf("&", timeoutStart);
                            if (timeoutEnd < 0) timeoutEnd = decryptedBody.length();
                            timeoutStr = decryptedBody.substring(timeoutStart + 19, timeoutEnd);
                        }
                    } else {
                        return request->send(400, "text/plain", "Decryption failed");
                    }
                } else
#endif
                {
                    // Fallback: незашифрованный запрос
                    if (request->hasParam("web_server_timeout", true)) {
                        timeoutStr = request->getParam("web_server_timeout", true)->value();
                    }
                }
                
                // Валидация
                if (timeoutStr.length() == 0) {
                    JsonDocument doc;
                    doc["success"] = false;
                    doc["message"] = "Missing parameters.";
                    String response;
                    serializeJson(doc, response);
                    return request->send(400, "application/json", response);
                }
                
                uint16_t timeout = timeoutStr.toInt();
                configManager.setWebServerTimeout(timeout);
                _timeoutMinutes = timeout;
                
                // Формирование JSON ответа
                JsonDocument doc;
                doc["success"] = true;
                doc["message"] = "Settings updated successfully! Device will restart...";
                String response;
                serializeJson(doc, response);
                
                // Отправка зашифрованного ответа
#ifdef SECURE_LAYER_ENABLED
                String clientId2 = WebServerSecureIntegration::getClientId(request);
                if (clientId2.length() > 0 && 
                    secureLayer.isSecureSessionValid(clientId2)) {
                    WebServerSecureIntegration::sendSecureResponse(
                        request, 200, "application/json", response, secureLayer);
                    
                    // Schedule device restart
                    extern bool shouldRestart;
                    shouldRestart = true;
                    return;
                }
#endif
                // Fallback: незашифрованный ответ
                request->send(200, "application/json", response);
                
                // Schedule device restart
                extern bool shouldRestart;
                shouldRestart = true;
            }
        });

#ifdef SECURE_LAYER_ENABLED
    // Добавляем secure endpoints для HTTPS-like шифрования
    SecureLayerManager& secureLayerManager = SecureLayerManager::getInstance();
    WebServerSecureIntegration::addSecureEndpoints(server, secureLayerManager, urlObfuscation);
    LOG_INFO("WebServer", "Secure endpoints added for HTTPS-like encryption");
    
    // Method Tunneling Integration - скрытие HTTP методов от анализа трафика
    MethodTunnelingManager& methodTunneling = MethodTunnelingManager::getInstance();
    methodTunneling.begin();
    
    // Регистрируем туннельный endpoint для скрытых методов
    server.on("/api/tunnel", HTTP_POST, [this, &methodTunneling](AsyncWebServerRequest *request) {
        // Главный handler - для GET запросов без тела
        if (!isAuthenticated(request)) return request->send(401);
        
        if (!request->hasHeader("X-Real-Method")) {
            request->send(400, "text/plain", "Missing X-Real-Method header");
            return;
        }
        
        String encryptedMethod = request->getHeader("X-Real-Method")->value();
        String clientId = WebServerSecureIntegration::getClientId(request);
        String realMethod = methodTunneling.decryptMethodHeader(encryptedMethod, clientId);
        
        if (realMethod.isEmpty()) {
            request->send(400, "text/plain", "Failed to decrypt method header");
            return;
        }
        
        // GET запросы будут обработаны в onBody handler
        // Не отвечаем здесь - ждем body с endpoint
        
    }, NULL, [this, &methodTunneling](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        // onBody callback - для POST/DELETE запросов С ТЕЛОМ
        // Используем request-specific хранилище для безопасности
        String* bufferPtr = nullptr;
        
        if (index == 0) {
            // Первый чанк - создаем буфер и сохраняем в request
            bufferPtr = new String();
            bufferPtr->reserve(total + 10); // Резервируем память
            request->_tempObject = bufferPtr;
        } else {
            // Последующие чанки - извлекаем буфер
            bufferPtr = (String*)request->_tempObject;
        }
        
        if (bufferPtr) {
            bufferPtr->concat((char*)data, len);
        }
        
        if (index + len >= total) {
            // Проверка аутентификации
            if (!isAuthenticated(request)) {
                if (bufferPtr) {
                    delete bufferPtr;
                    request->_tempObject = nullptr;
                }
                return request->send(401);
            }
            
            // Проверка заголовка
            if (!request->hasHeader("X-Real-Method")) {
                if (bufferPtr) {
                    delete bufferPtr;
                    request->_tempObject = nullptr;
                }
                request->send(400, "text/plain", "Missing X-Real-Method header");
                return;
            }
            
            String encryptedMethod = request->getHeader("X-Real-Method")->value();
            String clientId = WebServerSecureIntegration::getClientId(request);
            String realMethod = methodTunneling.decryptMethodHeader(encryptedMethod, clientId);
            
            if (realMethod.isEmpty()) {
                if (bufferPtr) {
                    delete bufferPtr;
                    request->_tempObject = nullptr;
                }
                request->send(400, "text/plain", "Failed to decrypt method header");
                return;
            }
            
            LOG_INFO("WebServer", "🚇 Method tunneling: " + realMethod + " with body [Client:" + 
                     (clientId.length() > 0 ? clientId.substring(0,8) + "...]" : "NONE]"));
            
#ifdef SECURE_LAYER_ENABLED
            // 🔐 РАСШИФРОВЫВАЕМ ТЕЛО TUNNEL ЗАПРОСА
            if (!bufferPtr) {
                LOG_ERROR("WebServer", "❌ Tunnel buffer is null!");
                return request->send(500, "text/plain", "Internal server error");
            }
            
            String encryptedBody = *bufferPtr;  // ✅ Используем накопленные данные!
            String decryptedTunnelBody;
            
            LOG_DEBUG("WebServer", "💾 Tunnel body size: " + String(encryptedBody.length()) + "b (total: " + String(total) + "b)");
            
            if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId) &&
                secureLayer.decryptRequest(clientId, encryptedBody, decryptedTunnelBody)) {
                
                LOG_DEBUG("WebServer", "🔐 Decrypted tunnel body: " + decryptedTunnelBody.substring(0, 100) + "...");
                
                // Парсим tunnel JSON: {"endpoint":"/api/add","method":"POST","data":{"name":"...","secret":"..."}}
                // ArduinoJson 7 оптимизирует память автоматически
                JsonDocument tunnelDoc;
                DeserializationError error = deserializeJson(tunnelDoc, decryptedTunnelBody);
                
                if (error) {
                    LOG_ERROR("WebServer", "🚇 Failed to parse tunnel JSON: " + String(error.c_str()));
                    return request->send(400, "text/plain", "Invalid tunnel body JSON");
                }
                
                String targetEndpoint = tunnelDoc["endpoint"].as<String>();
                String targetMethod = tunnelDoc["method"] | realMethod; // Fallback к расшифрованному методу
                JsonObject targetData = tunnelDoc["data"].as<JsonObject>();
                
                LOG_INFO("WebServer", "🚇 Tunnel target: " + targetMethod + " " + targetEndpoint);
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/keys GET
                if (targetEndpoint == "/api/keys" && targetMethod == "GET") {
                    if (request->hasHeader("X-User-Activity")) {
                        resetActivityTimer();
                    }
                    
                    // ArduinoJson 7 автоматически управляет памятью
                    JsonDocument doc;
                    JsonArray keysArray = doc.to<JsonArray>();
                    auto keys = keyManager.getAllKeys();
                    
                    // 🔐 Блокировка TOTP в AP/Offline режимах
                    bool blockTOTP = !totpGenerator.isTimeSynced();
                    
                    for (size_t i = 0; i < keys.size(); i++) {
                        JsonObject keyObj = keysArray.add<JsonObject>();
                        keyObj["name"] = keys[i].name;
                        // HOTP works without time sync, TOTP requires it
                        if (keys[i].type == TOTPType::HOTP) {
                            keyObj["code"] = totpGenerator.generateCode(keys[i]);
                        } else {
                            keyObj["code"] = blockTOTP ? "NOT SYNCED" : totpGenerator.generateCode(keys[i]);
                        }
                        
                        if (keys[i].type == TOTPType::TOTP) {
                            keyObj["timeLeft"] = totpGenerator.getTimeRemaining(keys[i].period);
                        } else {
                            keyObj["timeLeft"] = 0;
                        }
                        
                        // Метаданные (всегда включаем)
                        keyObj["t"] = (keys[i].type == TOTPType::HOTP) ? "H" : "T";
                        keyObj["a"] = (keys[i].algorithm == TOTPAlgorithm::SHA256) ? "256" :
                                      (keys[i].algorithm == TOTPAlgorithm::SHA512) ? "512" : "1";
                        keyObj["d"] = keys[i].digits;
                        keyObj["p"] = keys[i].period;
                        if (keys[i].counter != 0) {
                            keyObj["c"] = keys[i].counter;
                        }
                    }
                    
                    String response;
                    serializeJson(doc, response);
                    
                    LOG_INFO("WebServer", "🔐 TOTP ENCRYPTION: Securing tunneled keys data [TUNNELED]");
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", response, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/add POST
                if (targetEndpoint == "/api/add" && targetMethod == "POST") {
                    // 🆕 Поддержка нового компактного формата и старого
                    String name, secret;
                    TOTPType type = TOTPType::TOTP;
                    TOTPAlgorithm algo = TOTPAlgorithm::SHA1;
                    uint8_t digits = 6;
                    uint16_t period = 30;
                    
                    if (!targetData["n"].isNull() && !targetData["s"].isNull()) {
                        // Новый компактный формат
                        name = targetData["n"].as<String>();
                        secret = targetData["s"].as<String>();
                        
                        // Парсим опциональные параметры
                        if (!targetData["t"].isNull() && targetData["t"].as<String>() == "H") {
                            type = TOTPType::HOTP;
                        }
                        
                        if (!targetData["a"].isNull()) {
                            String algoStr = targetData["a"].as<String>();
                            if (algoStr == "256") algo = TOTPAlgorithm::SHA256;
                            else if (algoStr == "512") algo = TOTPAlgorithm::SHA512;
                        }
                        
                        if (!targetData["d"].isNull()) {
                            digits = targetData["d"].as<uint8_t>();
                        }
                        
                        if (!targetData["p"].isNull()) {
                            period = targetData["p"].as<uint16_t>();
                        }
                        
                        LOG_INFO("WebServer", "🚇 TUNNELED Key add (extended): " + name);
                        
                        bool success;
                        if (type == TOTPType::HOTP || algo != TOTPAlgorithm::SHA1 || 
                            digits != 6 || period != 30) {
                            success = keyManager.addKeyExtended(name, secret, type, algo, digits, period);
                        } else {
                            success = keyManager.addKey(name, secret);
                        }
                        
                        String output = success ? 
                            "{\"status\":\"success\",\"message\":\"Key added successfully\",\"name\":\"" + name + "\"}" :
                            "{\"status\":\"error\",\"message\":\"Failed to add key\"}";
                        
                        LOG_INFO("WebServer", "🔐 KEY ADD ENCRYPTION: Securing tunneled response");
                        WebServerSecureIntegration::sendSecureResponse(request, success ? 200 : 400, "application/json", output, secureLayer);
                        return;
                        
                    } else if (!targetData["name"].isNull() && !targetData["secret"].isNull()) {
                        // Старый формат (обратная совместимость)
                        name = targetData["name"].as<String>();
                        secret = targetData["secret"].as<String>();
                        
                        if (name.isEmpty() || secret.isEmpty()) {
                            return request->send(400, "text/plain", "Name and secret cannot be empty");
                        }
                        
                        LOG_INFO("WebServer", "🚇 TUNNELED Key add (legacy): " + name);
                        keyManager.addKey(name, secret);
                        
                        String output = "{\"status\":\"success\",\"message\":\"Key added successfully\",\"name\":\"" + name + "\"}";
                        
                        LOG_INFO("WebServer", "🔐 KEY ADD ENCRYPTION: Securing tunneled response");
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        return;
                    } else {
                        return request->send(400, "text/plain", "Missing required fields");
                    }
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/show_qr POST
                if (targetEndpoint == "/api/show_qr" && targetMethod == "POST") {
                    int keyIndex = targetData["key_id"].as<int>();
                    
                    auto keys = keyManager.getAllKeys();
                    if (keyIndex < 0 || keyIndex >= keys.size()) {
                        LOG_ERROR("WebServer", "🚇 TUNNELED SHOW_QR: Invalid key index: " + String(keyIndex));
                        String output = "{\"error\":\"Key not found\"}";
                        WebServerSecureIntegration::sendSecureResponse(request, 404, "application/json", output, secureLayer);
                        return;
                    }
                    
                    TOTPKey key = keys[keyIndex];
                    
                    // TOTP secret в uppercase (Base32 формат)
                    String secretUpper = key.secret;
                    secretUpper.toUpperCase();
                    
                    // Формируем полный TOTP/HOTP URI с расширенными параметрами
                    String totpUri;
                    if (key.type == TOTPType::HOTP) {
                        totpUri = "otpauth://hotp/" + key.name + "?secret=" + secretUpper;
                        totpUri += "&counter=" + String(key.counter);
                    } else {
                        totpUri = "otpauth://totp/" + key.name + "?secret=" + secretUpper;
                        if (key.period != 30) {
                            totpUri += "&period=" + String(key.period);
                        }
                    }
                    
                    if (key.algorithm == TOTPAlgorithm::SHA256) {
                        totpUri += "&algorithm=SHA256";
                    } else if (key.algorithm == TOTPAlgorithm::SHA512) {
                        totpUri += "&algorithm=SHA512";
                    }
                    
                    if (key.digits != 6) {
                        totpUri += "&digits=" + String(key.digits);
                    }
                    
                    LOG_INFO("WebServer", "🚇 TUNNELED SHOW_QR: Displaying QR for key at index " + String(index));
                    displayManager.requestShowQRCode(totpUri, 30);
                    
                    String output = "{\"success\":true,\"message\":\"QR code displayed on device screen for 30 seconds\"}";
                    
                    LOG_INFO("WebServer", "🔐 SHOW_QR ENCRYPTION: Securing tunneled response");
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/remove POST
                if (targetEndpoint == "/api/remove" && targetMethod == "POST") {
                    int index = targetData["index"].as<int>();
                    
                    LOG_INFO("WebServer", "🚇 TUNNELED Key remove: index=" + String(index));
                    keyManager.removeKey(index);
                    
                    // 🛡️ Ручное формирование JSON
                    String output = "{\"status\":\"success\",\"message\":\"Key removed successfully\"}";
                    
                    
                    LOG_INFO("WebServer", "🔐 KEY REMOVE ENCRYPTION: Securing tunneled response");
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/hotp/generate POST
                if (targetEndpoint == "/api/hotp/generate" && targetMethod == "POST") {
                    int index = targetData["index"].as<int>();
                    
                    auto keys = keyManager.getAllKeys();
                    if (index < 0 || index >= keys.size()) {
                        return request->send(400, "text/plain", "Invalid key index");
                    }
                    
                    if (keys[index].type != TOTPType::HOTP) {
                        return request->send(400, "text/plain", "Key is not HOTP type");
                    }
                    
                    LOG_INFO("WebServer", "🚇 TUNNELED HOTP generate: index=" + String(index));
                    
                    if (!keyManager.incrementHOTPCounter(index)) {
                        return request->send(500, "text/plain", "Failed to increment HOTP counter");
                    }
                    
                    auto updatedKeys = keyManager.getAllKeys();
                    String newCode = totpGenerator.generateCode(updatedKeys[index]);
                    
                    // 🛡️ Ручное формирование JSON
                    String output = "{\"status\":\"success\",\"code\":\"" + newCode + "\",\"counter\":" + String(updatedKeys[index].counter) + "}";
                    
                    LOG_INFO("WebServer", "🔐 HOTP GENERATE ENCRYPTION: Securing tunneled response");
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/keys/reorder POST
                if (targetEndpoint == "/api/keys/reorder" && targetMethod == "POST") {
                    // Парсим order массив
                    if (!targetData["order"].is<JsonArray>()) {
                        return request->send(400, "text/plain", "Missing or invalid 'order' field");
                    }
                    
                    std::vector<std::pair<String, int>> newOrder;
                    JsonArray orderArray = targetData["order"].as<JsonArray>();
                    
                    for (JsonObject item : orderArray) {
                        String name = item["name"].as<String>();
                        int order = item["order"].as<int>();
                        newOrder.push_back(std::make_pair(name, order));
                    }
                    
                    LOG_INFO("WebServer", "🚇 TUNNELED Keys reorder: " + String(newOrder.size()) + " keys");
                    bool success = keyManager.reorderKeys(newOrder);
                    
                    String output;
                    int statusCode;
                    if (success) {
                        output = "{\"status\":\"success\",\"message\":\"Keys reordered successfully!\"}";
                        statusCode = 200;
                        LOG_INFO("WebServer", "🚇 Keys reordered successfully");
                    } else {
                        output = "{\"status\":\"error\",\"message\":\"Failed to reorder keys\"}";
                        statusCode = 500;
                        LOG_ERROR("WebServer", "🚇 Failed to reorder keys");
                    }
                    
                    LOG_INFO("WebServer", "🔐 KEYS REORDER ENCRYPTION: Securing tunneled response");
                    WebServerSecureIntegration::sendSecureResponse(request, statusCode, "text/plain", output, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/export POST
                if (targetEndpoint == "/api/export" && targetMethod == "POST") {
                    // Проверка API доступа
                    if (!WebAdminManager::getInstance().isApiEnabled()) {
                        LOG_WARNING("WebServer", "🚇 TUNNELED export blocked: API disabled");
                        return request->send(403, "text/plain", "API access for import/export is disabled.");
                    }
                    
                    String password = targetData["password"].as<String>();
                    
                    if (password.isEmpty()) {
                        return request->send(400, "text/plain", "Password cannot be empty");
                    }
                    
                    // Проверка admin пароля
                    if (!WebAdminManager::getInstance().verifyCredentials(WebAdminManager::getInstance().getUsername(), password)) {
                        LOG_WARNING("WebServer", "🚇 TUNNELED export failed: Invalid admin password");
                        return request->send(401, "text/plain", "Invalid admin password.");
                    }
                    
                    LOG_INFO("WebServer", "🚇 TUNNELED TOTP export: Password verified");
                    auto keys = keyManager.getAllKeys();
                    
                    // ArduinoJson 7 автоматически управляет памятью
                    JsonDocument doc;
                    JsonArray array = doc.to<JsonArray>();
                    for (const auto& key : keys) {
                        JsonObject obj = array.add<JsonObject>();
                        obj["name"] = key.name;
                        obj["secret"] = key.secret;
                        // Добавляем расширенные метаданные
                        obj["type"] = (key.type == TOTPType::HOTP) ? "H" : "T";
                        obj["algorithm"] = (key.algorithm == TOTPAlgorithm::SHA256) ? "256" :
                                           (key.algorithm == TOTPAlgorithm::SHA512) ? "512" : "1";
                        obj["digits"] = key.digits;
                        obj["period"] = key.period;
                        if (key.counter != 0) {
                            obj["counter"] = key.counter;
                        }
                    }
                    String plaintext;
                    serializeJson(doc, plaintext);
                    
                    String encryptedContent = CryptoManager::getInstance().encryptWithPassword(plaintext, password);
                    
                    LOG_INFO("WebServer", "💾 TOTP EXPORT: Sending encrypted file directly [TUNNELED]");
                    // Файл уже зашифрован CryptoManager, отправляем напрямую
                    // Не нужно оборачивать в JSON - encryptedContent уже JSON
                    
                    // Отправляем через XOR шифрование
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", encryptedContent, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/import POST
                if (targetEndpoint == "/api/import" && targetMethod == "POST") {
                    // Проверка API доступа
                    if (!WebAdminManager::getInstance().isApiEnabled()) {
                        LOG_WARNING("WebServer", "🚇 TUNNELED import blocked: API disabled");
                        return request->send(403, "text/plain", "API access for import/export is disabled.");
                    }
                    
                    String password = targetData["password"].as<String>();
                    String fileContent = targetData["data"].as<String>();
                    
                    if (password.isEmpty() || fileContent.isEmpty()) {
                        LOG_ERROR("WebServer", "❌ TUNNELED import: Missing data (pwd:" + String(password.length()) + ", file:" + String(fileContent.length()) + ")");
                        return request->send(400, "text/plain", "Missing password or file data.");
                    }
                    
                    LOG_INFO("WebServer", "🚇 TUNNELED TOTP import: Decrypting file content");
                    String decryptedContent = CryptoManager::getInstance().decryptWithPassword(fileContent, password);
                    
                    if (decryptedContent.isEmpty()) {
                        LOG_WARNING("WebServer", "🚇 TUNNELED import failed: Decryption failed");
                        return request->send(400, "text/plain", "Decryption failed. Wrong password or corrupt file.");
                    }
                    
                    if (keyManager.replaceAllKeys(decryptedContent)) {
                        LOG_INFO("WebServer", "🚇 TUNNELED TOTP import: Keys imported successfully");
                        
                        // 🛡️ Ручное формирование JSON
                        String jsonResponse = "{\"status\":\"success\",\"message\":\"Import successful!\"}";
                        
                        LOG_INFO("WebServer", "🔐 IMPORT ENCRYPTION: Securing tunneled import response [TUNNELED]");
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", jsonResponse, secureLayer);
                        return;
                    } else {
                        LOG_ERROR("WebServer", "🚇 TUNNELED import failed: Failed to process keys");
                        
                        // 🛡️ Ручное формирование JSON
                        String errorResponse = "{\"status\":\"error\",\"message\":\"Failed to process keys after decryption.\"}";
                        
                        WebServerSecureIntegration::sendSecureResponse(request, 500, "application/json", errorResponse, secureLayer);
                        return;
                    }
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/passwords GET
                if (targetEndpoint == "/api/passwords" && targetMethod == "GET") {
                    if (request->hasHeader("X-User-Activity")) {
                        resetActivityTimer();
                    }
                    
                    LOG_INFO("WebServer", "🚇 TUNNELED passwords list request");
                    auto passwords = passwordManager.getAllPasswords();
                    
                    // Создаем JSON в том же формате что и прямой endpoint
                    // ArduinoJson 7 автоматически управляет памятью
                    JsonDocument doc;
                    JsonArray array = doc.to<JsonArray>();
                    for (const auto& entry : passwords) {
                        JsonObject obj = array.add<JsonObject>();
                        obj["name"] = entry.name;
                        obj["password"] = entry.password;  // Только name и password в PasswordEntry
                    }
                    String output;
                    serializeJson(doc, output);
                    
                    LOG_INFO("WebServer", "🔐 PASSWORD ENCRYPTION: Securing tunneled passwords data [TUNNELED]");
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/passwords/export POST
                if (targetEndpoint == "/api/passwords/export" && targetMethod == "POST") {
                    // Проверка API доступа
                    if (!WebAdminManager::getInstance().isApiEnabled()) {
                        LOG_WARNING("WebServer", "🚇 TUNNELED passwords export blocked: API disabled");
                        return request->send(403, "text/plain", "API access for import/export is disabled.");
                    }
                    
                    String password = targetData["password"].as<String>();
                    
                    if (password.isEmpty()) {
                        return request->send(400, "text/plain", "Password cannot be empty");
                    }
                    
                    // Проверка admin пароля
                    if (!WebAdminManager::getInstance().verifyCredentials(WebAdminManager::getInstance().getUsername(), password)) {
                        LOG_WARNING("WebServer", "🚇 TUNNELED passwords export failed: Invalid admin password");
                        return request->send(401, "text/plain", "Invalid admin password.");
                    }
                    
                    LOG_INFO("WebServer", "🚇 TUNNELED passwords export: Password verified");
                    auto passwords = passwordManager.getAllPasswords();
                    
                    JsonDocument doc;
                    JsonArray array = doc.to<JsonArray>();
                    for (const auto& entry : passwords) {
                        JsonObject obj = array.add<JsonObject>();
                        obj["name"] = entry.name;
                        obj["password"] = entry.password;  // Только name и password в PasswordEntry
                    }
                    String plaintext;
                    serializeJson(doc, plaintext);
                    
                    String encryptedContent = CryptoManager::getInstance().encryptWithPassword(plaintext, password);
                    
                    LOG_INFO("WebServer", "💾 PASSWORDS EXPORT: Sending encrypted file directly [TUNNELED]");
                    // Файл уже зашифрован CryptoManager, отправляем напрямую
                    // Не нужно оборачивать в JSON - encryptedContent уже JSON
                    
                    // Отправляем через XOR шифрование
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", encryptedContent, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/passwords/import POST
                if (targetEndpoint == "/api/passwords/import" && targetMethod == "POST") {
                    // Проверка API доступа
                    if (!WebAdminManager::getInstance().isApiEnabled()) {
                        LOG_WARNING("WebServer", "🚇 TUNNELED passwords import blocked: API disabled");
                        return request->send(403, "text/plain", "API access for import/export is disabled.");
                    }
                    
                    String password = targetData["password"].as<String>();
                    String fileContent = targetData["data"].as<String>();
                    
                    if (password.isEmpty() || fileContent.isEmpty()) {
                        LOG_ERROR("WebServer", "❌ TUNNELED passwords import: Missing data (pwd:" + String(password.length()) + ", file:" + String(fileContent.length()) + ")");
                        return request->send(400, "text/plain", "Missing password or file data.");
                    }
                    
                    LOG_INFO("WebServer", "🚇 TUNNELED passwords import: Decrypting file content");
                    String decryptedContent = CryptoManager::getInstance().decryptWithPassword(fileContent, password);
                    
                    if (decryptedContent.isEmpty()) {
                        LOG_WARNING("WebServer", "🚇 TUNNELED passwords import failed: Decryption failed");
                        return request->send(400, "text/plain", "Decryption failed. Wrong password or corrupt file.");
                    }
                    
                    if (passwordManager.replaceAllPasswords(decryptedContent)) {
                        LOG_INFO("WebServer", "🚇 TUNNELED passwords import: Passwords imported successfully");
                        
                        JsonDocument responseDoc;
                        responseDoc["status"] = "success";
                        responseDoc["message"] = "Import successful!";
                        String jsonResponse;
                        serializeJson(responseDoc, jsonResponse);
                        
                        LOG_INFO("WebServer", "🔐 PASSWORDS IMPORT ENCRYPTION: Securing tunneled import response [TUNNELED]");
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", jsonResponse, secureLayer);
                        return;
                    } else {
                        LOG_ERROR("WebServer", "🚇 TUNNELED passwords import failed: Failed to process passwords");
                        
                        JsonDocument errorDoc;
                        errorDoc["status"] = "error";
                        errorDoc["message"] = "Failed to process passwords after decryption.";
                        String errorResponse;
                        serializeJson(errorDoc, errorResponse);
                        
                        WebServerSecureIntegration::sendSecureResponse(request, 500, "application/json", errorResponse, secureLayer);
                        return;
                    }
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/passwords/add POST
                if (targetEndpoint == "/api/passwords/add" && targetMethod == "POST") {
                    if (request->hasHeader("X-User-Activity")) {
                        resetActivityTimer();
                    }
                    
                    String name = targetData["name"].as<String>();
                    String password = targetData["password"].as<String>();
                    
                    LOG_INFO("WebServer", "🚇 TUNNELED Password add: " + name);
                    
                    if (passwordManager.addPassword(name, password)) {
                        LOG_INFO("WebServer", "🔐 Password added: " + name);
                        
                        JsonDocument responseDoc;
                        responseDoc["status"] = "success";
                        responseDoc["message"] = "Password added successfully!";
                        String jsonResponse;
                        serializeJson(responseDoc, jsonResponse);
                        
                        LOG_INFO("WebServer", "🔐 PASSWORD ADD ENCRYPTION: Securing tunneled response");
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", jsonResponse, secureLayer);
                        return;
                    } else {
                        JsonDocument errorDoc;
                        errorDoc["status"] = "error";
                        errorDoc["message"] = "Failed to add password";
                        String errorResponse;
                        serializeJson(errorDoc, errorResponse);
                        
                        WebServerSecureIntegration::sendSecureResponse(request, 500, "application/json", errorResponse, secureLayer);
                        return;
                    }
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/passwords/delete POST
                if (targetEndpoint == "/api/passwords/delete" && targetMethod == "POST") {
                    if (request->hasHeader("X-User-Activity")) {
                        resetActivityTimer();
                    }
                    
                    int index = targetData["index"].as<int>();
                    
                    LOG_INFO("WebServer", "🚇 TUNNELED Password delete: index " + String(index));
                    
                    if (passwordManager.deletePassword(index)) {
                        LOG_INFO("WebServer", "🔐 Password deleted at index: " + String(index));
                        
                        JsonDocument responseDoc;
                        responseDoc["status"] = "success";
                        responseDoc["message"] = "Password deleted successfully!";
                        String jsonResponse;
                        serializeJson(responseDoc, jsonResponse);
                        
                        LOG_INFO("WebServer", "🔐 PASSWORD DELETE ENCRYPTION: Securing tunneled response");
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", jsonResponse, secureLayer);
                        return;
                    } else {
                        JsonDocument errorDoc;
                        errorDoc["status"] = "error";
                        errorDoc["message"] = "Failed to delete password";
                        String errorResponse;
                        serializeJson(errorDoc, errorResponse);
                        
                        WebServerSecureIntegration::sendSecureResponse(request, 500, "application/json", errorResponse, secureLayer);
                        return;
                    }
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/passwords/get POST
                if (targetEndpoint == "/api/passwords/get" && targetMethod == "POST") {
                    if (request->hasHeader("X-User-Activity")) {
                        resetActivityTimer();
                    }
                    
                    int index = targetData["index"].as<int>();
                    
                    LOG_INFO("WebServer", "🚇 TUNNELED Password get: index " + String(index));
                    
                    auto passwords = passwordManager.getAllPasswords();
                    if (index >= 0 && index < passwords.size()) {
                        const auto& pwd = passwords[index];
                        
                        JsonDocument responseDoc;
                        responseDoc["name"] = pwd.name;
                        responseDoc["password"] = pwd.password;
                        String jsonResponse;
                        serializeJson(responseDoc, jsonResponse);
                        
                        LOG_INFO("WebServer", "🔐 PASSWORD GET ENCRYPTION: Securing tunneled password data");
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", jsonResponse, secureLayer);
                        return;
                    } else {
                        JsonDocument errorDoc;
                        errorDoc["status"] = "error";
                        errorDoc["message"] = "Password not found";
                        String errorResponse;
                        serializeJson(errorDoc, errorResponse);
                        
                        WebServerSecureIntegration::sendSecureResponse(request, 404, "application/json", errorResponse, secureLayer);
                        return;
                    }
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/passwords/update POST
                if (targetEndpoint == "/api/passwords/update" && targetMethod == "POST") {
                    if (request->hasHeader("X-User-Activity")) {
                        resetActivityTimer();
                    }
                    
                    int index = targetData["index"].as<int>();
                    String name = targetData["name"].as<String>();
                    String password = targetData["password"].as<String>();
                    
                    LOG_INFO("WebServer", "🚇 TUNNELED Password update: index " + String(index) + ", name: " + name);
                    
                    if (passwordManager.updatePassword(index, name, password)) {
                        LOG_INFO("WebServer", "🔐 Password updated at index: " + String(index));
                        
                        JsonDocument responseDoc;
                        responseDoc["status"] = "success";
                        responseDoc["message"] = "Password updated successfully!";
                        String jsonResponse;
                        serializeJson(responseDoc, jsonResponse);
                        
                        LOG_INFO("WebServer", "🔐 PASSWORD UPDATE ENCRYPTION: Securing tunneled response");
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", jsonResponse, secureLayer);
                        return;
                    } else {
                        JsonDocument errorDoc;
                        errorDoc["status"] = "error";
                        errorDoc["message"] = "Failed to update password";
                        String errorResponse;
                        serializeJson(errorDoc, errorResponse);
                        
                        WebServerSecureIntegration::sendSecureResponse(request, 500, "application/json", errorResponse, secureLayer);
                        return;
                    }
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/passwords/reorder POST
                if (targetEndpoint == "/api/passwords/reorder" && targetMethod == "POST") {
                    if (request->hasHeader("X-User-Activity")) {
                        resetActivityTimer();
                    }
                    
                    if (!targetData["order"].is<JsonArray>()) {
                        return request->send(400, "text/plain", "Missing or invalid 'order' field");
                    }
                    
                    std::vector<std::pair<String, int>> newOrder;
                    JsonArray orderArray = targetData["order"].as<JsonArray>();
                    
                    for (JsonObject item : orderArray) {
                        String name = item["name"].as<String>();
                        int order = item["order"].as<int>();
                        newOrder.push_back(std::make_pair(name, order));
                    }
                    
                    LOG_INFO("WebServer", "🚇 TUNNELED Passwords reorder: " + String(newOrder.size()) + " passwords");
                    bool success = passwordManager.reorderPasswords(newOrder);
                    
                    String output;
                    int statusCode;
                    if (success) {
                        output = "{\"status\":\"success\",\"message\":\"Passwords reordered successfully!\"}";
                        statusCode = 200;
                        LOG_INFO("WebServer", "🚇 Passwords reordered successfully");
                    } else {
                        output = "{\"status\":\"error\",\"message\":\"Failed to reorder passwords\"}";
                        statusCode = 500;
                        LOG_ERROR("WebServer", "🚇 Failed to reorder passwords");
                    }
                    
                    LOG_INFO("WebServer", "🔐 PASSWORDS REORDER ENCRYPTION: Securing tunneled response");
                    WebServerSecureIntegration::sendSecureResponse(request, statusCode, "application/json", output, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/theme GET
                if (targetEndpoint == "/api/theme" && targetMethod == "GET") {
                    if (request->hasHeader("X-User-Activity")) {
                        resetActivityTimer();
                    }
                    
                    Theme currentTheme = configManager.loadTheme();
                    String themeName = (currentTheme == Theme::LIGHT) ? "light" : "dark";
                    JsonDocument doc;
                    doc["theme"] = themeName;
                    String output;
                    serializeJson(doc, output);
                    
                    LOG_INFO("WebServer", "🚇 TUNNELED theme GET: " + themeName);
                    LOG_INFO("WebServer", "🔐 THEME GET ENCRYPTION: Securing tunneled response [TUNNELED]");
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/theme POST
                if (targetEndpoint == "/api/theme" && targetMethod == "POST") {
                    if (request->hasHeader("X-User-Activity")) {
                        resetActivityTimer();
                    }
                    
                    String theme = targetData["theme"].as<String>();
                    
                    if (theme.length() == 0) {
                        JsonDocument errorDoc;
                        errorDoc["success"] = false;
                        errorDoc["message"] = "Theme parameter missing.";
                        String errorResponse;
                        serializeJson(errorDoc, errorResponse);
                        WebServerSecureIntegration::sendSecureResponse(request, 400, "application/json", errorResponse, secureLayer);
                        return;
                    }
                    
                    if (theme != "light" && theme != "dark") {
                        JsonDocument errorDoc;
                        errorDoc["success"] = false;
                        errorDoc["message"] = "Invalid theme. Must be 'light' or 'dark'.";
                        String errorResponse;
                        serializeJson(errorDoc, errorResponse);
                        WebServerSecureIntegration::sendSecureResponse(request, 400, "application/json", errorResponse, secureLayer);
                        return;
                    }
                    
                    Theme newTheme = (theme == "light") ? Theme::LIGHT : Theme::DARK;
                    configManager.saveTheme(newTheme);
                    
                    extern bool pendingThemeChange; extern Theme pendingTheme;
                    pendingThemeChange = true; pendingTheme = newTheme;
                    
                    LOG_INFO("WebServer", "🚇 TUNNELED theme changed to: " + theme);
                    
                    JsonDocument doc;
                    doc["success"] = true;
                    doc["message"] = "Theme updated successfully!";
                    doc["theme"] = theme;
                    String response;
                    serializeJson(doc, response);
                    
                    LOG_INFO("WebServer", "🔐 THEME POST ENCRYPTION: Securing tunneled response [TUNNELED]");
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", response, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/display_settings GET
                if (targetEndpoint == "/api/display_settings" && targetMethod == "GET") {
                    if (request->hasHeader("X-User-Activity")) {
                        resetActivityTimer();
                    }
                    
                    uint16_t displayTimeout = configManager.getDisplayTimeout();
                    uint32_t autoLockTimeout = configManager.getAutoLockTimeout();
                    JsonDocument doc;
                    doc["display_timeout"] = displayTimeout;
                    doc["auto_lock_timeout"] = autoLockTimeout;
                    String output;
                    serializeJson(doc, output);
                    
                    LOG_INFO("WebServer", "🚇 TUNNELED display_settings GET: timeout=" + String(displayTimeout) + " autoLock=" + String(autoLockTimeout));
                    LOG_INFO("WebServer", "🔐 DISPLAY_SETTINGS GET ENCRYPTION: Securing tunneled response [TUNNELED]");
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/rtc GET
                if (targetEndpoint == "/api/rtc" && targetMethod == "GET") {
                    RTCConfig cfg = rtcManager.getConfig();
                    JsonDocument doc;
                    doc["enabled"]        = cfg.enabled;
                    doc["sda_pin"]        = cfg.sda_pin;
                    doc["scl_pin"]        = cfg.scl_pin;
                    doc["available"]      = rtcManager.isAvailable();
                    doc["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
                    if (rtcManager.isAvailable()) {
                        DateTime now = rtcManager.getCurrentTime();
                        char iso[25];
                        snprintf(iso, sizeof(iso), "%04d-%02d-%02dT%02d:%02d:%02dZ",
                                 now.year(), now.month(), now.day(),
                                 now.hour(), now.minute(), now.second());
                        doc["rtc_time"] = iso;
                    } else {
                        doc["rtc_time"] = nullptr;
                    }
                    String out; serializeJson(doc, out);
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", out, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/rtc POST
                if (targetEndpoint == "/api/rtc" && targetMethod == "POST") {
                    RTCConfig cfg;
                    cfg.enabled = targetData["enabled"] | false;
                    cfg.sda_pin = targetData["sda_pin"]  | 21;
                    cfg.scl_pin = targetData["scl_pin"]  | 22;
                    time_t ts   = (time_t)(long)targetData["timestamp"];
                    rtcManager.saveConfig(cfg);
                    bool time_synced = false, available = false;
                    String message = "Config saved";
                    if (cfg.enabled) {
                        if (!rtcManager.isInitialized()) {
                            rtcManager.init();
                        } else if (cfg.sda_pin != rtcManager.getConfig().sda_pin ||
                                   cfg.scl_pin != rtcManager.getConfig().scl_pin) {
                            rtcManager.reinit();
                        }
                        available = rtcManager.isAvailable();
                        if (available && ts > 1609459200) {
                            struct timeval tv = { ts, 0 };
                            settimeofday(&tv, nullptr);
                            rtcManager.syncToRTC(ts);
                            time_synced = true;
                            message = "Config saved and time synchronized";
                        } else if (!available) {
                            message = "Config saved but DS3231 not found on I2C bus";
                        } else {
                            message = "Config saved but timestamp invalid";
                        }
                    }
                    JsonDocument resp;
                    resp["success"]     = true;
                    resp["time_synced"] = time_synced;
                    resp["available"]   = available;
                    resp["message"]     = message;
                    String out; serializeJson(resp, out);
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", out, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/boot-mode GET
                if (targetEndpoint == "/api/boot-mode" && targetMethod == "GET") {
                    String mode = configManager.getBootMode();
                    JsonDocument resp;
                    resp["boot_mode"] = mode;
                    String out; serializeJson(resp, out);
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", out, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/boot-mode POST
                if (targetEndpoint == "/api/boot-mode" && targetMethod == "POST") {
                    String mode = targetData["boot_mode"] | "wifi";
                    if (mode != "wifi" && mode != "ap" && mode != "offline") {
                        WebServerSecureIntegration::sendSecureResponse(request, 400, "application/json", "{\"error\":\"Invalid boot_mode\"}", secureLayer);
                        return;
                    }
                    bool ok = configManager.saveBootMode(mode);
                    JsonDocument resp;
                    resp["success"] = ok;
                    resp["boot_mode"] = mode;
                    String out; serializeJson(resp, out);
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", out, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/clock_settings GET
                if (targetEndpoint == "/api/clock_settings" && targetMethod == "GET") {
                    JsonDocument doc;
                    doc["timezone"] = configManager.getTimezone();
                    String response;
                    serializeJson(doc, response);
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", response, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/clock_settings POST
                if (targetEndpoint == "/api/clock_settings" && targetMethod == "POST") {
                    if (request->hasHeader("X-User-Activity")) resetActivityTimer();
                    
                    String tz = targetData["timezone"].as<String>();
                    JsonDocument doc;
                    String response;
                    int statusCode = 200;
                    
                    if (tz.length() == 0) {
                        doc["success"] = false;
                        doc["message"] = "Missing timezone parameter!";
                        statusCode = 400;
                    } else if (tz.length() > 64) {
                        doc["success"] = false;
                        doc["message"] = "Invalid timezone value!";
                        statusCode = 400;
                    } else if (configManager.saveTimezone(tz)) {
                        setenv("TZ", tz.c_str(), 1);
                        tzset();
                        LOG_INFO("WebServer", "🚇 Tunneled timezone changed to: " + tz);
                        doc["success"] = true;
                        doc["message"] = "Timezone saved successfully!";
                        doc["timezone"] = tz;
                    } else {
                        doc["success"] = false;
                        doc["message"] = "Failed to save timezone!";
                        statusCode = 500;
                    }
                    
                    serializeJson(doc, response);
                    WebServerSecureIntegration::sendSecureResponse(request, statusCode, "application/json", response, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/display_settings POST
                if (targetEndpoint == "/api/display_settings" && targetMethod == "POST") {
                    if (request->hasHeader("X-User-Activity")) {
                        resetActivityTimer();
                    }
                    
                    String timeoutStr = targetData["display_timeout"].as<String>();
                    
                    if (timeoutStr.length() == 0) {
                        JsonDocument errorDoc;
                        errorDoc["success"] = false;
                        errorDoc["message"] = "Missing display_timeout parameter!";
                        String errorResponse;
                        serializeJson(errorDoc, errorResponse);
                        WebServerSecureIntegration::sendSecureResponse(request, 400, "application/json", errorResponse, secureLayer);
                        return;
                    }
                    
                    uint16_t timeout = timeoutStr.toInt();
                    
                    if (timeout != 0 && timeout != 15 && timeout != 30 && timeout != 60 && 
                        timeout != 300 && timeout != 1800) {
                        JsonDocument errorDoc;
                        errorDoc["success"] = false;
                        errorDoc["message"] = "Invalid timeout value!";
                        String errorResponse;
                        serializeJson(errorDoc, errorResponse);
                        WebServerSecureIntegration::sendSecureResponse(request, 400, "application/json", errorResponse, secureLayer);
                        return;
                    }
                    
                    String autoLockStr = targetData["auto_lock_timeout"].as<String>();
                    uint32_t autoLockTimeout = (autoLockStr.length() > 0 && autoLockStr != "null") ? (uint32_t)autoLockStr.toInt() : configManager.getAutoLockTimeout();
                    
                    if (autoLockTimeout != 0 && autoLockTimeout != 300 && autoLockTimeout != 900 &&
                        autoLockTimeout != 1800 && autoLockTimeout != 3600 && autoLockTimeout != 14400) {
                        JsonDocument errorDoc;
                        errorDoc["success"] = false;
                        errorDoc["message"] = "Invalid auto lock timeout value!";
                        String errorResponse;
                        serializeJson(errorDoc, errorResponse);
                        WebServerSecureIntegration::sendSecureResponse(request, 400, "application/json", errorResponse, secureLayer);
                        return;
                    }
                    
                    if (timeout > 0 && autoLockTimeout > 0 && autoLockTimeout <= timeout) {
                        JsonDocument errorDoc;
                        errorDoc["success"] = false;
                        errorDoc["message"] = "Auto lock timeout must be greater than screen timeout!";
                        String errorResponse;
                        serializeJson(errorDoc, errorResponse);
                        WebServerSecureIntegration::sendSecureResponse(request, 400, "application/json", errorResponse, secureLayer);
                        return;
                    }
                    
                    JsonDocument doc;
                    int statusCode;
                    
                    if (configManager.saveDisplayTimeout(timeout) && configManager.saveAutoLockTimeout(autoLockTimeout)) {
                        doc["success"] = true;
                        doc["message"] = "Display settings saved successfully!";
                        doc["timeout"] = timeout;
                        doc["auto_lock_timeout"] = autoLockTimeout;
                        statusCode = 200;
                        LOG_INFO("WebServer", "🚇 TUNNELED display timeout=" + String(timeout) + "s autoLock=" + String(autoLockTimeout) + "s");
                    } else {
                        doc["success"] = false;
                        doc["message"] = "Failed to save display settings!";
                        statusCode = 500;
                        LOG_ERROR("WebServer", "🚇 TUNNELED Failed to save display settings");
                    }
                    
                    String response;
                    serializeJson(doc, response);
                    
                    LOG_INFO("WebServer", "🔐 DISPLAY_SETTINGS POST ENCRYPTION: Securing tunneled response [TUNNELED]");
                    WebServerSecureIntegration::sendSecureResponse(request, statusCode, "application/json", response, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/pincode_settings GET
                if (targetEndpoint == "/api/pincode_settings" && targetMethod == "GET") {
                    if (request->hasHeader("X-User-Activity")) {
                        resetActivityTimer();
                    }
                    
                    pinManager.loadPinConfig();
                    
                    // Проверяем текущее состояние device key
                    bool pinProtectionEnabled = CryptoManager::getInstance().isDeviceKeyEncrypted();
                    bool deviceKeyExists = CryptoManager::getInstance().isDeviceKeyFileExists();
                    
                    // Проверяем статус Device BLE PIN
                    bool deviceBlePinEnabled = CryptoManager::getInstance().isDeviceBlePinEnabled();
                    bool deviceBlePinConfigured = CryptoManager::getInstance().isDeviceBlePinConfigured();
                    
                    JsonDocument doc;
                    doc["pinProtectionEnabled"] = pinProtectionEnabled;
                    doc["deviceKeyExists"] = deviceKeyExists;
                    doc["length"] = pinManager.getPinLength();
                    
                    // Device BLE PIN status
                    doc["deviceBlePinEnabled"] = deviceBlePinEnabled;
                    doc["deviceBlePinConfigured"] = deviceBlePinConfigured;
                    
                    // Legacy fields для обратной совместимости
                    doc["enabled"] = pinProtectionEnabled;
                    doc["enabledForDevice"] = pinProtectionEnabled;
                    
                    String output;
                    serializeJson(doc, output);
                    
                    LOG_INFO("WebServer", "🚇 TUNNELED pincode_settings GET");
                    LOG_INFO("WebServer", "🔐 PINCODE_SETTINGS GET ENCRYPTION: Securing tunneled response [TUNNELED]");
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/pincode_settings POST
                if (targetEndpoint == "/api/pincode_settings" && targetMethod == "POST") {
                    if (request->hasHeader("X-User-Activity")) {
                        resetActivityTimer();
                    }
                    
                    bool enablePinProtection = targetData["enabledForDevice"].as<bool>();
                    bool enabledForBle = targetData["enabledForBle"].as<bool>();
                    int pinLength = targetData["length"].as<int>();
                    String newPin = targetData["pin"].as<String>();
                    String confirmPin = targetData["pin_confirm"].as<String>();
                    String currentPin = targetData["current_pin"].as<String>();
                    bool factoryResetConfirmed = targetData["factory_reset_confirmed"].as<bool>();
                    
                    LOG_INFO("WebServer", "🚇 TUNNELED PIN settings update: device=" + String(enablePinProtection) + ", ble=" + String(enabledForBle) + ", len=" + String(pinLength));
                    
                    String message;
                    int statusCode;
                    bool success;
                    bool requiresReboot = false;
                    
                    // Проверяем текущее состояние device key
                    bool currentlyEncrypted = CryptoManager::getInstance().isDeviceKeyEncrypted();
                    
                    if (enablePinProtection) {
                        // ВКЛЮЧЕНИЕ PIN ЗАЩИТЫ
                        
                        if (currentlyEncrypted) {
                            // Device key уже зашифрован - меняем PIN
                            LOG_INFO("WebServer", "🚇 TUNNELED: Changing PIN encryption...");
                            
                            if (currentPin.length() == 0) {
                                message = "Current PIN is required to change PIN.";
                                statusCode = 400;
                                success = false;
                            } else if (CryptoManager::getInstance().changePinEncryption(currentPin, newPin)) {
                                message = "PIN changed successfully! Device will reboot.";
                                statusCode = 200;
                                success = true;
                                requiresReboot = true;
                                LOG_INFO("WebServer", "🚇 TUNNELED: PIN changed successfully");
                            } else {
                                message = "Failed to change PIN. Current PIN may be incorrect.";
                                statusCode = 400;
                                success = false;
                                LOG_ERROR("WebServer", "🚇 TUNNELED: Failed to change PIN");
                            }
                        } else {
                            // Device key не зашифрован - включаем защиту через factory reset
                            LOG_WARNING("WebServer", "🚇 TUNNELED: Enabling PIN protection requires factory reset!");
                            
                            if (!factoryResetConfirmed) {
                                message = "WARNING: Enabling PIN protection will perform FACTORY RESET and delete ALL data (TOTP keys, passwords, WiFi). Export your data first! Send 'factory_reset_confirmed=true' to proceed.";
                                statusCode = 400;
                                success = false;
                                LOG_WARNING("WebServer", "🚇 TUNNELED: Factory reset not confirmed - aborting PIN enable");
                            } else {
                                // Удаляем все данные (полная очистка как Hardware Reset)
                                LOG_INFO("WebServer", "🚇 TUNNELED: Starting full factory reset...");
                                
                                LittleFS.remove(KEYS_FILE);
                                LittleFS.remove(PASSWORD_FILE);
                                LittleFS.remove("/wifi_config.json");
                                LittleFS.remove("/wifi_config.json.enc");
                                LittleFS.remove("/splash_config.json");
                                LittleFS.remove(PIN_FILE);
                                LittleFS.remove(CONFIG_FILE);
                                LittleFS.remove(DEVICE_KEY_FILE);
                                LittleFS.remove(BLE_CONFIG_FILE);
                                LittleFS.remove(WEB_ADMIN_FILE);
                                LittleFS.remove(MDNS_CONFIG_FILE);
                                LittleFS.remove(LOGIN_STATE_FILE);
                                LittleFS.remove("/rtc_config.json");
                                LittleFS.remove("/ble_pin.json.enc");
                                LittleFS.remove("/session.json.enc");
                                
                                // URL Obfuscation: Удаление boot counter и всех mappings
                                LOG_INFO("WebServer", "🚇 TUNNELED: Clearing URL obfuscation data...");
                                LittleFS.remove("/boot_counter.txt");
                                
                                // Удаляем все url_mappings_*.json файлы
                                fs::File root = LittleFS.open("/", "r");
                                if (root) {
                                    fs::File file = root.openNextFile();
                                    while (file) {
                                        String filename = String(file.name());
                                        if (filename.startsWith("/url_mappings_") && filename.endsWith(".json")) {
                                            LOG_DEBUG("WebServer", "🚇 TUNNELED: Removing URL mapping file: " + filename);
                                            LittleFS.remove(filename);
                                        }
                                        file = root.openNextFile();
                                    }
                                }
                                
                                // Очистка NVS partition (BLE bonding keys)
                                LOG_INFO("WebServer", "🚇 TUNNELED: Clearing NVS partition...");
                                nvs_flash_erase_partition("nvs");
                                
                                LOG_INFO("WebServer", "🚇 TUNNELED: All data files deleted for factory reset");
                                
                                // НЕ создаем ключ здесь - устройство создаст его при следующей загрузке
                                // и запросит PIN на физическом экране
                                
                                // Длина PIN выбирается на устройстве при первой загрузке
                                
                                message = "Factory reset completed! Device will reboot. On next boot, choose PIN length and create PIN on device screen.";
                                statusCode = 200;
                                success = true;
                                requiresReboot = true;
                                LOG_INFO("WebServer", "🚇 TUNNELED: Factory reset completed - device will prompt for PIN length and PIN on next boot");
                            }
                        }
                    } else {
                        // ОТКЛЮЧЕНИЕ PIN ЗАЩИТЫ
                        
                        if (!currentlyEncrypted) {
                            message = "PIN protection is already disabled.";
                            statusCode = 200;
                            success = false;
                            LOG_INFO("WebServer", "🚇 TUNNELED: PIN protection already disabled");
                        } else {
                            // Новая логика: устанавливаем глобальный флаг для запроса PIN на устройстве
                            LOG_INFO("WebServer", "🚇 TUNNELED: PIN disable requested - will prompt on device");
                            
                            extern bool shouldPromptPinDisable;
                            shouldPromptPinDisable = true;
                            
                            message = "Please enter PIN on device to confirm. Check device screen.";
                            statusCode = 200;
                            success = true;
                            requiresReboot = false; // Не перезагружаем сразу
                            LOG_INFO("WebServer", "🚇 TUNNELED: PIN disable flag set - device will prompt for PIN");
                        }
                    }
                    
                    // BLE PIN настройки (независимо от device PIN)
                    pinManager.setPinEnabledForBle(enabledForBle);
                    pinManager.saveConfig();
                    
                    // Формируем JSON ответ
                    JsonDocument doc;
                    doc["success"] = success;
                    doc["message"] = message;
                    doc["requiresReboot"] = requiresReboot;
                    String response;
                    serializeJson(doc, response);
                    
                    // Если требуется перезагрузка, планируем её
                    if (requiresReboot && success) {
                        extern bool shouldRestart;
                        shouldRestart = true;
                    }
                    
                    LOG_INFO("WebServer", "🔐 PINCODE_SETTINGS POST ENCRYPTION: Securing tunneled response [TUNNELED]");
                    WebServerSecureIntegration::sendSecureResponse(request, statusCode, "application/json", response, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/ble_pin_update POST
                if (targetEndpoint == "/api/ble_pin_update" && targetMethod == "POST") {
                    // DEBUG: Логируем весь targetData
                    String targetDataDebug;
                    serializeJson(targetData, targetDataDebug);
                    LOG_DEBUG("WebServer", "🚇 [TUNNELED] targetData JSON: " + targetDataDebug);
                    
                    // Новая логика: поддержка двух типов PIN
                    String bleBondingPinStr = targetData["ble_bonding_pin"].as<String>();
                    String deviceBlePinStr = targetData["device_ble_pin"].as<String>();
                    String deviceBlePinEnabledStr = targetData["device_ble_pin_enabled"].as<String>();
                    bool deviceBlePinEnabled = (deviceBlePinEnabledStr == "true");
                    bool deviceBlePinEnabledProvided = (deviceBlePinEnabledStr.length() > 0 && deviceBlePinEnabledStr != "null"); // Параметр был передан
                    
                    // Проверка на null/пустые значения
                    bool hasBondingPin = (bleBondingPinStr.length() > 0 && bleBondingPinStr != "null");
                    bool hasDevicePin = (deviceBlePinStr.length() > 0 && deviceBlePinStr != "null");
                    
                    LOG_INFO("WebServer", "🚇 TUNNELED BLE PIN update: bonding=" + String(hasBondingPin) + ", device=" + String(hasDevicePin));
                    LOG_DEBUG("WebServer", "🚇 PIN values protected in logs");
                    
                    String message;
                    int statusCode = 200;
                    bool success = true;
                    
                    // Обработка BLE Bonding PIN
                    if (hasBondingPin) {
                        if (bleBondingPinStr.length() != 6) {
                            message = "BLE Bonding PIN must be exactly 6 digits";
                            statusCode = 400;
                            success = false;
                        } else {
                            bool validDigits = true;
                            for (char c : bleBondingPinStr) {
                                if (!isdigit(c)) {
                                    validDigits = false;
                                    break;
                                }
                            }
                            
                            if (!validDigits) {
                                message = "BLE Bonding PIN must contain only digits";
                                statusCode = 400;
                                success = false;
                            } else {
                                uint32_t blePin = bleBondingPinStr.toInt();
                                
                                if (CryptoManager::getInstance().saveBlePin(blePin)) {
                                    LOG_INFO("WebServer", "🚇 TUNNELED BLE Bonding PIN updated successfully");
                                    
                                    // Clear all BLE bonding keys when PIN changes for security
                                    if (bleKeyboardManager) {
                                        bleKeyboardManager->clearBondingKeys();
                                        LOG_INFO("WebServer", "🚇 TUNNELED BLE bonding keys cleared");
                                    }
                                    
                                    message = "BLE Bonding PIN updated successfully! All BLE clients cleared.";
                                } else {
                                    LOG_ERROR("WebServer", "🚇 TUNNELED Failed to save BLE Bonding PIN");
                                    message = "Failed to save BLE Bonding PIN";
                                    statusCode = 500;
                                    success = false;
                                }
                            }
                        }
                    }
                    
                    // Обработка Device BLE PIN
                    if (success && hasDevicePin) {
                        if (deviceBlePinStr.length() != 6) {
                            message = "Device BLE PIN must be exactly 6 digits";
                            statusCode = 400;
                            success = false;
                        } else {
                            bool validDigits = true;
                            for (char c : deviceBlePinStr) {
                                if (!isdigit(c)) {
                                    validDigits = false;
                                    break;
                                }
                            }
                            
                            if (!validDigits) {
                                message = "Device BLE PIN must contain only digits";
                                statusCode = 400;
                                success = false;
                            } else {
                                uint32_t devicePin = deviceBlePinStr.toInt();
                                
                                if (deviceBlePinStr == "000000") {
                                    devicePin = 0;
                                }
                                
                                LOG_INFO("WebServer", "🚇 Device BLE PIN validated, saving (value protected)");
                                
                                if (CryptoManager::getInstance().saveDeviceBlePin(devicePin)) {
                                    LOG_INFO("WebServer", "🚇 TUNNELED Device BLE PIN updated successfully");
                                    
                                    if (hasBondingPin) {
                                        message += " Device BLE PIN also updated.";
                                    } else {
                                        message = "Device BLE PIN updated successfully!";
                                    }
                                } else {
                                    LOG_ERROR("WebServer", "🚇 TUNNELED Failed to save Device BLE PIN");
                                    message = "Failed to save Device BLE PIN";
                                    statusCode = 500;
                                    success = false;
                                }
                            }
                        }
                    } else if (success && !hasDevicePin && deviceBlePinEnabledProvided && deviceBlePinEnabled == false) {
                        // Выключение Device BLE PIN (только enabled=false без PIN, и параметр ЯВНО передан)
                        if (CryptoManager::getInstance().isDeviceBlePinConfigured()) {
                            CryptoManager::getInstance().setDeviceBlePinEnabled(false);
                            LOG_INFO("WebServer", "🚇 TUNNELED Device BLE PIN disabled successfully");
                            message = "Device BLE PIN disabled successfully!";
                            hasDevicePin = true; // Помечаем что операция выполнена
                        } else {
                            message = "Device BLE PIN not configured";
                            statusCode = 400;
                            success = false;
                        }
                    }
                    
                    if (!hasBondingPin && !hasDevicePin) {
                        message = "No PIN parameters provided";
                        statusCode = 400;
                        success = false;
                    }
                    
                    JsonDocument doc;
                    doc["success"] = success;
                    doc["message"] = message;
                    String response;
                    serializeJson(doc, response);
                    
                    LOG_INFO("WebServer", "🔐 BLE_PIN_UPDATE POST ENCRYPTION: Securing tunneled response, success=" + String(success));
                    WebServerSecureIntegration::sendSecureResponse(request, statusCode, "application/json", response, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/clear_ble_clients POST
                if (targetEndpoint == "/api/clear_ble_clients" && targetMethod == "POST") {
                    LOG_INFO("WebServer", "🚇 TUNNELED Clear BLE clients request");
                    
                    if (bleKeyboardManager) {
                        bleKeyboardManager->clearBondingKeys();
                        LOG_INFO("WebServer", "🚇 TUNNELED BLE bonding keys cleared successfully");
                        
                        String output = "{\"success\":true,\"message\":\"BLE clients cleared successfully!\"}";
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                    } else {
                        String output = "{\"success\":false,\"message\":\"BLE manager not available\"}";
                        WebServerSecureIntegration::sendSecureResponse(request, 500, "application/json", output, secureLayer);
                    }
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/config GET
                if (targetEndpoint == "/api/config" && targetMethod == "GET") {
                    uint16_t timeout = configManager.getWebServerTimeout();
                    String output;
                    output.reserve(40);
                    output = "{\"web_server_timeout\":";
                    output += String(timeout);
                    output += "}";
                    
                    LOG_DEBUG("WebServer", "🚇 TUNNELED config: timeout=" + String(timeout));
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/startup_mode GET
                if (targetEndpoint == "/api/startup_mode" && targetMethod == "GET") {
                    String mode = configManager.getStartupMode();
                    String output;
                    output.reserve(30 + mode.length());
                    output = "{\"mode\":\"";
                    output += mode;
                    output += "\"}";
                    
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/startup_mode POST
                if (targetEndpoint == "/api/startup_mode" && targetMethod == "POST") {
                    String mode = targetData["mode"].as<String>();
                    
                    if (mode.length() == 0) {
                        return request->send(400, "text/plain", "Missing mode parameter.");
                    }
                    if (mode != "totp" && mode != "password") {
                        return request->send(400, "text/plain", "Invalid startup mode. Must be 'totp' or 'password'.");
                    }
                    
                    bool success = configManager.saveStartupMode(mode);
                    String message = success ? "Startup mode saved successfully!" : "Failed to save startup mode.";
                    String output;
                    output.reserve(50 + message.length());
                    output = "{\"success\":";
                    output += success ? "true" : "false";
                    output += ",\"message\":\"";
                    output += message;
                    output += "\"}";
                    
                    WebServerSecureIntegration::sendSecureResponse(request, success ? 200 : 500, "application/json", output, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/settings GET
                if (targetEndpoint == "/api/settings" && targetMethod == "GET") {
                    uint16_t timeout = configManager.getWebServerTimeout();
                    String username = WebAdminManager::getInstance().getUsername();
                    String output;
                    output.reserve(60 + username.length());
                    output = "{\"web_server_timeout\":";
                    output += String(timeout);
                    output += ",\"admin_login\":\"";
                    output += username;
                    output += "\"}";
                    
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/settings POST
                if (targetEndpoint == "/api/settings" && targetMethod == "POST") {
                    String timeoutStr = targetData["web_server_timeout"].as<String>();
                    
                    if (timeoutStr.length() == 0) {
                        return request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing parameters.\"}");
                    }
                    
                    uint16_t timeout = timeoutStr.toInt();
                    configManager.setWebServerTimeout(timeout);
                    _timeoutMinutes = timeout;
                    
                    String output = "{\"success\":true,\"message\":\"Settings updated successfully! Device will restart...\"}";
                    
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                    
                    // Schedule restart
                    extern bool shouldRestart;
                    shouldRestart = true;
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/ble_settings GET
                if (targetEndpoint == "/api/ble_settings" && targetMethod == "GET") {
                    String deviceName = configManager.loadBleDeviceName();
                    String output;
                    output.reserve(30 + deviceName.length());
                    output = "{\"device_name\":\"";
                    output += deviceName;
                    output += "\"}";
                    
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/ble_settings POST
                if (targetEndpoint == "/api/ble_settings" && targetMethod == "POST") {
                    String deviceName = targetData["device_name"].as<String>();
                    
                    if (deviceName.length() == 0) {
                        return request->send(400, "text/plain", "Device name parameter missing.");
                    }
                    if (deviceName.length() > 15) {
                        return request->send(400, "text/plain", "Device name too long (max 15 characters)");
                    }
                    
                    configManager.saveBleDeviceName(deviceName);
                    if (bleKeyboardManager) {
                        bleKeyboardManager->setDeviceName(deviceName);
                    }
                    
                    String output = "{\"success\":true,\"message\":\"BLE device name updated successfully!\"}";
                    
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/mdns_settings GET
                if (targetEndpoint == "/api/mdns_settings" && targetMethod == "GET") {
                    String hostname = configManager.loadMdnsHostname();
                    String output;
                    output.reserve(30 + hostname.length());
                    output = "{\"hostname\":\"";
                    output += hostname;
                    output += "\"}";
                    
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/mdns_settings POST
                if (targetEndpoint == "/api/mdns_settings" && targetMethod == "POST") {
                    String hostname = targetData["hostname"].as<String>();
                    
                    if (hostname.length() == 0) {
                        return request->send(400, "text/plain", "Hostname parameter missing.");
                    }
                    if (hostname.length() > 63) {
                        return request->send(400, "text/plain", "Invalid hostname length (1-63 characters)");
                    }
                    
                    configManager.saveMdnsHostname(hostname);
                    if (wifiManager) {
                        wifiManager->updateMdnsHostname();
                    }
                    
                    String output = "{\"success\":true,\"message\":\"mDNS hostname updated successfully!\"}";
                    
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/session_duration GET
                if (targetEndpoint == "/api/session_duration" && targetMethod == "GET") {
                    ConfigManager::SessionDuration duration = configManager.getSessionDuration();
                    int durationValue = static_cast<int>(duration);
                    String output;
                    output.reserve(30);
                    output = "{\"duration\":";
                    output += String(durationValue);
                    output += "}";
                    
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/session_duration POST
                if (targetEndpoint == "/api/session_duration" && targetMethod == "POST") {
                    String durationStr = targetData["duration"].as<String>();
                    
                    if (durationStr.length() == 0) {
                        return request->send(400, "text/plain", "Duration parameter missing.");
                    }
                    
                    int durationValue = durationStr.toInt();
                    
                    if (durationValue == 0 || durationValue == 1 || durationValue == 6 || 
                        durationValue == 24 || durationValue == 72) {
                        
                        ConfigManager::SessionDuration duration = static_cast<ConfigManager::SessionDuration>(durationValue);
                        configManager.setSessionDuration(duration);
                        
                        String output = "{\"success\":true,\"message\":\"Session duration updated successfully!\"}";
                        
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        return;
                    } else {
                        return request->send(400, "text/plain", "Invalid session duration value.");
                    }
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /logout POST
                if (targetEndpoint == "/logout" && targetMethod == "POST") {
                    LOG_INFO("WebServer", "🚇 TUNNELED logout request");
                    
                    // Полная очистка сессии (включая персистентную)
                    clearSession();
                    clearSecureSession();
                    
                    LOG_INFO("WebServer", "🚇 Session cleared (memory + persistent storage)");
                    
                    String output = "{\"success\":true,\"message\":\"Logged out successfully\"}";
                    
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/change_password POST
                if (targetEndpoint == "/api/change_password" && targetMethod == "POST") {
                    LOG_INFO("WebServer", "🚇 TUNNELED change_password request");
                    
                    String newPassword = targetData["password"].as<String>();
                    
                    if (newPassword.length() == 0) {
                        return request->send(400, "text/plain", "Password parameter missing.");
                    }
                    if (newPassword.length() < 4) {
                        return request->send(400, "text/plain", "Password must be at least 4 characters long.");
                    }
                    
                    String output;
                    if (WebAdminManager::getInstance().changePassword(newPassword)) {
                        output = "Password changed successfully!";
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "text/plain", output, secureLayer);
                    } else {
                        return request->send(500, "text/plain", "Failed to save new password.");
                    }
                    return;
                }
                
                if (targetEndpoint == "/api/wifi_credentials" && targetMethod == "POST") {
                    String ssid = targetData["ssid"].as<String>();
                    String password = targetData["password"].as<String>();
                    String confirm = targetData["confirm_password"].as<String>();
                    
                    if (ssid.length() == 0) {
                        String out = "{\"success\":false,\"message\":\"SSID is required\"}";
                        WebServerSecureIntegration::sendSecureResponse(request, 400,
                            "application/json", out, secureLayer);
                        return;
                    }
                    if (password != confirm) {
                        String out = "{\"success\":false,\"message\":\"Passwords do not match\"}";
                        WebServerSecureIntegration::sendSecureResponse(request, 400,
                            "application/json", out, secureLayer);
                        return;
                    }
                    if (password.length() > 0 && password.length() < 8) {
                        String out = "{\"success\":false,\"message\":\"Password must be at least 8 characters\"}";
                        WebServerSecureIntegration::sendSecureResponse(request, 400,
                            "application/json", out, secureLayer);
                        return;
                    }
                    
                    String out;
                    if (wifiManager && wifiManager->saveCredentials(ssid, password)) {
                        out = "{\"success\":true,\"message\":\"WiFi credentials saved. Reboot to apply.\"}";
                        WebServerSecureIntegration::sendSecureResponse(request, 200,
                            "application/json", out, secureLayer);
                    } else {
                        out = "{\"success\":false,\"message\":\"Failed to save WiFi credentials\"}";
                        WebServerSecureIntegration::sendSecureResponse(request, 500,
                            "application/json", out, secureLayer);
                    }
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/change_ap_password POST
                if (targetEndpoint == "/api/change_ap_password" && targetMethod == "POST") {
                    LOG_INFO("WebServer", "🚇 TUNNELED change_ap_password request");
                    
                    String newPassword = targetData["password"].as<String>();
                    
                    if (newPassword.length() == 0) {
                        return request->send(400, "text/plain", "Password parameter missing.");
                    }
                    if (newPassword.length() < 8) {
                        return request->send(400, "text/plain", "WiFi password must be at least 8 characters long.");
                    }
                    
                    String output;
                    if (configManager.saveApPassword(newPassword)) {
                        output = "{\"success\":true,\"message\":\"WiFi AP password updated. Device will restart in 3 seconds...\",\"restart\":true}";
                        LOG_INFO("WebServer", "AP password changed successfully - scheduling restart");
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        delay(3000);
                        ESP.restart();
                    } else {
                        return request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save new AP password.\"}");
                    }
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/reboot POST
                if (targetEndpoint == "/api/reboot" && targetMethod == "POST") {
                    LOG_INFO("WebServer", "🚇 TUNNELED reboot request");
                    
                    String output = "{\"success\":true,\"message\":\"Rebooting\"}";
                    
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                    
                    delay(1000);
                    ESP.restart();
                    return;
                }
                
                // 🎯 МАРШРУТИЗАЦИЯ: /api/reboot_with_web POST
                if (targetEndpoint == "/api/reboot_with_web" && targetMethod == "POST") {
                    LOG_INFO("WebServer", "🚇 TUNNELED reboot_with_web request");
                    
                    configManager.setWebServerAutoStart(true);
                    LOG_INFO("WebServer", "Web server auto-start flag set successfully");
                    
                    String output = "{\"success\":true,\"message\":\"Rebooting with web server\"}";
                    
                    WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                    
                    delay(1000);
                    ESP.restart();
                    return;
                }
                
                // Activity timer reset
                if (targetEndpoint == "/api/activity" && targetMethod == "POST") {
                    if (!isAuthenticated(request)) return request->send(401);
                    resetActivityTimer();
                    request->send(200, "application/json", "{\"status\":\"success\"}");
                    return;
                }
                
                if (targetEndpoint == "/api/battery" && targetMethod == "GET") {
                    if (!batteryManager) {
                        WebServerSecureIntegration::sendSecureResponse(request, 503, 
                            "application/json", 
                            "{\"error\":\"Battery manager not available\"}", secureLayer);
                        return;
                    }
                    JsonDocument doc;
                    doc["level"] = batteryManager->getPercentage();
                    doc["charging"] = (batteryManager->getVoltage() > 4.15f);
                    String out;
                    serializeJson(doc, out);
                    WebServerSecureIntegration::sendSecureResponse(request, 200, 
                        "application/json", out, secureLayer);
                    return;
                }
                
                // Fallback для неподдерживаемых endpoints
                LOG_WARNING("WebServer", "🚇 Unsupported tunnel target: " + targetEndpoint);
                
                // 🗑️ Очистка памяти
                if (bufferPtr) {
                    delete bufferPtr;
                    request->_tempObject = nullptr;
                }
                
                request->send(404, "text/plain", "Tunnel target not supported: " + targetEndpoint);
                return;
                
            } else {
                LOG_ERROR("WebServer", "🔐 Failed to decrypt tunnel body or invalid session");
                
                // 🗑️ Очистка памяти
                if (bufferPtr) {
                    delete bufferPtr;
                    request->_tempObject = nullptr;
                }
                
                return request->send(400, "text/plain", "Decryption failed or invalid session");
            }
#else
            // Без SECURE_LAYER_ENABLED
            if (bufferPtr) {
                delete bufferPtr;
                request->_tempObject = nullptr;
            }
            request->send(500, "text/plain", "Tunneling requires SECURE_LAYER_ENABLED");
#endif
            // 🗑️ Очистка памяти перед завершением
            if (bufferPtr) {
                delete bufferPtr;
                request->_tempObject = nullptr;
            }
        }
    });
    
    // 🔗 Регистрируем обфусцированный URL для /api/tunnel
    String obfuscatedTunnelPath = urlObfuscation.obfuscateURL("/api/tunnel");
    if (obfuscatedTunnelPath.length() > 0 && obfuscatedTunnelPath != "/api/tunnel") {
        LOG_INFO("WebServer", "🔗 Registering obfuscated tunnel path: " + obfuscatedTunnelPath);
        
        // Повторяем ту же логику для обфусцированного пути
        server.on(obfuscatedTunnelPath.c_str(), HTTP_POST, [this, &methodTunneling](AsyncWebServerRequest *request) {
            if (!isAuthenticated(request)) return request->send(401);
            if (!request->hasHeader("X-Real-Method")) {
                request->send(400, "text/plain", "Missing X-Real-Method header");
                return;
            }
            String encryptedMethod = request->getHeader("X-Real-Method")->value();
            String clientId = WebServerSecureIntegration::getClientId(request);
            String realMethod = methodTunneling.decryptMethodHeader(encryptedMethod, clientId);
            if (realMethod.isEmpty()) {
                request->send(400, "text/plain", "Failed to decrypt method header");
                return;
            }
        }, NULL, [this, &methodTunneling](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            // Та же логика onBody - копируем из основного handler
            String* bufferPtr = nullptr;
            if (index == 0) {
                bufferPtr = new String();
                bufferPtr->reserve(total + 10);
                request->_tempObject = bufferPtr;
            } else {
                bufferPtr = (String*)request->_tempObject;
            }
            if (bufferPtr) {
                bufferPtr->concat((char*)data, len);
            }
            if (index + len >= total) {
                if (!isAuthenticated(request)) {
                    if (bufferPtr) {
                        delete bufferPtr;
                        request->_tempObject = nullptr;
                    }
                    return request->send(401);
                }
                if (!request->hasHeader("X-Real-Method")) {
                    if (bufferPtr) {
                        delete bufferPtr;
                        request->_tempObject = nullptr;
                    }
                    return request->send(400, "text/plain", "Missing X-Real-Method header");
                }
                String encryptedMethod = request->getHeader("X-Real-Method")->value();
                String clientId = WebServerSecureIntegration::getClientId(request);
                String realMethod = methodTunneling.decryptMethodHeader(encryptedMethod, clientId);
                if (realMethod.isEmpty()) {
                    if (bufferPtr) {
                        delete bufferPtr;
                        request->_tempObject = nullptr;
                    }
                    request->send(400, "text/plain", "Failed to decrypt method header");
                    return;
                }
                LOG_DEBUG("WebServer", "🔗 Obfuscated tunnel: " + realMethod + " [Client:" + 
                         (clientId.length() > 0 ? clientId.substring(0,8) + "...]" : "NONE]"));
#ifdef SECURE_LAYER_ENABLED
                String encryptedBody = *bufferPtr;
                String decryptedTunnelBody;
                
                if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId) &&
                    secureLayer.decryptRequest(clientId, encryptedBody, decryptedTunnelBody)) {
                    
                    JsonDocument tunnelDoc;
                    DeserializationError error = deserializeJson(tunnelDoc, decryptedTunnelBody);
                    
                    if (error) {
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return request->send(400, "text/plain", "Invalid tunnel body JSON");
                    }
                    
                    String targetEndpoint = tunnelDoc["endpoint"].as<String>();
                    String targetMethod = tunnelDoc["method"] | realMethod;
                    JsonObject targetData = tunnelDoc["data"].as<JsonObject>();
                    
                    LOG_DEBUG("WebServer", "🎯 Obfuscated: " + targetMethod + " " + targetEndpoint);
                    
                    // /api/keys GET
                    if (targetEndpoint == "/api/keys" && targetMethod == "GET") {
                        if (request->hasHeader("X-User-Activity")) resetActivityTimer();
                        JsonDocument doc;
                        JsonArray keysArray = doc.to<JsonArray>();
                        auto keys = keyManager.getAllKeys();
                        
                        // 🔐 Блокировка TOTP в AP/Offline режимах
                        bool blockTOTP = !totpGenerator.isTimeSynced();
                        
                        for (size_t i = 0; i < keys.size(); i++) {
                            JsonObject keyObj = keysArray.add<JsonObject>();
                            keyObj["name"] = keys[i].name;
                            // HOTP works without time sync, TOTP requires it
                            if (keys[i].type == TOTPType::HOTP) {
                                keyObj["code"] = totpGenerator.generateCode(keys[i]);
                            } else {
                                keyObj["code"] = blockTOTP ? "NOT SYNCED" : totpGenerator.generateCode(keys[i]);
                            }
                            
                            if (keys[i].type == TOTPType::TOTP) {
                                keyObj["timeLeft"] = totpGenerator.getTimeRemaining(keys[i].period);
                            } else {
                                keyObj["timeLeft"] = 0;
                            }
                            
                            // Метаданные (всегда включаем)
                            keyObj["t"] = (keys[i].type == TOTPType::HOTP) ? "H" : "T";
                            keyObj["a"] = (keys[i].algorithm == TOTPAlgorithm::SHA256) ? "256" :
                                          (keys[i].algorithm == TOTPAlgorithm::SHA512) ? "512" : "1";
                            keyObj["d"] = keys[i].digits;
                            keyObj["p"] = keys[i].period;
                            if (keys[i].counter != 0) {
                                keyObj["c"] = keys[i].counter;
                            }
                        }
                        String response;
                        serializeJson(doc, response);
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", response, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/keys/reorder POST
                    if (targetEndpoint == "/api/keys/reorder" && targetMethod == "POST") {
                        // Парсим order массив
                        if (!targetData["order"].is<JsonArray>()) {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(400, "text/plain", "Missing or invalid 'order' field");
                        }
                        
                        std::vector<std::pair<String, int>> newOrder;
                        JsonArray orderArray = targetData["order"].as<JsonArray>();
                        
                        for (JsonObject item : orderArray) {
                            String name = item["name"].as<String>();
                            int order = item["order"].as<int>();
                            newOrder.push_back(std::make_pair(name, order));
                        }
                        
                        LOG_DEBUG("WebServer", "🔗 Obfuscated Keys reorder: " + String(newOrder.size()) + " keys");
                        bool success = keyManager.reorderKeys(newOrder);
                        
                        String output;
                        int statusCode;
                        if (success) {
                            output = "{\"status\":\"success\",\"message\":\"Keys reordered successfully!\"}";
                            statusCode = 200;
                            LOG_DEBUG("WebServer", "🔗 Obfuscated keys reordered successfully");
                        } else {
                            output = "{\"status\":\"error\",\"message\":\"Failed to reorder keys\"}";
                            statusCode = 500;
                            LOG_ERROR("WebServer", "🔗 Obfuscated failed to reorder keys");
                        }
                        
                        LOG_INFO("WebServer", "🔐 OBFUSCATED KEYS REORDER: Securing response");
                        WebServerSecureIntegration::sendSecureResponse(request, statusCode, "text/plain", output, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/add POST
                    if (targetEndpoint == "/api/add" && targetMethod == "POST") {
                        // 🆕 Поддержка нового компактного формата и старого
                        String name, secret;
                        TOTPType type = TOTPType::TOTP;
                        TOTPAlgorithm algo = TOTPAlgorithm::SHA1;
                        uint8_t digits = 6;
                        uint16_t period = 30;
                        bool useExtended = false;
                        
                        if (!targetData["n"].isNull() && !targetData["s"].isNull()) {
                            // Новый компактный формат
                            name = targetData["n"].as<String>();
                            secret = targetData["s"].as<String>();
                            
                            if (!targetData["t"].isNull() && targetData["t"].as<String>() == "H") {
                                type = TOTPType::HOTP;
                                useExtended = true;
                            }
                            
                            if (!targetData["a"].isNull()) {
                                String algoStr = targetData["a"].as<String>();
                                if (algoStr == "256") { algo = TOTPAlgorithm::SHA256; useExtended = true; }
                                else if (algoStr == "512") { algo = TOTPAlgorithm::SHA512; useExtended = true; }
                            }
                            
                            if (!targetData["d"].isNull()) {
                                digits = targetData["d"].as<uint8_t>();
                                if (digits != 6) useExtended = true;
                            }
                            
                            if (!targetData["p"].isNull()) {
                                period = targetData["p"].as<uint16_t>();
                                if (period != 30) useExtended = true;
                            }
                        } else if (!targetData["name"].isNull() && !targetData["secret"].isNull()) {
                            // Старый формат
                            name = targetData["name"].as<String>();
                            secret = targetData["secret"].as<String>();
                        } else {
                            String output = "{\"status\":\"error\",\"message\":\"Missing required fields\"}";
                            WebServerSecureIntegration::sendSecureResponse(request, 400, "application/json", output, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        }
                        
                        LOG_DEBUG("WebServer", "🚇 OBFUSCATED Key add request");
                        
                        bool success;
                        if (useExtended) {
                            success = keyManager.addKeyExtended(name, secret, type, algo, digits, period);
                        } else {
                            success = keyManager.addKey(name, secret);
                        }
                        
                        if (success) {
                            LOG_DEBUG("WebServer", "🚇 OBFUSCATED Key added successfully");
                            String output = "{\"status\":\"success\",\"message\":\"Key added successfully\"}";
                            WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        } else {
                            LOG_ERROR("WebServer", "🚇 OBFUSCATED Failed to add key");
                            String output = "{\"status\":\"error\",\"message\":\"Failed to add key\"}";
                            WebServerSecureIntegration::sendSecureResponse(request, 500, "application/json", output, secureLayer);
                        }
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/show_qr POST
                    if (targetEndpoint == "/api/show_qr" && targetMethod == "POST") {
                        int keyIndex = targetData["key_id"].as<int>();
                        
                        auto keys = keyManager.getAllKeys();
                        if (keyIndex < 0 || keyIndex >= keys.size()) {
                            LOG_ERROR("WebServer", "🔗 OBFUSCATED SHOW_QR: Invalid key index: " + String(keyIndex));
                            String output = "{\"error\":\"Key not found\"}";
                            WebServerSecureIntegration::sendSecureResponse(request, 404, "application/json", output, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        }
                        
                        TOTPKey key = keys[keyIndex];
                        
                        // TOTP secret в uppercase
                        String secretUpper = key.secret;
                        secretUpper.toUpperCase();
                        
                        // Формируем полный TOTP/HOTP URI с расширенными параметрами
                        String totpUri;
                        if (key.type == TOTPType::HOTP) {
                            totpUri = "otpauth://hotp/" + key.name + "?secret=" + secretUpper;
                            totpUri += "&counter=" + String(key.counter);
                        } else {
                            totpUri = "otpauth://totp/" + key.name + "?secret=" + secretUpper;
                            if (key.period != 30) {
                                totpUri += "&period=" + String(key.period);
                            }
                        }
                        
                        if (key.algorithm == TOTPAlgorithm::SHA256) {
                            totpUri += "&algorithm=SHA256";
                        } else if (key.algorithm == TOTPAlgorithm::SHA512) {
                            totpUri += "&algorithm=SHA512";
                        }
                        
                        if (key.digits != 6) {
                            totpUri += "&digits=" + String(key.digits);
                        }
                        
                        LOG_DEBUG("WebServer", "🔗 OBFUSCATED SHOW_QR: Displaying QR for key at index " + String(index));
                        displayManager.requestShowQRCode(totpUri, 30);
                        
                        String output = "{\"success\":true,\"message\":\"QR code displayed on device screen for 30 seconds\"}";
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/remove POST
                    if (targetEndpoint == "/api/remove" && targetMethod == "POST") {
                        int index = targetData["index"].as<int>();
                        
                        LOG_DEBUG("WebServer", "🚇 OBFUSCATED Key remove: index=" + String(index));
                        keyManager.removeKey(index);
                        
                        // 🛡️ Ручное формирование JSON
                        String output = "{\"status\":\"success\",\"message\":\"Key removed successfully\"}";
                        
                        LOG_INFO("WebServer", "🔐 OBFUSCATED KEY REMOVE: Securing tunneled response");
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/hotp/generate POST
                    if (targetEndpoint == "/api/hotp/generate" && targetMethod == "POST") {
                        int index = targetData["index"].as<int>();
                        
                        auto keys = keyManager.getAllKeys();
                        if (index < 0 || index >= keys.size()) {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(400, "text/plain", "Invalid key index");
                        }
                        
                        if (keys[index].type != TOTPType::HOTP) {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(400, "text/plain", "Key is not HOTP type");
                        }
                        
                        LOG_DEBUG("WebServer", "🚇 OBFUSCATED HOTP generate: index=" + String(index));
                        
                        if (!keyManager.incrementHOTPCounter(index)) {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(500, "text/plain", "Failed to increment HOTP counter");
                        }
                        
                        auto updatedKeys = keyManager.getAllKeys();
                        String newCode = totpGenerator.generateCode(updatedKeys[index]);
                        
                        // 🛡️ Ручное формирование JSON
                        String output = "{\"status\":\"success\",\"code\":\"" + newCode + "\",\"counter\":" + String(updatedKeys[index].counter) + "}";
                        
                        LOG_INFO("WebServer", "🔐 OBFUSCATED HOTP GENERATE: Securing response");
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/pincode_settings GET
                    if (targetEndpoint == "/api/pincode_settings" && targetMethod == "GET") {
                        if (request->hasHeader("X-User-Activity")) resetActivityTimer();
                        pinManager.loadPinConfig();
                        
                        // Проверяем текущее состояние device key
                        bool pinProtectionEnabled = CryptoManager::getInstance().isDeviceKeyEncrypted();
                        bool deviceKeyExists = CryptoManager::getInstance().isDeviceKeyFileExists();
                        
                        // Проверяем статус Device BLE PIN
                        bool deviceBlePinEnabled = CryptoManager::getInstance().isDeviceBlePinEnabled();
                        bool deviceBlePinConfigured = CryptoManager::getInstance().isDeviceBlePinConfigured();
                        
                        JsonDocument doc;
                        doc["pinProtectionEnabled"] = pinProtectionEnabled;
                        doc["deviceKeyExists"] = deviceKeyExists;
                        doc["length"] = pinManager.getPinLength();
                        
                        // Device BLE PIN status
                        doc["deviceBlePinEnabled"] = deviceBlePinEnabled;
                        doc["deviceBlePinConfigured"] = deviceBlePinConfigured;
                        
                        // Legacy fields для обратной совместимости
                        doc["enabled"] = pinProtectionEnabled;
                        doc["enabledForDevice"] = pinProtectionEnabled;
                        
                        String response;
                        serializeJson(doc, response);
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", response, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/enable_import_export POST
                    if (targetEndpoint == "/api/enable_import_export" && targetMethod == "POST") {
                        if (request->hasHeader("X-User-Activity")) resetActivityTimer();
                        
                        WebAdminManager::getInstance().enableApi();
                        LOG_INFO("WebServer", "🚇 OBFUSCATED API access enabled");
                        
                        String output = "{\"status\":\"success\",\"message\":\"API access enabled for 5 minutes\"}";
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/import_export_status GET
                    if (targetEndpoint == "/api/import_export_status" && targetMethod == "GET") {
                        if (request->hasHeader("X-User-Activity")) resetActivityTimer();
                        
                        bool isEnabled = WebAdminManager::getInstance().isApiEnabled();
                        int remaining = WebAdminManager::getInstance().getApiTimeRemaining();
                        
                        LOG_INFO("WebServer", "🚇 OBFUSCATED API status: " + String(isEnabled ? "enabled" : "disabled") + ", remaining: " + String(remaining) + "s");
                        
                        String output;
                        output.reserve(60);
                        output = "{\"enabled\":";
                        output += isEnabled ? "true" : "false";
                        output += ",\"remaining\":";
                        output += String(remaining);
                        output += "}";
                        
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/ble_settings GET
                    if (targetEndpoint == "/api/ble_settings" && targetMethod == "GET") {
                        String deviceName = configManager.loadBleDeviceName();
                        String output;
                        output.reserve(30 + deviceName.length());
                        output = "{\"device_name\":\"";
                        output += deviceName;
                        output += "\"}";
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/mdns_settings GET
                    if (targetEndpoint == "/api/mdns_settings" && targetMethod == "GET") {
                        String hostname = configManager.loadMdnsHostname();
                        String output;
                        output.reserve(30 + hostname.length());
                        output = "{\"hostname\":\"";
                        output += hostname;
                        output += "\"}";
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/session_duration GET
                    if (targetEndpoint == "/api/session_duration" && targetMethod == "GET") {
                        ConfigManager::SessionDuration duration = configManager.getSessionDuration();
                        int durationValue = static_cast<int>(duration);
                        String output;
                        output.reserve(30);
                        output = "{\"duration\":";
                        output += String(durationValue);
                        output += "}";
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/theme GET
                    if (targetEndpoint == "/api/theme" && targetMethod == "GET") {
                        if (request->hasHeader("X-User-Activity")) resetActivityTimer();
                        Theme currentTheme = configManager.loadTheme();
                        String output;
                        output.reserve(20);
                        output = "{\"theme\":";
                        output += String(static_cast<int>(currentTheme));
                        output += "}";
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/display_settings GET
                    if (targetEndpoint == "/api/display_settings" && targetMethod == "GET") {
                        if (request->hasHeader("X-User-Activity")) resetActivityTimer();
                        uint16_t displayTimeout = configManager.getDisplayTimeout();
                        uint32_t autoLockTimeout = configManager.getAutoLockTimeout();
                        String output;
                        output.reserve(60);
                        output = "{\"display_timeout\":";
                        output += String(displayTimeout);
                        output += ",\"auto_lock_timeout\":";
                        output += String(autoLockTimeout);
                        output += "}";
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/startup_mode GET
                    if (targetEndpoint == "/api/startup_mode" && targetMethod == "GET") {
                        String mode = configManager.getStartupMode();
                        String output;
                        output.reserve(30 + mode.length());
                        output = "{\"mode\":\"";
                        output += mode;
                        output += "\"}";
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/settings GET
                    if (targetEndpoint == "/api/settings" && targetMethod == "GET") {
                        uint16_t timeout = configManager.getWebServerTimeout();
                        String username = WebAdminManager::getInstance().getUsername();
                        String output;
                        output.reserve(100 + username.length());
                        output = "{\"web_server_timeout\":";
                        output += String(timeout);
                        output += ",\"admin_login\":\"";
                        output += username;
                        output += "\"}";
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/config GET
                    if (targetEndpoint == "/api/config" && targetMethod == "GET") {
                        uint16_t timeout = configManager.getWebServerTimeout();
                        String output;
                        output.reserve(40);
                        output = "{\"web_server_timeout\":";
                        output += String(timeout);
                        output += "}";
                        
                        LOG_DEBUG("WebServer", "🔗 Obfuscated config: timeout=" + String(timeout));
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/ble_settings POST
                    if (targetEndpoint == "/api/ble_settings" && targetMethod == "POST") {
                        String deviceName = targetData["device_name"].as<String>();
                        if (deviceName.length() == 0) {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(400, "text/plain", "Device name parameter missing.");
                        }
                        if (deviceName.length() > 15) {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(400, "text/plain", "Device name too long (max 15 characters)");
                        }
                        configManager.saveBleDeviceName(deviceName);
                        if (bleKeyboardManager) {
                            bleKeyboardManager->setDeviceName(deviceName);
                        }
                        String output = "{\"success\":true,\"message\":\"BLE device name updated successfully!\"}";
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/mdns_settings POST
                    if (targetEndpoint == "/api/mdns_settings" && targetMethod == "POST") {
                        String hostname = targetData["hostname"].as<String>();
                        if (hostname.length() == 0) {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(400, "text/plain", "Hostname parameter missing.");
                        }
                        if (hostname.length() > 63) {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(400, "text/plain", "Invalid hostname length (1-63 characters)");
                        }
                        configManager.saveMdnsHostname(hostname);
                        if (wifiManager) {
                            wifiManager->updateMdnsHostname();
                        }
                        String output = "{\"success\":true,\"message\":\"mDNS hostname updated successfully!\"}";
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/session_duration POST
                    if (targetEndpoint == "/api/session_duration" && targetMethod == "POST") {
                        String durationStr = targetData["duration"].as<String>();
                        if (durationStr.length() == 0) {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(400, "text/plain", "Duration parameter missing.");
                        }
                        int durationValue = durationStr.toInt();
                        if (durationValue == 0 || durationValue == 1 || durationValue == 6 || 
                            durationValue == 24 || durationValue == 72) {
                            ConfigManager::SessionDuration duration = static_cast<ConfigManager::SessionDuration>(durationValue);
                            configManager.setSessionDuration(duration);
                            String output = "{\"success\":true,\"message\":\"Session duration updated successfully!\"}";
                            WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        } else {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(400, "text/plain", "Invalid session duration value.");
                        }
                    }
                    
                    // /api/startup_mode POST
                    if (targetEndpoint == "/api/startup_mode" && targetMethod == "POST") {
                        String mode = targetData["mode"].as<String>();
                        if (mode.length() == 0) {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(400, "text/plain", "Missing mode parameter.");
                        }
                        if (mode != "totp" && mode != "password") {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(400, "text/plain", "Invalid startup mode. Must be 'totp' or 'password'.");
                        }
                        bool success = configManager.saveStartupMode(mode);
                        String message = success ? "Startup mode saved successfully!" : "Failed to save startup mode.";
                        String output;
                        output.reserve(50 + message.length());
                        output = "{\"success\":";
                        output += success ? "true" : "false";
                        output += ",\"message\":\"";
                        output += message;
                        output += "\"}";
                        WebServerSecureIntegration::sendSecureResponse(request, success ? 200 : 500, "application/json", output, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/settings POST
                    if (targetEndpoint == "/api/settings" && targetMethod == "POST") {
                        String timeoutStr = targetData["web_server_timeout"].as<String>();
                        if (timeoutStr.length() == 0) {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing parameters.\"}");
                        }
                        uint16_t timeout = timeoutStr.toInt();
                        configManager.setWebServerTimeout(timeout);
                        _timeoutMinutes = timeout;
                        String output = "{\"success\":true,\"message\":\"Settings updated successfully! Device will restart...\"}";
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        // Schedule restart
                        extern bool shouldRestart;
                        shouldRestart = true;
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/theme POST
                    if (targetEndpoint == "/api/theme" && targetMethod == "POST") {
                        if (request->hasHeader("X-User-Activity")) {
                            resetActivityTimer();
                        }
                        
                        String theme = targetData["theme"].as<String>();
                        
                        if (theme.length() == 0) {
                            JsonDocument errorDoc;
                            errorDoc["success"] = false;
                            errorDoc["message"] = "Theme parameter missing.";
                            String errorResponse;
                            serializeJson(errorDoc, errorResponse);
                            WebServerSecureIntegration::sendSecureResponse(request, 400, "application/json", errorResponse, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        }
                        
                        if (theme != "light" && theme != "dark") {
                            JsonDocument errorDoc;
                            errorDoc["success"] = false;
                            errorDoc["message"] = "Invalid theme. Must be 'light' or 'dark'.";
                            String errorResponse;
                            serializeJson(errorDoc, errorResponse);
                            WebServerSecureIntegration::sendSecureResponse(request, 400, "application/json", errorResponse, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        }
                        
                        Theme newTheme = (theme == "light") ? Theme::LIGHT : Theme::DARK;
                        configManager.saveTheme(newTheme);
                        
                        extern bool pendingThemeChange; extern Theme pendingTheme;
                        pendingThemeChange = true; pendingTheme = newTheme;
                        
                        LOG_INFO("WebServer", "🔗 Obfuscated theme changed to: " + theme);
                        
                        JsonDocument doc;
                        doc["success"] = true;
                        doc["message"] = "Theme updated successfully!";
                        doc["theme"] = theme;
                        String response;
                        serializeJson(doc, response);
                        
                        LOG_INFO("WebServer", "🔐 OBFUSCATED THEME: Securing response");
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", response, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/splash/mode GET
                    if (targetEndpoint == "/api/splash/mode" && targetMethod == "GET") {
                        if (request->hasHeader("X-User-Activity")) {
                            resetActivityTimer();
                        }
                        
                        String mode = splashManager.loadSplashConfig();
                        
                        LOG_INFO("WebServer", "🔗 Obfuscated splash mode requested: " + mode);
                        
                        JsonDocument doc;
                        doc["mode"] = mode;
                        String response;
                        serializeJson(doc, response);
                        
                        LOG_INFO("WebServer", "🔐 OBFUSCATED SPLASH GET: Securing response");
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", response, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/splash/mode POST
                    if (targetEndpoint == "/api/splash/mode" && targetMethod == "POST") {
                        if (request->hasHeader("X-User-Activity")) {
                            resetActivityTimer();
                        }
                        
                        String mode = targetData["mode"].as<String>();
                        
                        if (mode.length() == 0) {
                            JsonDocument errorDoc;
                            errorDoc["success"] = false;
                            errorDoc["message"] = "Mode parameter missing.";
                            String errorResponse;
                            serializeJson(errorDoc, errorResponse);
                            WebServerSecureIntegration::sendSecureResponse(request, 400, "application/json", errorResponse, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        }
                        
                        if (mode != "bladerunner" && mode != "combs" && mode != "securegen" && mode != "disabled") {
                            JsonDocument errorDoc;
                            errorDoc["success"] = false;
                            errorDoc["message"] = "Invalid mode. Must be 'bladerunner', 'combs', 'securegen' or 'disabled'.";
                            String errorResponse;
                            serializeJson(errorDoc, errorResponse);
                            WebServerSecureIntegration::sendSecureResponse(request, 400, "application/json", errorResponse, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        }
                        
                        if (splashManager.saveSplashConfig(mode)) {
                            LOG_INFO("WebServer", "🔗 Obfuscated splash mode saved: " + mode);
                            
                            JsonDocument doc;
                            doc["success"] = true;
                            doc["message"] = "Splash mode saved successfully!";
                            doc["mode"] = mode;
                            String response;
                            serializeJson(doc, response);
                            
                            LOG_INFO("WebServer", "🔐 OBFUSCATED SPLASH POST: Securing response");
                            WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", response, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        } else {
                            JsonDocument errorDoc;
                            errorDoc["success"] = false;
                            errorDoc["message"] = "Invalid splash mode";
                            String errorResponse;
                            serializeJson(errorDoc, errorResponse);
                            WebServerSecureIntegration::sendSecureResponse(request, 400, "application/json", errorResponse, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        }
                    }
                    
                    // /api/rtc GET
                    if (targetEndpoint == "/api/rtc" && targetMethod == "GET") {
                        RTCConfig cfg = rtcManager.getConfig();
                        JsonDocument doc;
                        doc["enabled"]        = cfg.enabled;
                        doc["sda_pin"]        = cfg.sda_pin;
                        doc["scl_pin"]        = cfg.scl_pin;
                        doc["available"]      = rtcManager.isAvailable();
                        doc["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
                        if (rtcManager.isAvailable()) {
                            DateTime now = rtcManager.getCurrentTime();
                            char iso[25];
                            snprintf(iso, sizeof(iso), "%04d-%02d-%02dT%02d:%02d:%02dZ",
                                     now.year(), now.month(), now.day(),
                                     now.hour(), now.minute(), now.second());
                            doc["rtc_time"] = iso;
                        } else {
                            doc["rtc_time"] = nullptr;
                        }
                        String out; serializeJson(doc, out);
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", out, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/rtc POST
                    if (targetEndpoint == "/api/rtc" && targetMethod == "POST") {
                        RTCConfig cfg;
                        cfg.enabled = targetData["enabled"] | false;
                        cfg.sda_pin = targetData["sda_pin"]  | 21;
                        cfg.scl_pin = targetData["scl_pin"]  | 22;
                        time_t ts   = (time_t)(long)targetData["timestamp"];
                        rtcManager.saveConfig(cfg);
                        bool time_synced = false, available = false;
                        String message = "Config saved";
                        if (cfg.enabled) {
                            if (!rtcManager.isInitialized()) {
                                rtcManager.init();
                            } else if (cfg.sda_pin != rtcManager.getConfig().sda_pin ||
                                       cfg.scl_pin != rtcManager.getConfig().scl_pin) {
                                rtcManager.reinit();
                            }
                            available = rtcManager.isAvailable();
                            if (available && ts > 1609459200) {
                                struct timeval tv = { ts, 0 };
                                settimeofday(&tv, nullptr);
                                rtcManager.syncToRTC(ts);
                                time_synced = true;
                                message = "Config saved and time synchronized";
                            } else if (!available) {
                                message = "Config saved but DS3231 not found on I2C bus";
                            } else {
                                message = "Config saved but timestamp invalid";
                            }
                        }
                        JsonDocument resp;
                        resp["success"]     = true;
                        resp["time_synced"] = time_synced;
                        resp["available"]   = available;
                        resp["message"]     = message;
                        String out; serializeJson(resp, out);
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", out, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    if (targetEndpoint == "/api/battery" && targetMethod == "GET") {
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        if (!batteryManager) {
                            WebServerSecureIntegration::sendSecureResponse(request, 503, 
                                "application/json", 
                                "{\"error\":\"Battery manager not available\"}", secureLayer);
                            return;
                        }
                        JsonDocument doc;
                        doc["level"] = batteryManager->getPercentage();
                        doc["charging"] = (batteryManager->getVoltage() > 4.15f);
                        String out;
                        serializeJson(doc, out);
                        WebServerSecureIntegration::sendSecureResponse(request, 200, 
                            "application/json", out, secureLayer);
                        return;
                    }
                    
                    // /api/boot-mode GET
                    if (targetEndpoint == "/api/boot-mode" && targetMethod == "GET") {
                        String mode = configManager.getBootMode();
                        JsonDocument resp;
                        resp["boot_mode"] = mode;
                        String out; serializeJson(resp, out);
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", out, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/boot-mode POST
                    if (targetEndpoint == "/api/boot-mode" && targetMethod == "POST") {
                        String mode = targetData["boot_mode"] | "wifi";
                        if (mode != "wifi" && mode != "ap" && mode != "offline") {
                            WebServerSecureIntegration::sendSecureResponse(request, 400, "application/json", "{\"error\":\"Invalid boot_mode\"}", secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        }
                        bool ok = configManager.saveBootMode(mode);
                        JsonDocument resp;
                        resp["success"] = ok;
                        resp["boot_mode"] = mode;
                        String out; serializeJson(resp, out);
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", out, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/clock_settings GET
                    if (targetEndpoint == "/api/clock_settings" && targetMethod == "GET") {
                        JsonDocument doc;
                        doc["timezone"] = configManager.getTimezone();
                        String response;
                        serializeJson(doc, response);
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", response, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/clock_settings POST
                    if (targetEndpoint == "/api/clock_settings" && targetMethod == "POST") {
                        if (request->hasHeader("X-User-Activity")) resetActivityTimer();
                        
                        String tz = targetData["timezone"].as<String>();
                        JsonDocument doc;
                        String response;
                        int statusCode = 200;
                        
                        if (tz.length() == 0) {
                            doc["success"] = false;
                            doc["message"] = "Missing timezone parameter!";
                            statusCode = 400;
                        } else if (tz.length() > 64) {
                            doc["success"] = false;
                            doc["message"] = "Invalid timezone value!";
                            statusCode = 400;
                        } else if (configManager.saveTimezone(tz)) {
                            setenv("TZ", tz.c_str(), 1);
                            tzset();
                            LOG_INFO("WebServer", "🔗 Obfuscated timezone changed to: " + tz);
                            doc["success"] = true;
                            doc["message"] = "Timezone saved successfully!";
                            doc["timezone"] = tz;
                        } else {
                            doc["success"] = false;
                            doc["message"] = "Failed to save timezone!";
                            statusCode = 500;
                        }
                        
                        serializeJson(doc, response);
                        WebServerSecureIntegration::sendSecureResponse(request, statusCode, "application/json", response, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/display_settings POST
                    if (targetEndpoint == "/api/display_settings" && targetMethod == "POST") {
                        if (request->hasHeader("X-User-Activity")) {
                            resetActivityTimer();
                        }
                        
                        String timeoutStr = targetData["display_timeout"].as<String>();
                        
                        if (timeoutStr.length() == 0) {
                            JsonDocument errorDoc;
                            errorDoc["success"] = false;
                            errorDoc["message"] = "Missing display_timeout parameter!";
                            String errorResponse;
                            serializeJson(errorDoc, errorResponse);
                            WebServerSecureIntegration::sendSecureResponse(request, 400, "application/json", errorResponse, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        }
                        
                        uint16_t timeout = timeoutStr.toInt();
                        
                        if (timeout != 0 && timeout != 15 && timeout != 30 && timeout != 60 && 
                            timeout != 300 && timeout != 1800) {
                            JsonDocument errorDoc;
                            errorDoc["success"] = false;
                            errorDoc["message"] = "Invalid timeout value!";
                            String errorResponse;
                            serializeJson(errorDoc, errorResponse);
                            WebServerSecureIntegration::sendSecureResponse(request, 400, "application/json", errorResponse, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        }
                        
                        String autoLockStr = targetData["auto_lock_timeout"].as<String>();
                        uint32_t autoLockTimeout = (autoLockStr.length() > 0 && autoLockStr != "null") ? (uint32_t)autoLockStr.toInt() : configManager.getAutoLockTimeout();
                        
                        if (autoLockTimeout != 0 && autoLockTimeout != 300 && autoLockTimeout != 900 &&
                            autoLockTimeout != 1800 && autoLockTimeout != 3600 && autoLockTimeout != 14400) {
                            JsonDocument errorDoc;
                            errorDoc["success"] = false;
                            errorDoc["message"] = "Invalid auto lock timeout value!";
                            String errorResponse;
                            serializeJson(errorDoc, errorResponse);
                            WebServerSecureIntegration::sendSecureResponse(request, 400, "application/json", errorResponse, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        }
                        
                        if (timeout > 0 && autoLockTimeout > 0 && autoLockTimeout <= timeout) {
                            JsonDocument errorDoc;
                            errorDoc["success"] = false;
                            errorDoc["message"] = "Auto lock timeout must be greater than screen timeout!";
                            String errorResponse;
                            serializeJson(errorDoc, errorResponse);
                            WebServerSecureIntegration::sendSecureResponse(request, 400, "application/json", errorResponse, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        }
                        
                        JsonDocument doc;
                        int statusCode;
                        
                        if (configManager.saveDisplayTimeout(timeout) && configManager.saveAutoLockTimeout(autoLockTimeout)) {
                            doc["success"] = true;
                            doc["message"] = "Display settings saved successfully!";
                            doc["timeout"] = timeout;
                            doc["auto_lock_timeout"] = autoLockTimeout;
                            statusCode = 200;
                            LOG_INFO("WebServer", "🔗 Obfuscated display timeout=" + String(timeout) + "s autoLock=" + String(autoLockTimeout) + "s");
                        } else {
                            doc["success"] = false;
                            doc["message"] = "Failed to save display settings!";
                            statusCode = 500;
                            LOG_ERROR("WebServer", "🔗 Obfuscated Failed to save display settings");
                        }
                        
                        String response;
                        serializeJson(doc, response);
                        
                        LOG_INFO("WebServer", "🔐 OBFUSCATED DISPLAY_SETTINGS: Securing response");
                        WebServerSecureIntegration::sendSecureResponse(request, statusCode, "application/json", response, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/pincode_settings POST
                    if (targetEndpoint == "/api/pincode_settings" && targetMethod == "POST") {
                        if (request->hasHeader("X-User-Activity")) {
                            resetActivityTimer();
                        }
                        
                        bool enablePinProtection = targetData["enabledForDevice"].as<bool>();
                        bool enabledForBle = targetData["enabledForBle"].as<bool>();
                        int pinLength = targetData["length"].as<int>();
                        String newPin = targetData["pin"].as<String>();
                        String confirmPin = targetData["pin_confirm"].as<String>();
                        String currentPin = targetData["current_pin"].as<String>();
                        
                        LOG_INFO("WebServer", "🔗 Obfuscated PIN settings: device=" + String(enablePinProtection) + ", ble=" + String(enabledForBle) + ", len=" + String(pinLength));
                        
                        String message;
                        int statusCode;
                        bool success;
                        bool requiresReboot = false;
                        
                        // Проверяем текущее состояние device key
                        bool currentlyEncrypted = CryptoManager::getInstance().isDeviceKeyEncrypted();
                        
                        if (enablePinProtection) {
                            // ВКЛЮЧЕНИЕ PIN ЗАЩИТЫ
                            
                            if (newPin.length() == 0) {
                                message = "PIN is required to enable protection.";
                                statusCode = 400;
                                success = false;
                            } else if (newPin != confirmPin) {
                                message = "PINs do not match.";
                                statusCode = 400;
                                success = false;
                            } else if (newPin.length() < 4 || newPin.length() > 10) {
                                message = "PIN must be 4-10 digits.";
                                statusCode = 400;
                                success = false;
                            } else {
                                if (currentlyEncrypted) {
                                    // Device key уже зашифрован - меняем PIN
                                    LOG_INFO("WebServer", "🔗 Obfuscated: Changing PIN encryption...");
                                    
                                    if (currentPin.length() == 0) {
                                        message = "Current PIN is required to change PIN.";
                                        statusCode = 400;
                                        success = false;
                                    } else if (CryptoManager::getInstance().changePinEncryption(currentPin, newPin)) {
                                        message = "PIN changed successfully! Device will reboot.";
                                        statusCode = 200;
                                        success = true;
                                        requiresReboot = true;
                                        LOG_INFO("WebServer", "🔗 Obfuscated: PIN changed successfully");
                                    } else {
                                        message = "Failed to change PIN. Current PIN may be incorrect.";
                                        statusCode = 400;
                                        success = false;
                                        LOG_ERROR("WebServer", "🔗 Obfuscated: Failed to change PIN");
                                    }
                                } else {
                                    // Device key не зашифрован - включаем защиту через factory reset
                                    LOG_WARNING("WebServer", "🔗 Obfuscated: Enabling PIN protection requires factory reset!");
                                    
                                    // Проверяем что пользователь подтвердил factory reset
                                    bool factoryResetConfirmed = targetData["factory_reset_confirmed"].as<bool>();
                                    
                                    if (!factoryResetConfirmed) {
                                        message = "WARNING: Enabling PIN protection will perform FACTORY RESET and delete ALL data (TOTP keys, passwords, WiFi). Export your data first! Send 'factory_reset_confirmed=true' to proceed.";
                                        statusCode = 400;
                                        success = false;
                                        LOG_WARNING("WebServer", "🔗 Obfuscated: Factory reset not confirmed - aborting PIN enable");
                                    } else {
                                        // Удаляем все данные (полная очистка как Hardware Reset)
                                        LOG_INFO("WebServer", "🔗 Obfuscated: Starting full factory reset...");
                                        
                                        LittleFS.remove(KEYS_FILE);
                                        LittleFS.remove(PASSWORD_FILE);
                                        LittleFS.remove("/wifi_config.json");
                                        LittleFS.remove("/wifi_config.json.enc");
                                        LittleFS.remove("/splash_config.json");
                                        LittleFS.remove(PIN_FILE);
                                        LittleFS.remove(CONFIG_FILE);
                                        LittleFS.remove(DEVICE_KEY_FILE);
                                        LittleFS.remove(BLE_CONFIG_FILE);
                                        LittleFS.remove(WEB_ADMIN_FILE);
                                        LittleFS.remove(MDNS_CONFIG_FILE);
                                        LittleFS.remove(LOGIN_STATE_FILE);
                                        LittleFS.remove("/rtc_config.json");
                                        LittleFS.remove("/ble_pin.json.enc");
                                        LittleFS.remove("/session.json.enc");
                                        
                                        // URL Obfuscation: Удаление boot counter и всех mappings
                                        LOG_INFO("WebServer", "🔗 Obfuscated: Clearing URL obfuscation data...");
                                        LittleFS.remove("/boot_counter.txt");
                                        
                                        // Удаляем все url_mappings_*.json файлы
                                        fs::File root = LittleFS.open("/", "r");
                                        if (root) {
                                            fs::File file = root.openNextFile();
                                            while (file) {
                                                String filename = String(file.name());
                                                if (filename.startsWith("/url_mappings_") && filename.endsWith(".json")) {
                                                    LOG_DEBUG("WebServer", "🔗 Obfuscated: Removing URL mapping file: " + filename);
                                                    LittleFS.remove(filename);
                                                }
                                                file = root.openNextFile();
                                            }
                                        }
                                        
                                        // Очистка NVS partition (BLE bonding keys)
                                        LOG_INFO("WebServer", "🔗 Obfuscated: Clearing NVS partition...");
                                        nvs_flash_erase_partition("nvs");
                                        
                                        LOG_INFO("WebServer", "🔗 Obfuscated: All data files deleted for factory reset");
                                        
                                        // Длина PIN выбирается на устройстве при первой загрузке
                                        
                                        message = "Factory reset completed! Device will reboot. On next boot, choose PIN length and create PIN on device screen.";
                                        statusCode = 200;
                                        success = true;
                                        requiresReboot = true;
                                        LOG_INFO("WebServer", "🔗 Obfuscated: Factory reset completed - device will prompt for PIN length and PIN on next boot");
                                    }
                                }
                            }
                        } else {
                            // ОТКЛЮЧЕНИЕ PIN ЗАЩИТЫ
                            
                            if (!currentlyEncrypted) {
                                message = "PIN protection is already disabled.";
                                statusCode = 200;
                                success = false;
                                LOG_INFO("WebServer", "🔗 Obfuscated: PIN protection already disabled");
                            } else {
                                // Новая логика: устанавливаем глобальный флаг для запроса PIN на устройстве
                                LOG_INFO("WebServer", "🔗 Obfuscated: PIN disable requested - will prompt on device");
                                
                                extern bool shouldPromptPinDisable;
                                shouldPromptPinDisable = true;
                                
                                message = "Please enter PIN on device to confirm. Check device screen.";
                                statusCode = 200;
                                success = true;
                                requiresReboot = false; // Не перезагружаем сразу
                                LOG_INFO("WebServer", "🔗 Obfuscated: PIN disable flag set - device will prompt for PIN");
                            }
                        }
                        
                        // BLE PIN настройки (независимо от device PIN)
                        pinManager.setPinEnabledForBle(enabledForBle);
                        pinManager.saveConfig();
                        
                        // Формируем JSON ответ
                        JsonDocument doc;
                        doc["success"] = success;
                        doc["message"] = message;
                        doc["requiresReboot"] = requiresReboot;
                        String response;
                        serializeJson(doc, response);
                        
                        // Если требуется перезагрузка, планируем её
                        if (requiresReboot && success) {
                            extern bool shouldRestart;
                            shouldRestart = true;
                        }
                        
                        LOG_INFO("WebServer", "🔐 OBFUSCATED PINCODE_SETTINGS: Securing response");
                        WebServerSecureIntegration::sendSecureResponse(request, statusCode, "application/json", response, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/ble_pin_update POST
                    if (targetEndpoint == "/api/ble_pin_update" && targetMethod == "POST") {
                        // DEBUG: Логируем весь targetData
                        String targetDataDebug;
                        serializeJson(targetData, targetDataDebug);
                        LOG_DEBUG("WebServer", "🔗 [OBFUSCATED] targetData JSON: " + targetDataDebug);
                        
                        // Новая логика: поддержка двух типов PIN
                        String bleBondingPinStr = targetData["ble_bonding_pin"].as<String>();
                        String deviceBlePinStr = targetData["device_ble_pin"].as<String>();
                        String deviceBlePinEnabledStr = targetData["device_ble_pin_enabled"].as<String>();
                        bool deviceBlePinEnabled = (deviceBlePinEnabledStr == "true");
                        bool deviceBlePinEnabledProvided = (deviceBlePinEnabledStr.length() > 0 && deviceBlePinEnabledStr != "null"); // Параметр был передан
                        
                        // Проверка на null/пустые значения
                        bool hasBondingPin = (bleBondingPinStr.length() > 0 && bleBondingPinStr != "null");
                        bool hasDevicePin = (deviceBlePinStr.length() > 0 && deviceBlePinStr != "null");
                        
                        LOG_INFO("WebServer", "🔗 [OBFUSCATED] BLE PIN UPDATE: hasBondingPin=" + String(hasBondingPin) + ", hasDevicePin=" + String(hasDevicePin));
                        
                        String message;
                        int statusCode = 200;
                        bool success = true;
                        
                        // Обработка BLE Bonding PIN
                        if (hasBondingPin) {
                            if (bleBondingPinStr.length() != 6) {
                                message = "BLE Bonding PIN must be exactly 6 digits";
                                statusCode = 400;
                                success = false;
                            } else {
                                bool validDigits = true;
                                for (char c : bleBondingPinStr) {
                                    if (!isdigit(c)) {
                                        validDigits = false;
                                        break;
                                    }
                                }
                                
                                if (!validDigits) {
                                    message = "BLE Bonding PIN must contain only digits";
                                    statusCode = 400;
                                    success = false;
                                } else {
                                    uint32_t blePin = bleBondingPinStr.toInt();
                                    
                                    if (CryptoManager::getInstance().saveBlePin(blePin)) {
                                        LOG_INFO("WebServer", "🔗 Obfuscated BLE Bonding PIN updated successfully");
                                        
                                        // Clear all BLE bonding keys when PIN changes for security
                                        if (bleKeyboardManager) {
                                            bleKeyboardManager->clearBondingKeys();
                                            LOG_INFO("WebServer", "🔗 Obfuscated BLE bonding keys cleared");
                                        }
                                        
                                        message = "BLE Bonding PIN updated successfully! All BLE clients cleared.";
                                    } else {
                                        LOG_ERROR("WebServer", "🔗 Obfuscated Failed to save BLE Bonding PIN");
                                        message = "Failed to save BLE Bonding PIN";
                                        statusCode = 500;
                                        success = false;
                                    }
                                }
                            }
                        }
                        
                        // Обработка Device BLE PIN
                        if (success && hasDevicePin) {
                            if (deviceBlePinStr.length() != 6) {
                                message = "Device BLE PIN must be exactly 6 digits";
                                statusCode = 400;
                                success = false;
                            } else {
                                bool validDigits = true;
                                for (char c : deviceBlePinStr) {
                                    if (!isdigit(c)) {
                                        validDigits = false;
                                        break;
                                    }
                                }
                                
                                if (!validDigits) {
                                    message = "Device BLE PIN must contain only digits";
                                    statusCode = 400;
                                    success = false;
                                } else {
                                    uint32_t devicePin = deviceBlePinStr.toInt();
                                    
                                    if (deviceBlePinStr == "000000") {
                                        devicePin = 0;
                                    }
                                    
                                    LOG_INFO("WebServer", "🔗 Device BLE PIN validated, saving (value protected)");
                                    
                                    if (CryptoManager::getInstance().saveDeviceBlePin(devicePin)) {
                                        LOG_INFO("WebServer", "🔗 Obfuscated Device BLE PIN updated successfully");
                                        
                                        if (hasBondingPin) {
                                            message += " Device BLE PIN also updated.";
                                        } else {
                                            message = "Device BLE PIN updated successfully!";
                                        }
                                    } else {
                                        LOG_ERROR("WebServer", "🔗 Obfuscated Failed to save Device BLE PIN");
                                        message = "Failed to save Device BLE PIN";
                                        statusCode = 500;
                                        success = false;
                                    }
                                }
                            }
                        } else if (success && !hasDevicePin && deviceBlePinEnabledProvided && deviceBlePinEnabled == false) {
                            // Выключение Device BLE PIN (только enabled=false без PIN, и параметр ЯВНО передан)
                            if (CryptoManager::getInstance().isDeviceBlePinConfigured()) {
                                CryptoManager::getInstance().setDeviceBlePinEnabled(false);
                                LOG_INFO("WebServer", "🔗 Obfuscated Device BLE PIN disabled successfully");
                                message = "Device BLE PIN disabled successfully!";
                                hasDevicePin = true; // Помечаем что операция выполнена
                            } else {
                                message = "Device BLE PIN not configured";
                                statusCode = 400;
                                success = false;
                            }
                        }
                        
                        if (!hasBondingPin && !hasDevicePin) {
                            message = "No PIN parameters provided";
                            statusCode = 400;
                            success = false;
                        }
                        
                        JsonDocument doc;
                        doc["success"] = success;
                        doc["message"] = message;
                        String response;
                        serializeJson(doc, response);
                        
                        LOG_INFO("WebServer", "🔐 OBFUSCATED BLE_PIN_UPDATE: Securing response, success=" + String(success));
                        WebServerSecureIntegration::sendSecureResponse(request, statusCode, "application/json", response, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/change_password POST
                    if (targetEndpoint == "/api/change_password" && targetMethod == "POST") {
                        LOG_INFO("WebServer", "🔗 Obfuscated change_password request");
                        
                        String newPassword = targetData["password"].as<String>();
                        
                        if (newPassword.length() == 0) {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(400, "text/plain", "Password parameter missing.");
                        }
                        if (newPassword.length() < 4) {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(400, "text/plain", "Password must be at least 4 characters long.");
                        }
                        
                        String output;
                        if (WebAdminManager::getInstance().changePassword(newPassword)) {
                            output = "Password changed successfully!";
                            WebServerSecureIntegration::sendSecureResponse(request, 200, "text/plain", output, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        } else {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(500, "text/plain", "Failed to save new password.");
                        }
                        return;
                    }
                    
                    if (targetEndpoint == "/api/wifi_credentials" && targetMethod == "POST") {
                        String ssid = targetData["ssid"].as<String>();
                        String password = targetData["password"].as<String>();
                        String confirm = targetData["confirm_password"].as<String>();
                        
                        if (ssid.length() == 0) {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            String out = "{\"success\":false,\"message\":\"SSID is required\"}";
                            WebServerSecureIntegration::sendSecureResponse(request, 400,
                                "application/json", out, secureLayer);
                            return;
                        }
                        if (password != confirm) {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            String out = "{\"success\":false,\"message\":\"Passwords do not match\"}";
                            WebServerSecureIntegration::sendSecureResponse(request, 400,
                                "application/json", out, secureLayer);
                            return;
                        }
                        if (password.length() > 0 && password.length() < 8) {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            String out = "{\"success\":false,\"message\":\"Password must be at least 8 characters\"}";
                            WebServerSecureIntegration::sendSecureResponse(request, 400,
                                "application/json", out, secureLayer);
                            return;
                        }
                        
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        
                        String out;
                        if (wifiManager && wifiManager->saveCredentials(ssid, password)) {
                            out = "{\"success\":true,\"message\":\"WiFi credentials saved. Reboot to apply.\"}";
                            WebServerSecureIntegration::sendSecureResponse(request, 200,
                                "application/json", out, secureLayer);
                        } else {
                            out = "{\"success\":false,\"message\":\"Failed to save WiFi credentials\"}";
                            WebServerSecureIntegration::sendSecureResponse(request, 500,
                                "application/json", out, secureLayer);
                        }
                        return;
                    }
                    
                    // /api/change_ap_password POST
                    if (targetEndpoint == "/api/change_ap_password" && targetMethod == "POST") {
                        LOG_INFO("WebServer", "🔗 Obfuscated change_ap_password request");
                        
                        String newPassword = targetData["password"].as<String>();
                        
                        if (newPassword.length() == 0) {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(400, "text/plain", "Password parameter missing.");
                        }
                        if (newPassword.length() < 8) {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(400, "text/plain", "WiFi password must be at least 8 characters long.");
                        }
                        
                        String output;
                        if (configManager.saveApPassword(newPassword)) {
                            output = "{\"success\":true,\"message\":\"WiFi AP password updated. Device will restart in 3 seconds...\",\"restart\":true}";
                            LOG_INFO("WebServer", "AP password changed successfully - scheduling restart");
                            WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            delay(3000);
                            ESP.restart();
                        } else {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save new AP password.\"}");
                        }
                        return;
                    }
                    
                    // /logout POST
                    if (targetEndpoint == "/logout" && targetMethod == "POST") {
                        LOG_INFO("WebServer", "🔗 Obfuscated logout request");
                        
                        // Полная очистка сессии (включая персистентную)
                        clearSession();
                        clearSecureSession();
                        
                        LOG_INFO("WebServer", "🔗 Obfuscated session cleared (memory + persistent storage)");
                        
                        String output = "{\"success\":true,\"message\":\"Logged out successfully\"}";
                        
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/passwords GET
                    if (targetEndpoint == "/api/passwords" && targetMethod == "GET") {
                        if (request->hasHeader("X-User-Activity")) {
                            resetActivityTimer();
                        }
                        
                        LOG_DEBUG("WebServer", "🔗 Obfuscated passwords list request");
                        auto passwords = passwordManager.getAllPasswords();
                        
                        JsonDocument doc;
                        JsonArray array = doc.to<JsonArray>();
                        for (const auto& entry : passwords) {
                            JsonObject obj = array.add<JsonObject>();
                            obj["name"] = entry.name;
                            obj["password"] = entry.password;
                        }
                        String output;
                        serializeJson(doc, output);
                        
                        LOG_INFO("WebServer", "🔐 OBFUSCATED PASSWORDS: Securing passwords list");
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/passwords/add POST
                    if (targetEndpoint == "/api/passwords/add" && targetMethod == "POST") {
                        if (request->hasHeader("X-User-Activity")) {
                            resetActivityTimer();
                        }
                        
                        String name = targetData["name"].as<String>();
                        String password = targetData["password"].as<String>();
                        
                        LOG_INFO("WebServer", "🔗 Obfuscated Password add: " + name);
                        
                        if (passwordManager.addPassword(name, password)) {
                            LOG_INFO("WebServer", "🔗 Obfuscated Password added: " + name);
                            
                            JsonDocument responseDoc;
                            responseDoc["status"] = "success";
                            responseDoc["message"] = "Password added successfully!";
                            String jsonResponse;
                            serializeJson(responseDoc, jsonResponse);
                            
                            LOG_INFO("WebServer", "🔐 OBFUSCATED PASSWORD ADD: Securing response");
                            WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", jsonResponse, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        } else {
                            JsonDocument errorDoc;
                            errorDoc["status"] = "error";
                            errorDoc["message"] = "Failed to add password";
                            String errorResponse;
                            serializeJson(errorDoc, errorResponse);
                            
                            WebServerSecureIntegration::sendSecureResponse(request, 500, "application/json", errorResponse, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        }
                    }
                    
                    // /api/passwords/delete POST
                    if (targetEndpoint == "/api/passwords/delete" && targetMethod == "POST") {
                        if (request->hasHeader("X-User-Activity")) {
                            resetActivityTimer();
                        }
                        
                        int index = targetData["index"].as<int>();
                        
                        LOG_INFO("WebServer", "🔗 Obfuscated Password delete: index " + String(index));
                        
                        if (passwordManager.deletePassword(index)) {
                            LOG_INFO("WebServer", "🔗 Obfuscated Password deleted at index: " + String(index));
                            
                            JsonDocument responseDoc;
                            responseDoc["status"] = "success";
                            responseDoc["message"] = "Password deleted successfully!";
                            String jsonResponse;
                            serializeJson(responseDoc, jsonResponse);
                            
                            LOG_INFO("WebServer", "🔐 OBFUSCATED PASSWORD DELETE: Securing response");
                            WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", jsonResponse, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        } else {
                            JsonDocument errorDoc;
                            errorDoc["status"] = "error";
                            errorDoc["message"] = "Failed to delete password";
                            String errorResponse;
                            serializeJson(errorDoc, errorResponse);
                            
                            WebServerSecureIntegration::sendSecureResponse(request, 500, "application/json", errorResponse, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        }
                    }
                    
                    // /api/passwords/get POST
                    if (targetEndpoint == "/api/passwords/get" && targetMethod == "POST") {
                        if (request->hasHeader("X-User-Activity")) {
                            resetActivityTimer();
                        }
                        
                        int index = targetData["index"].as<int>();
                        
                        LOG_INFO("WebServer", "🔗 Obfuscated Password get: index " + String(index));
                        
                        auto passwords = passwordManager.getAllPasswords();
                        if (index >= 0 && index < passwords.size()) {
                            const auto& pwd = passwords[index];
                            
                            JsonDocument responseDoc;
                            responseDoc["name"] = pwd.name;
                            responseDoc["password"] = pwd.password;
                            String jsonResponse;
                            serializeJson(responseDoc, jsonResponse);
                            
                            LOG_INFO("WebServer", "🔐 OBFUSCATED PASSWORD GET: Securing password data");
                            WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", jsonResponse, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        } else {
                            JsonDocument errorDoc;
                            errorDoc["status"] = "error";
                            errorDoc["message"] = "Password not found";
                            String errorResponse;
                            serializeJson(errorDoc, errorResponse);
                            
                            WebServerSecureIntegration::sendSecureResponse(request, 404, "application/json", errorResponse, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        }
                    }
                    
                    // /api/passwords/update POST
                    if (targetEndpoint == "/api/passwords/update" && targetMethod == "POST") {
                        if (request->hasHeader("X-User-Activity")) {
                            resetActivityTimer();
                        }
                        
                        int index = targetData["index"].as<int>();
                        String name = targetData["name"].as<String>();
                        String password = targetData["password"].as<String>();
                        
                        LOG_INFO("WebServer", "🔗 Obfuscated Password update: index " + String(index) + ", name: " + name);
                        
                        if (passwordManager.updatePassword(index, name, password)) {
                            LOG_INFO("WebServer", "🔗 Obfuscated Password updated at index: " + String(index));
                            
                            JsonDocument responseDoc;
                            responseDoc["status"] = "success";
                            responseDoc["message"] = "Password updated successfully!";
                            String jsonResponse;
                            serializeJson(responseDoc, jsonResponse);
                            
                            LOG_INFO("WebServer", "🔐 OBFUSCATED PASSWORD UPDATE: Securing response");
                            WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", jsonResponse, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        } else {
                            JsonDocument errorDoc;
                            errorDoc["status"] = "error";
                            errorDoc["message"] = "Failed to update password";
                            String errorResponse;
                            serializeJson(errorDoc, errorResponse);
                            
                            WebServerSecureIntegration::sendSecureResponse(request, 500, "application/json", errorResponse, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        }
                    }
                    
                    // /api/passwords/reorder POST
                    if (targetEndpoint == "/api/passwords/reorder" && targetMethod == "POST") {
                        if (request->hasHeader("X-User-Activity")) {
                            resetActivityTimer();
                        }
                        
                        if (!targetData["order"].is<JsonArray>()) {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(400, "text/plain", "Missing or invalid 'order' field");
                        }
                        
                        std::vector<std::pair<String, int>> newOrder;
                        JsonArray orderArray = targetData["order"].as<JsonArray>();
                        
                        for (JsonObject item : orderArray) {
                            String name = item["name"].as<String>();
                            int order = item["order"].as<int>();
                            newOrder.push_back(std::make_pair(name, order));
                        }
                        
                        LOG_DEBUG("WebServer", "🔗 Obfuscated Passwords reorder: " + String(newOrder.size()) + " passwords");
                        bool success = passwordManager.reorderPasswords(newOrder);
                        
                        String output;
                        int statusCode;
                        if (success) {
                            output = "{\"status\":\"success\",\"message\":\"Passwords reordered successfully!\"}";
                            statusCode = 200;
                            LOG_DEBUG("WebServer", "🔗 Obfuscated passwords reordered successfully");
                        } else {
                            output = "{\"status\":\"error\",\"message\":\"Failed to reorder passwords\"}";
                            statusCode = 500;
                            LOG_ERROR("WebServer", "🔗 Obfuscated failed to reorder passwords");
                        }
                        
                        LOG_INFO("WebServer", "🔐 OBFUSCATED PASSWORDS REORDER: Securing response");
                        WebServerSecureIntegration::sendSecureResponse(request, statusCode, "application/json", output, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/passwords/export POST
                    if (targetEndpoint == "/api/passwords/export" && targetMethod == "POST") {
                        if (!WebAdminManager::getInstance().isApiEnabled()) {
                            LOG_WARNING("WebServer", "🔗 Obfuscated passwords export blocked: API disabled");
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(403, "text/plain", "API access for import/export is disabled.");
                        }
                        
                        String password = targetData["password"].as<String>();
                        
                        if (password.isEmpty()) {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(400, "text/plain", "Password cannot be empty");
                        }
                        
                        if (!WebAdminManager::getInstance().verifyCredentials(WebAdminManager::getInstance().getUsername(), password)) {
                            LOG_WARNING("WebServer", "🔗 Obfuscated passwords export failed: Invalid admin password");
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(401, "text/plain", "Invalid admin password.");
                        }
                        
                        LOG_INFO("WebServer", "🔗 Obfuscated passwords export: Password verified");
                        auto passwords = passwordManager.getAllPasswords();
                        
                        // 🛡️ Шаг 1: Сериализация паролей
                        String plaintext;
                        {
                            JsonDocument doc;
                            JsonArray array = doc.to<JsonArray>();
                            for (const auto& entry : passwords) {
                                JsonObject obj = array.add<JsonObject>();
                                obj["name"] = entry.name;
                                obj["password"] = entry.password;
                            }
                            serializeJson(doc, plaintext);
                            // JsonDocument автоматически освобождается здесь
                        }
                        
                        LOG_DEBUG("WebServer", "📏 Passwords plaintext size: " + String(plaintext.length()) + " bytes");
                        
                        // 🛡️ Шаг 2: Шифрование
                        String encryptedContent = CryptoManager::getInstance().encryptWithPassword(plaintext, password);
                        plaintext.clear(); // Освобождаем память
                        plaintext = String(); // Полная очистка
                        
                        LOG_DEBUG("WebServer", "🔐 Encrypted size: " + String(encryptedContent.length()) + " bytes");
                        LOG_INFO("WebServer", "💾 OBFUSCATED PASSWORDS EXPORT: Sending encrypted file directly");
                        
                        // Файл уже зашифрован CryptoManager, отправляем напрямую
                        // encryptedContent уже JSON формата {"salt":"...","iv":"...","ciphertext":"..."}
                        
                        LOG_DEBUG("WebServer", "📦 Encrypted content size: " + String(encryptedContent.length()) + " bytes");
                        LOG_DEBUG("WebServer", "💾 Free heap before send: " + String(ESP.getFreeHeap()) + " bytes");
                        
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", encryptedContent, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/passwords/import POST
                    if (targetEndpoint == "/api/passwords/import" && targetMethod == "POST") {
                        if (!WebAdminManager::getInstance().isApiEnabled()) {
                            LOG_WARNING("WebServer", "🔗 Obfuscated passwords import blocked: API disabled");
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(403, "text/plain", "API access for import/export is disabled.");
                        }
                        
                        String password = targetData["password"].as<String>();
                        String fileContent = targetData["data"].as<String>();
                        
                        LOG_DEBUG("WebServer", "🔍 Obfuscated passwords import received:");
                        LOG_DEBUG("WebServer", "  - Password length: " + String(password.length()));
                        LOG_DEBUG("WebServer", "  - FileContent length: " + String(fileContent.length()));
                        
                        if (password.isEmpty() || fileContent.isEmpty()) {
                            LOG_ERROR("WebServer", "❌ Obfuscated passwords import: Missing data");
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(400, "text/plain", "Missing password or file data.");
                        }
                        
                        LOG_INFO("WebServer", "🔗 Obfuscated passwords import: Decrypting file content");
                        String decryptedContent = CryptoManager::getInstance().decryptWithPassword(fileContent, password);
                        
                        if (decryptedContent.isEmpty()) {
                            LOG_WARNING("WebServer", "🔗 Obfuscated passwords import failed: Decryption failed");
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(400, "text/plain", "Decryption failed. Wrong password or corrupt file.");
                        }
                        
                        if (passwordManager.replaceAllPasswords(decryptedContent)) {
                            LOG_INFO("WebServer", "🔗 Obfuscated passwords import: Passwords imported successfully");
                            
                            JsonDocument responseDoc;
                            responseDoc["status"] = "success";
                            responseDoc["message"] = "Import successful!";
                            String jsonResponse;
                            serializeJson(responseDoc, jsonResponse);
                            
                            LOG_INFO("WebServer", "🔐 OBFUSCATED PASSWORDS IMPORT: Securing response");
                            WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", jsonResponse, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        } else {
                            LOG_ERROR("WebServer", "🔗 Obfuscated passwords import failed: Failed to process passwords");
                            
                            JsonDocument errorDoc;
                            errorDoc["status"] = "error";
                            errorDoc["message"] = "Failed to process passwords after decryption.";
                            String errorResponse;
                            serializeJson(errorDoc, errorResponse);
                            
                            WebServerSecureIntegration::sendSecureResponse(request, 500, "application/json", errorResponse, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        }
                    }
                    
                    // /api/export POST
                    if (targetEndpoint == "/api/export" && targetMethod == "POST") {
                        if (!WebAdminManager::getInstance().isApiEnabled()) {
                            LOG_WARNING("WebServer", "🔗 Obfuscated export blocked: API disabled");
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(403, "text/plain", "API access for import/export is disabled.");
                        }
                        
                        String password = targetData["password"].as<String>();
                        
                        if (password.isEmpty()) {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(400, "text/plain", "Password cannot be empty");
                        }
                        
                        if (!WebAdminManager::getInstance().verifyCredentials(WebAdminManager::getInstance().getUsername(), password)) {
                            LOG_WARNING("WebServer", "🔗 Obfuscated export failed: Invalid admin password");
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(401, "text/plain", "Invalid admin password.");
                        }
                        
                        LOG_INFO("WebServer", "🔗 Obfuscated TOTP export: Password verified");
                        auto keys = keyManager.getAllKeys();
                        
                        // 🛡️ Шаг 1: Сериализация TOTP ключей
                        String plaintext;
                        {
                            JsonDocument doc;
                            JsonArray array = doc.to<JsonArray>();
                            for (const auto& key : keys) {
                                JsonObject obj = array.add<JsonObject>();
                                obj["name"] = key.name;
                                obj["secret"] = key.secret;
                                // Добавляем расширенные метаданные
                                obj["type"] = (key.type == TOTPType::HOTP) ? "H" : "T";
                                obj["algorithm"] = (key.algorithm == TOTPAlgorithm::SHA256) ? "256" :
                                                   (key.algorithm == TOTPAlgorithm::SHA512) ? "512" : "1";
                                obj["digits"] = key.digits;
                                obj["period"] = key.period;
                                if (key.counter != 0) {
                                    obj["counter"] = key.counter;
                                }
                            }
                            serializeJson(doc, plaintext);
                            // JsonDocument автоматически освобождается здесь
                        }
                        
                        LOG_DEBUG("WebServer", "📏 TOTP keys plaintext size: " + String(plaintext.length()) + " bytes");
                        
                        // 🛡️ Шаг 2: Шифрование
                        String encryptedContent = CryptoManager::getInstance().encryptWithPassword(plaintext, password);
                        plaintext.clear();
                        plaintext = String();
                        
                        LOG_DEBUG("WebServer", "🔐 Encrypted size: " + String(encryptedContent.length()) + " bytes");
                        LOG_INFO("WebServer", "💾 OBFUSCATED TOTP EXPORT: Sending encrypted file directly");
                        
                        // Файл уже зашифрован CryptoManager, отправляем напрямую
                        // encryptedContent уже JSON формата {"salt":"...","iv":"...","ciphertext":"..."}
                        
                        LOG_DEBUG("WebServer", "📦 Encrypted content size: " + String(encryptedContent.length()) + " bytes");
                        LOG_DEBUG("WebServer", "💾 Free heap before send: " + String(ESP.getFreeHeap()) + " bytes");
                        
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", encryptedContent, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/import POST
                    if (targetEndpoint == "/api/import" && targetMethod == "POST") {
                        if (!WebAdminManager::getInstance().isApiEnabled()) {
                            LOG_WARNING("WebServer", "🔗 Obfuscated import blocked: API disabled");
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(403, "text/plain", "API access for import/export is disabled.");
                        }
                        
                        String password = targetData["password"].as<String>();
                        String fileContent = targetData["data"].as<String>();
                        
                        LOG_DEBUG("WebServer", "🔍 Obfuscated import received:");
                        LOG_DEBUG("WebServer", "  - Password length: " + String(password.length()));
                        LOG_DEBUG("WebServer", "  - FileContent length: " + String(fileContent.length()));
                        
                        if (password.isEmpty() || fileContent.isEmpty()) {
                            LOG_ERROR("WebServer", "❌ Obfuscated import: Missing data");
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(400, "text/plain", "Missing password or file data.");
                        }
                        
                        LOG_INFO("WebServer", "🔗 Obfuscated TOTP import: Decrypting file content");
                        String decryptedContent = CryptoManager::getInstance().decryptWithPassword(fileContent, password);
                        
                        if (decryptedContent.isEmpty()) {
                            LOG_WARNING("WebServer", "🔗 Obfuscated import failed: Decryption failed");
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(400, "text/plain", "Decryption failed. Wrong password or corrupt file.");
                        }
                        
                        if (keyManager.replaceAllKeys(decryptedContent)) {
                            LOG_INFO("WebServer", "🔗 Obfuscated TOTP import: Keys imported successfully");
                            
                            String jsonResponse = "{\"status\":\"success\",\"message\":\"Import successful!\"}";
                            
                            LOG_INFO("WebServer", "🔐 OBFUSCATED IMPORT: Securing response");
                            WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", jsonResponse, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        } else {
                            LOG_ERROR("WebServer", "🔗 Obfuscated import failed: Failed to process keys");
                            
                            String errorResponse = "{\"status\":\"error\",\"message\":\"Failed to process keys after decryption.\"}";
                            WebServerSecureIntegration::sendSecureResponse(request, 500, "application/json", errorResponse, secureLayer);
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return;
                        }
                    }
                    
                    // /api/reboot POST
                    if (targetEndpoint == "/api/reboot" && targetMethod == "POST") {
                        LOG_INFO("WebServer", "🔗 Obfuscated reboot request");
                        
                        String output = "{\"success\":true,\"message\":\"Rebooting\"}";
                        
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        
                        delay(1000);
                        ESP.restart();
                        return;
                    }
                    
                    // /api/reboot_with_web POST
                    if (targetEndpoint == "/api/reboot_with_web" && targetMethod == "POST") {
                        LOG_INFO("WebServer", "🔗 Obfuscated reboot_with_web request");
                        
                        configManager.setWebServerAutoStart(true);
                        LOG_INFO("WebServer", "Web server auto-start flag set successfully");
                        
                        String output = "{\"success\":true,\"message\":\"Rebooting with web server\"}";
                        
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        
                        delay(1000);
                        ESP.restart();
                        return;
                    }
                    
                    // /api/clear_ble_clients POST
                    if (targetEndpoint == "/api/clear_ble_clients" && targetMethod == "POST") {
                        LOG_INFO("WebServer", "🔗 Obfuscated clear_ble_clients request");
                        
                        String responseMsg;
                        bool success = false;
                        
                        if (bleKeyboardManager) {
                            bleKeyboardManager->clearBondingKeys();
                            LOG_INFO("WebServer", "BLE bonding keys cleared via obfuscated tunnel");
                            responseMsg = "{\"success\":true,\"message\":\"BLE clients cleared\"}";
                            success = true;
                        } else {
                            LOG_ERROR("WebServer", "BLE Keyboard Manager not available");
                            responseMsg = "{\"success\":false,\"message\":\"BLE Manager not available\"}";
                        }
                        
                        WebServerSecureIntegration::sendSecureResponse(request, success ? 200 : 500, "application/json", responseMsg, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // /api/activity POST
                    if (targetEndpoint == "/api/activity" && targetMethod == "POST") {
                        if (!isAuthenticated(request)) {
                            if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                            return request->send(401);
                        }
                        resetActivityTimer();
                        String output = "{\"status\":\"success\"}";
                        WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
                        if (bufferPtr) { delete bufferPtr; request->_tempObject = nullptr; }
                        return;
                    }
                    
                    // Fallback
                    LOG_WARNING("WebServer", "⚠️ Endpoint " + targetEndpoint + " not in obfuscated handler");
                    request->send(501, "application/json", "{\"error\":\"Not implemented\"}");
                } else {
                    request->send(400, "text/plain", "Decryption failed");
                }
#else
                request->send(500, "text/plain", "Requires SECURE_LAYER_ENABLED");
#endif
                
                if (bufferPtr) {
                    delete bufferPtr;
                    request->_tempObject = nullptr;
                }
            }
        });
    }
    
    LOG_INFO("WebServer", "🚇 Method Tunneling enabled - HTTP methods hidden from traffic analysis");
#endif

    // ========================================
    // 🖼️ SPLASH SCREEN API ENDPOINTS
    // ========================================
    
    // API: Get current splash mode configuration (ЗАЩИЩЕНО - скопировано с /api/theme)
    server.on("/api/splash/mode", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        
        String mode = splashManager.loadSplashConfig();
        // loadSplashConfig() always returns valid value ("disabled" by default)
        
        JsonDocument doc;
        doc["mode"] = mode;
        // has_custom removed - custom splash upload disabled for security
        
        String output;
        serializeJson(doc, output);
        
#ifdef SECURE_LAYER_ENABLED
        String clientId = WebServerSecureIntegration::getClientId(request);
        if (clientId.length() > 0 && secureLayer.isSecureSessionValid(clientId)) {
            LOG_INFO("WebServer", "🔐 SPLASH GET: Securing response for " + clientId.substring(0,8) + "...");
            WebServerSecureIntegration::sendSecureResponse(request, 200, "application/json", output, secureLayer);
            return;
        }
#endif
        
        LOG_INFO("WebServer", "🖼️ Splash mode requested: " + mode);
        request->send(200, "application/json", output);
    });
    
    // API: Set splash mode configuration (ЗАЩИЩЕНО - скопировано с /api/theme POST)
    server.on("/api/splash/mode", HTTP_POST,
        [this](AsyncWebServerRequest *request){
            // Пустой основной обработчик - вся логика в onBody callback
        },
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index + len == total) {
                // Проверка аутентификации
                if (!isAuthenticated(request)) {
                    return request->send(401, "text/plain", "Unauthorized");
                }
                
                // Проверка CSRF токена
                if (!verifyCsrfToken(request)) {
                    return request->send(403, "text/plain", "CSRF token mismatch");
                }
                
                String mode;
                
#ifdef SECURE_LAYER_ENABLED
                String clientId = WebServerSecureIntegration::getClientId(request);
                
                if (clientId.length() > 0 && 
                    secureLayer.isSecureSessionValid(clientId) &&
                    (request->hasHeader("X-Secure-Request") || 
                     request->hasHeader("X-Security-Level"))) {
                    
                    LOG_INFO("WebServer", "🔐 SPLASH: Decrypting request for " + clientId.substring(0,8) + "...");
                    
                    // Расшифровка запроса
                    String encryptedBody = String((char*)data, len);
                    String decryptedBody;
                    
                    if (secureLayer.decryptRequest(clientId, encryptedBody, decryptedBody)) {
                        LOG_DEBUG("WebServer", "🔐 SPLASH: Decrypted body: " + decryptedBody);
                        
                        // Парсинг параметра mode
                        int modeStart = decryptedBody.indexOf("mode=");
                        if (modeStart >= 0) {
                            int modeEnd = decryptedBody.indexOf("&", modeStart);
                            if (modeEnd < 0) modeEnd = decryptedBody.length();
                            mode = decryptedBody.substring(modeStart + 5, modeEnd);
                            
                            // URL decode (если нужно)
                            mode = urlDecode(mode);
                            LOG_DEBUG("WebServer", "🔐 SPLASH: Parsed mode=" + mode);
                        }
                    } else {
                        LOG_ERROR("WebServer", "🔐 CHANGE_AP_PASSWORD: Decryption failed");
                        return request->send(400, "text/plain", "Decryption failed. Please check your password and try again.");
                    }
                } else
#endif
                {
                    // Fallback: незашифрованный запрос
                    if (request->hasParam("mode", true)) {
                        mode = request->getParam("mode", true)->value();
                    }
                }
                
                // Валидация mode
                if (mode.length() == 0) {
                    return request->send(400, "text/plain", "Mode parameter missing.");
                }
                if (mode != "bladerunner" && mode != "combs" && mode != "securegen" && mode != "disabled") {
                    return request->send(400, "text/plain", "Invalid mode. Must be 'bladerunner', 'combs', 'securegen' or 'disabled'.");
                }
                
                // Применение режима splash
                if (splashManager.saveSplashConfig(mode)) {
                    LOG_INFO("WebServer", "🖼️ Splash mode saved: " + mode);
                    
                    // Формируем JSON ответ
                    JsonDocument doc;
                    doc["success"] = true;
                    doc["mode"] = mode;
                    String response;
                    serializeJson(doc, response);
                    
                    // Отправка зашифрованного ответа
#ifdef SECURE_LAYER_ENABLED
                    String clientId2 = WebServerSecureIntegration::getClientId(request);
                    if (clientId2.length() > 0 && 
                        secureLayer.isSecureSessionValid(clientId2)) {
                        WebServerSecureIntegration::sendSecureResponse(
                            request, 200, "application/json", response, secureLayer);
                        return;
                    }
#endif
                    // Fallback: незашифрованный ответ
                    request->send(200, "application/json", response);
                } else {
                    LOG_ERROR("WebServer", "❌ Failed to save splash mode: " + mode);
                    request->send(400, "text/plain", "Invalid splash mode");
                }
            }
        });
    
    // 🔒 SECURITY: Custom splash upload/delete endpoints REMOVED for security
    // Only embedded splash screens (bladerunner, combs, securegen, disabled) are supported
    
    LOG_INFO("WebServer", "🖼️ Splash screen API endpoints registered");

    // 🛡️ Проверка памяти перед server.begin()
    uint32_t freeHeapBeforeBegin = ESP.getFreeHeap();
    LOG_INFO("WebServer", "📡 Memory before server.begin(): Free=" + String(freeHeapBeforeBegin) + "b");
    
    if (freeHeapBeforeBegin < 30000) {
        LOG_CRITICAL("WebServer", "❌ CRITICAL: Not enough memory to call server.begin()! Aborting.");
        return;
    }
    
    server.begin();
    
    uint32_t freeHeapAfterBegin = ESP.getFreeHeap();
    LOG_INFO("WebServer", "📡 Memory after server.begin(): Free=" + String(freeHeapAfterBegin) + "b");
    LOG_INFO("WebServer", "✅ Web server started successfully!");
    
    _isRunning = true;
}

void WebServerManager::startConfigServer() {
    // Captive portal detection routes
    server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *r){ r->redirect("http://192.168.4.1"); });
    server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *r){ r->redirect("http://192.168.4.1"); });
    server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest *r){ r->redirect("http://192.168.4.1"); });
    server.on("/fwlink", HTTP_GET, [](AsyncWebServerRequest *r){ r->redirect("http://192.168.4.1"); });
    
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        String html = wifi_setup_html;
        // Вставляем mDNS имя хоста по умолчанию в HTML для редиректа
        html.replace("##MDNS_HOSTNAME##", DEFAULT_MDNS_HOSTNAME);
        request->send(200, "text/html", html);
    });
    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request){
        int n = WiFi.scanComplete();
        if (n == WIFI_SCAN_RUNNING) {
            request->send(202, "application/json", "[]");
            return;
        }
        if (n == WIFI_SCAN_FAILED || n == 0) {
            WiFi.scanNetworks(true);
            request->send(200, "application/json", "[]");
            return;
        }
        JsonDocument doc;
        JsonArray array = doc.to<JsonArray>();
        for (int i = 0; i < n; ++i) {
            JsonObject net = array.add<JsonObject>();
            net["ssid"] = WiFi.SSID(i);
            net["rssi"] = WiFi.RSSI(i);
        }
        WiFi.scanDelete();
        WiFi.scanNetworks(true);
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });
    server.on("/save", HTTP_POST, [this](AsyncWebServerRequest *request){
        if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
            // Устанавливаем флаг автозапуска веб-сервера при первой настройке
            configManager.setWebServerAutoStart(true);
            
            String ssid = request->getParam("ssid", true)->value();
            String password = request->getParam("password", true)->value();
            
            // 🔒 Используем WifiManager для шифрованного сохранения credentials
            if (wifiManager != nullptr) {
                bool saved = wifiManager->saveCredentials(ssid, password);
                if (saved) {
                    LOG_INFO("WebServer", "WiFi credentials saved successfully (encrypted)");
                    request->send(200, "text/plain", "Credentials saved. Rebooting...");
                    delay(1000);
                    ESP.restart();
                } else {
                    LOG_ERROR("WebServer", "Failed to save WiFi credentials");
                    request->send(500, "text/plain", "Failed to save credentials. Please try again.");
                }
            } else {
                LOG_ERROR("WebServer", "WifiManager not initialized");
                request->send(500, "text/plain", "Server error: WiFi manager not available.");
            }
        } else {
            request->send(400, "text/plain", "Missing SSID or password.");
        }
    });
    server.onNotFound([](AsyncWebServerRequest *r){ r->redirect("http://192.168.4.1"); });
    server.begin();
}

void WebServerManager::stop() {
    // Очищаем активные сессии при остановке веб-сервера
    clearSession(); // This now also clears secure sessions
    
#ifdef SECURE_LAYER_ENABLED
    // Завершаем работу SecureLayerManager
    secureLayer.end();
    LOG_INFO("WebServer", "🔐 SecureLayerManager shutdown complete");
    
    // ✅ Останавливаем TrafficObfuscation при остановке веб-сервера
    TrafficObfuscationManager::getInstance().end();
    LOG_INFO("WebServer", "🎭 Traffic Obfuscation shutdown complete");
#endif
    
    server.end();
    _isRunning = false;
    LOG_INFO("WebServer", "Web server stopped and sessions cleared");
}

bool WebServerManager::isRunning() {
    return _isRunning;
}

bool WebServerManager::shouldInitializeSecureSession(AsyncWebServerRequest* request) {
#ifdef SECURE_LAYER_ENABLED
    // Инициализируем защищенную сессию для всех страниц (включая login)
    // Это обеспечивает защиту даже до аутентификации
    if (!secureHandshakeActive) {
        return true;
    }
#endif
    return false;
}

void WebServerManager::initializeSecureSession(AsyncWebServerRequest* request) {
#ifdef SECURE_LAYER_ENABLED
    LOG_INFO("WebServer", "🔐 Initializing protected handshake session");
    
    // Генерируем browser-specific client ID
    currentSecureClientId = generateBrowserClientId(request);
    secureHandshakeActive = true;
    handshakeStartTime = millis();
    
    LOG_DEBUG("WebServer", "Protected session initialized for client: " + currentSecureClientId.substring(0,8) + "...");
#endif
}

String WebServerManager::generateBrowserClientId(AsyncWebServerRequest* request) {
    // Создаем уникальный client ID на основе browser fingerprint
    String fingerprint = "";
    
    // IP address
    fingerprint += request->client()->remoteIP().toString();
    
    // User-Agent
    if (request->hasHeader("User-Agent")) {
        fingerprint += request->getHeader("User-Agent")->value();
    }
    
    // Accept-Language
    if (request->hasHeader("Accept-Language")) {
        fingerprint += request->getHeader("Accept-Language")->value();
    }
    
    // Session ID как дополнительная энтропия
    fingerprint += session_id;
    
    // Timestamp для уникальности
    fingerprint += String(millis());
    
    // Хешируем fingerprint для создания client ID
    return CryptoManager::getInstance().generateClientId(fingerprint);
}

void WebServerManager::injectSecureInitScript(AsyncWebServerRequest* request, String& htmlContent) {
#ifdef SECURE_LAYER_ENABLED
    if (!secureHandshakeActive || currentSecureClientId.isEmpty()) {
        LOG_WARNING("WebServer", "Cannot inject secure script: session not initialized");
        return;
    }
    
    // ✅ FIX: Вставляем скрипт в конец <body> с defer чтобы НЕ блокировать рендеринг
    String secureScript = "<script>\n";
    secureScript += "// ESP32 Protected Handshake Auto-Initializer\n";
    secureScript += "window.ESP32_CLIENT_ID = '" + currentSecureClientId + "';\n";
    secureScript += "window.ESP32_SECURE_INIT = true;\n";
    secureScript += "</script>\n";
    secureScript += "<script src='/secure/auto-init.js' defer></script>\n";
    
    // Ищем </body> и вставляем script ПЕРЕД ним
    int bodyClosePos = htmlContent.indexOf("</body>");
    if (bodyClosePos != -1) {
        htmlContent = htmlContent.substring(0, bodyClosePos) + secureScript + htmlContent.substring(bodyClosePos);
        LOG_DEBUG("WebServer", "Secure initialization script injected before </body>");
    } else {
        // Fallback: в конец HTML
        int htmlClosePos = htmlContent.indexOf("</html>");
        if (htmlClosePos != -1) {
            htmlContent = htmlContent.substring(0, htmlClosePos) + secureScript + htmlContent.substring(htmlClosePos);
            LOG_DEBUG("WebServer", "Secure initialization script injected before </html>");
        } else {
            LOG_WARNING("WebServer", "Failed to inject secure script: no </body> or </html> tag found");
        }
    }
#endif
}

void WebServerManager::clearSecureSession() {
#ifdef SECURE_LAYER_ENABLED
    if (secureHandshakeActive) {
        LOG_INFO("WebServer", "🔐 Clearing protected handshake session");
        
        // Invalidate secure session если существует
        if (!currentSecureClientId.isEmpty()) {
            secureLayer.invalidateSecureSession(currentSecureClientId);
        }
        
        secureHandshakeActive = false;
        currentSecureClientId = "";
        handshakeStartTime = 0;
    }
#endif
}


void WebServerManager::resetActivityTimer() {
    _lastActivityTimestamp = millis();
    _oneMinuteWarningShown = false; // Сбросить флаг предупреждения при активности пользователя
}

// Note: generateKeysTable and generatePasswordsTable are no longer needed as the frontend is now JS-based.
String WebServerManager::generateKeysTable() { return ""; }
String WebServerManager::generatePasswordsTable() { return ""; }
