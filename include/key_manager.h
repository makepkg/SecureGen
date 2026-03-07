#ifndef KEY_MANAGER_H
#define KEY_MANAGER_H

#include <vector>
#include <Arduino.h>

// Перечисления для типов
enum class TOTPType : uint8_t {
    TOTP = 0,  // Time-based OTP
    HOTP = 1   // HMAC-based OTP (counter)
};

enum class TOTPAlgorithm : uint8_t {
    SHA1 = 0,
    SHA256 = 1,
    SHA512 = 2
};

// Расширенная структура для хранения ключа
struct TOTPKey {
    String name;
    String secret;
    int order = 0;  // Порядок сортировки
    
    // Новые поля для расширенного функционала
    TOTPType type = TOTPType::TOTP;
    TOTPAlgorithm algorithm = TOTPAlgorithm::SHA1;
    uint8_t digits = 6;           // 6 или 8
    uint16_t period = 30;         // 30 или 60 секунд (только для TOTP)
    uint32_t counter = 0;         // Счетчик для HOTP
    
    // Конструктор по умолчанию для обратной совместимости
    TOTPKey() = default;
};

class KeyManager {
public:
    KeyManager();
    bool begin(); // Загружает ключи в память при старте
    
    // Функции для управления ключами
    bool addKey(const String& name, const String& secret);
    bool removeKey(int index);
    bool updateKey(int index, const String& name, const String& secret);
    bool reorderKeys(const std::vector<std::pair<String, int>>& newOrder);
    std::vector<TOTPKey> getAllKeys();
    bool replaceAllKeys(const String& jsonContent);
    
    // Новые функции для расширенного функционала
    bool addKeyExtended(const String& name, const String& secret, 
                       TOTPType type, TOTPAlgorithm algo, 
                       uint8_t digits, uint16_t period);
    bool updateKeyExtended(int index, const String& name, const String& secret,
                          TOTPType type, TOTPAlgorithm algo,
                          uint8_t digits, uint16_t period);
    bool incrementHOTPCounter(int index); // Для HOTP
    TOTPKey& getKeyRef(int index); // Get reference to key for in-memory modification
    bool saveKeys(); // Made public for deferred save pattern
    
    // Wipe secrets from memory
    void wipeSecrets();

private:
    bool loadKeys();
    std::vector<TOTPKey> keys; // Ключи хранятся в памяти в расшифрованном виде
};

#endif // KEY_MANAGER_H