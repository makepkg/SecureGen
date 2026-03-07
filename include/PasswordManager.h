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
};

class PasswordManager {
public:
    PasswordManager();
    void begin();
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
};

#endif // PASSWORD_MANAGER_H