#ifndef PASSWORD_MANAGER_H
#define PASSWORD_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "crypto_manager.h"

// Структура для хранения одной записи пароля
struct PasswordEntry {
    String name;
    String password;
    int order = 0;  // Порядок сортировки
    uint8_t strength = 0;   // 0=unknown, 1=weak, 2=medium, 3=strong
    String pw_hash = "";    // SHA256 first 8 bytes as hex (16 chars), for duplicate detection
};

class PasswordManager {
public:
    PasswordManager();
    void begin();
    uint8_t computeStrength(const String& password);
    String  computePwHash(const String& password);
    uint32_t getRevision() const { return _revision; }
    
    bool addPassword(const String& name, const String& password);
    bool deletePassword(int index);
    bool updatePassword(int index, const String& name, const String& password); // <-- ADDED
    bool reorderPasswords(const std::vector<std::pair<String, int>>& newOrder); // Изменение порядка
    std::vector<PasswordEntry> getAllPasswords();
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