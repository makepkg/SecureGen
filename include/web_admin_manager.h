#ifndef WEB_ADMIN_MANAGER_H
#define WEB_ADMIN_MANAGER_H

#include <Arduino.h>

#define WEB_ADMIN_FILE "/web_admin.json"
#define LOGIN_STATE_FILE "/login_state.json"

class WebAdminManager {
public:
    static WebAdminManager& getInstance();
    void begin();

    bool isRegistered();
    bool registerAdmin(const String& username, const String& password);
    bool verifyCredentials(const String& username, const String& password);
    bool changePassword(const String& newPassword);
    String getUsername();
    unsigned long getLockoutTimeRemaining();
    void handleFailedLoginAttempt();
    void resetLoginAttempts();

private:
    WebAdminManager();
    WebAdminManager(const WebAdminManager&) = delete;
    void operator=(const WebAdminManager&) = delete;

    // Приватные переменные
    bool _isRegistered;
    String _username;
    int _failed_attempts;
    unsigned long _lockout_until;

    // Приватные методы
    void loadCredentials();
    void loadLoginState();
    void saveLoginState();
};

#endif // WEB_ADMIN_MANAGER_H