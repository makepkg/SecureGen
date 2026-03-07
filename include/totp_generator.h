#ifndef TOTP_GENERATOR_H
#define TOTP_GENERATOR_H

#include <Arduino.h>
#include "key_manager.h"

class TOTPGenerator {
public:
    // 🆕 Универсальный метод генерации кода (TOTP/HOTP с любыми параметрами)
    String generateCode(const TOTPKey& key);
    
    // Генерация TOTP кода из секрета в формате Base32 (для обратной совместимости)
    String generateTOTP(const String& base32Secret);

    // Получение оставшегося времени до следующего кода
    int getTimeRemaining(uint16_t period = 30);

    // Проверка синхронизации времени (время >= 2020 год)
    bool isTimeSynced();

private:
    // HMAC функции для разных алгоритмов (используют mbedtls)
    void hmacSha1(const uint8_t* key, size_t keyLen, const uint8_t* data, size_t dataLen, uint8_t* output);
    void hmacSha256(const uint8_t* key, size_t keyLen, const uint8_t* data, size_t dataLen, uint8_t* output);
    void hmacSha512(const uint8_t* key, size_t keyLen, const uint8_t* data, size_t dataLen, uint8_t* output);
    
    // Динамическое отсечение с поддержкой разной длины хеша
    uint32_t dynamicTruncation(uint8_t* hash, size_t hashLen);
    
    // Декодирование Base32
    size_t base32Decode(const String& base32, uint8_t* output);
    
    // Вспомогательная функция для генерации кода из хеша
    String formatCode(uint32_t code, uint8_t digits);
};

#endif