#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <DNSServer.h>
#include "display_manager.h"
#include "config_manager.h"

class WifiManager {
public:
    // Передаем DisplayManager и ConfigManager
    WifiManager(DisplayManager& display, ConfigManager& configManager);
    // Основная функция подключения. Возвращает true, если удалось подключиться.
    bool connect(); 
    // Тихое подключение без сообщений на экране
    bool connectSilent();
    // Запускает портал настройки
    void startConfigPortal();
    String getIP();
    void disconnect();
    // Обновление mDNS имени хоста
    void updateMdnsHostname();
    void startMdnsService();
    void startDns();
    void processDnsRequests();
    
    // 🔒 Сохранение зашифрованных WiFi credentials (public для Web API)
    bool saveCredentials(const String& ssid, const String& password);

private:
    bool loadCredentials(String& ssid, String& password);
    
    DisplayManager& _display;
    ConfigManager& _configManager;
    String _ipAddress;
    DNSServer _dnsServer;
};

#endif // WIFI_MANAGER_H
