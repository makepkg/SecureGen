#include "url_obfuscation_integration.h"
#include <ArduinoJson.h>

void URLObfuscationIntegration::registerDualEndpoint(AsyncWebServer& server, 
                                                   const String& realPath,
                                                   WebRequestMethodComposite method,
                                                   ArRequestHandlerFunction onRequest,
                                                   URLObfuscationManager& obfuscationManager) {
    
    // Регистрируем обычный endpoint (для обратной совместимости)
    server.on(realPath.c_str(), method, onRequest);
    
    // Регистрируем обфусцированный endpoint
    String obfuscatedPath = obfuscationManager.obfuscateURL(realPath);
    if (!obfuscatedPath.isEmpty() && !obfuscatedPath.equals(realPath)) {
        server.on(obfuscatedPath.c_str(), method, createPathResolvingWrapper(onRequest, obfuscationManager));
        logEndpointRegistration(realPath, obfuscatedPath);
    }
}

void URLObfuscationIntegration::registerDualEndpointWithBody(AsyncWebServer& server,
                                                           const String& realPath, 
                                                           WebRequestMethodComposite method,
                                                           ArRequestHandlerFunction onRequest,
                                                           ArBodyHandlerFunction onBody,
                                                           URLObfuscationManager& obfuscationManager) {
    
    // Регистрируем обычный endpoint
    server.on(realPath.c_str(), method, onRequest, NULL, onBody);
    
    // Регистрируем обфусцированный endpoint
    String obfuscatedPath = obfuscationManager.obfuscateURL(realPath);
    if (!obfuscatedPath.isEmpty() && !obfuscatedPath.equals(realPath)) {
        server.on(obfuscatedPath.c_str(), method, 
                 createPathResolvingWrapper(onRequest, obfuscationManager), 
                 NULL, onBody);
        logEndpointRegistration(realPath, obfuscatedPath);
    }
}

void URLObfuscationIntegration::registerDualEndpointWithUpload(AsyncWebServer& server,
                                                             const String& realPath,
                                                             WebRequestMethodComposite method,
                                                             ArRequestHandlerFunction onRequest,
                                                             ArUploadHandlerFunction onUpload,
                                                             ArBodyHandlerFunction onBody,
                                                             URLObfuscationManager& obfuscationManager) {
    
    // Регистрируем обычный endpoint
    server.on(realPath.c_str(), method, onRequest, onUpload, onBody);
    
    // Регистрируем обфусцированный endpoint  
    String obfuscatedPath = obfuscationManager.obfuscateURL(realPath);
    if (!obfuscatedPath.isEmpty() && !obfuscatedPath.equals(realPath)) {
        server.on(obfuscatedPath.c_str(), method, 
                 createPathResolvingWrapper(onRequest, obfuscationManager), 
                 onUpload, onBody);
        logEndpointRegistration(realPath, obfuscatedPath);
    }
}

void URLObfuscationIntegration::addObfuscationAPIEndpoints(AsyncWebServer& server, URLObfuscationManager& obfuscationManager) {
    LOG_INFO("URLObfuscationAPI", "Adding URL obfuscation API endpoints");
    
    
#ifdef DEBUG_BUILD
    // Debug only: full mappings endpoint (not available in production)
    server.on("/api/obfuscation/mappings", HTTP_GET, [&obfuscationManager](AsyncWebServerRequest *request) {
        
        String mappingsJson = obfuscationManager.getObfuscatedMappingJSON();
        request->send(200, "application/json", mappingsJson);
    });
#endif
    
    LOG_INFO("URLObfuscationAPI", "URL obfuscation API endpoints added");
}

String URLObfuscationIntegration::getRequestedPath(AsyncWebServerRequest* request) {
    if (!request) return "";
    return request->url();
}

bool URLObfuscationIntegration::isObfuscatedRequest(AsyncWebServerRequest* request, URLObfuscationManager& obfuscationManager) {
    if (!request) return false;
    
    String requestedPath = getRequestedPath(request);
    return obfuscationManager.isObfuscatedPath(requestedPath);
}

String URLObfuscationIntegration::resolveToRealPath(AsyncWebServerRequest* request, URLObfuscationManager& obfuscationManager) {
    if (!request) return "";
    
    String requestedPath = getRequestedPath(request);
    
    // Если это обфусцированный path, resolve to real
    if (obfuscationManager.isObfuscatedPath(requestedPath)) {
        String realPath = obfuscationManager.getRealPath(requestedPath);
        logRequestMapping(request, realPath);
        return realPath;
    }
    
    // Иначе возвращаем как есть
    return requestedPath;
}

void URLObfuscationIntegration::logEndpointRegistration(const String& realPath, const String& obfuscatedPath) {
    // Логирование отключено для снижения нагрузки
}

void URLObfuscationIntegration::logRequestMapping(AsyncWebServerRequest* request, const String& realPath) {
    // Логирование отключено для производительности
}

ArRequestHandlerFunction URLObfuscationIntegration::createPathResolvingWrapper(ArRequestHandlerFunction originalHandler, 
                                                                             URLObfuscationManager& obfuscationManager) {
    
    return [originalHandler, &obfuscationManager](AsyncWebServerRequest *request) {
        // Log the obfuscated request for debugging
        String requestedPath = getRequestedPath(request);
        String realPath = resolveToRealPath(request, obfuscationManager);
        
        // Вызываем оригинальный handler
        originalHandler(request);
    };
}
