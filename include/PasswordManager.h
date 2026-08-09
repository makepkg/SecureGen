#ifndef PASSWORD_MANAGER_H
#define PASSWORD_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "crypto_manager.h"

// Структура для хранения одной записи пароля
#define PW_FLAG_AUTO_SEND    (1 << 0)
#define PW_FLAG_SEND_LOGIN   (1 << 1)
#define PW_FLAG_WILDCARD     (1 << 2)

struct PasswordEntry {
    String name;
    String password;
    int order = 0;
    uint8_t strength = 0;
    String pw_hash = "";
    String category = "";
    uint16_t flags = 0;         // bitmask, see PW_FLAG_*
    String login = "";          // optional, used only when PW_FLAG_SEND_LOGIN set
    String nav_mode = "enter";  // "enter"|"tab", used only when PW_FLAG_SEND_LOGIN set
    int login_delay_ms = 300;   // delay after nav before typing password, only relevant when nav_mode="enter"

    // Password Wildcard — system-managed singleton entry.
    // When set: name/password are NOT user-editable via API, only
    // wildcard_len is. Value is never persisted here — it exists only
    // in the device-side HID session cache during an active transfer.
    int wildcard_len = 16;  // reuse existing generator bounds (1-64)

    bool getAutoSend() const { return flags & PW_FLAG_AUTO_SEND; }
    void setAutoSend(bool v) { if (v) flags |= PW_FLAG_AUTO_SEND; else flags &= ~PW_FLAG_AUTO_SEND; }
    bool getSendLogin() const { return flags & PW_FLAG_SEND_LOGIN; }
    void setSendLogin(bool v) { if (v) flags |= PW_FLAG_SEND_LOGIN; else flags &= ~PW_FLAG_SEND_LOGIN; }
    bool getWildcard() const { return flags & PW_FLAG_WILDCARD; }
    void setWildcard(bool v) { if (v) flags |= PW_FLAG_WILDCARD; else flags &= ~PW_FLAG_WILDCARD; }
};

class PasswordManager {
public:
    PasswordManager();
    void begin();
    uint8_t computeStrength(const String& password);
    String  computePwHash(const String& password);
    uint32_t getRevision() const { return _revision; }
    
    bool addPassword(const String& name, const String& password, const String& category = "", 
                     bool auto_send = false, const String& login = "", bool send_login = false, 
                     const String& nav_mode = "enter", int login_delay_ms = 300,
                     bool wildcard = false, int wildcard_len = 16);
    bool deletePassword(int index);
    bool updatePassword(int index, const String& name, const String& password, const String& category = "", 
                        bool auto_send = false, const String& login = "", bool send_login = false, 
                        const String& nav_mode = "enter", int login_delay_ms = 300,
                        bool wildcard = false, int wildcard_len = 16);
    bool reorderPasswords(const std::vector<std::pair<String, int>>& newOrder); // Изменение порядка
    const std::vector<PasswordEntry>& getAllPasswords() const;
    std::vector<PasswordEntry> getAllPasswordsForExport();
    bool replaceAllPasswords(const String& jsonContent); // Новая функция для импорта
    
    // Wipe passwords from memory
    void wipePasswords();

private:
    bool loadPasswords();
    bool savePasswords();

    std::vector<PasswordEntry> passwords;
    bool _needsSave = false;  // Flag for migration save
    uint32_t _revision = 0;   // Incremented on each save for cache invalidation
};

#endif // PASSWORD_MANAGER_H