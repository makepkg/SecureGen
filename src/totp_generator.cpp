#include "totp_generator.h"
#include "config.h"
#include "log_manager.h"
#include <mbedtls/md.h>
#include <esp_task_wdt.h>
#include <time.h>

// 🆕 Универсальный метод генерации кода (TOTP/HOTP)
String TOTPGenerator::generateCode(const TOTPKey& key) {
    // Декодируем Base32 секрет
    uint8_t decodedKey[128]; // Увеличен буфер для SHA512 (может быть длиннее)
    size_t keyLen = base32Decode(key.secret, decodedKey);

    if (keyLen == 0) {
        LOG_ERROR("TOTPGenerator", "Failed to decode Base32 secret for key: " + key.name);
        return "DECODE ERROR";
    }

    // Подготовка данных для HMAC (время или счетчик)
    uint64_t value;
    
    if (key.type == TOTPType::TOTP) {
        // TOTP: используем временной шаг
        time_t now;
        time(&now);
        value = now / key.period;
    } else {
        // HOTP: используем счетчик
        value = key.counter;
    }

    // Конвертируем значение в big-endian байты
    uint8_t valueBytes[8];
    for (int i = 7; i >= 0; i--) {
        valueBytes[i] = value & 0xFF;
        value >>= 8;
    }

    // Выбираем размер хеша в зависимости от алгоритма
    size_t hashLen;
    uint8_t hash[64]; // Максимум для SHA512
    
    switch (key.algorithm) {
        case TOTPAlgorithm::SHA1:
            hashLen = 20;
            hmacSha1(decodedKey, keyLen, valueBytes, 8, hash);
            break;
            
        case TOTPAlgorithm::SHA256:
            hashLen = 32;
            hmacSha256(decodedKey, keyLen, valueBytes, 8, hash);
            break;
            
        case TOTPAlgorithm::SHA512:
            hashLen = 64;
            // 🛡️ Сброс watchdog для тяжелых вычислений SHA512
            esp_task_wdt_reset();
            hmacSha512(decodedKey, keyLen, valueBytes, 8, hash);
            esp_task_wdt_reset();
            break;
            
        default:
            LOG_ERROR("TOTPGenerator", "Unknown algorithm for key: " + key.name);
            return "ALG ERROR";
    }

    // Динамическое отсечение
    uint32_t code = dynamicTruncation(hash, hashLen);
    
    // Форматирование кода с нужным количеством знаков
    return formatCode(code, key.digits);
}

String TOTPGenerator::generateTOTP(const String& base32Secret) {
    uint8_t key[64];
    size_t keyLen = base32Decode(base32Secret, key);

    if (keyLen == 0) {
        return "DECODE ERROR";
    }

    time_t now;
    time(&now);
    uint64_t timeStep = now / CONFIG_TOTP_STEP_SIZE;

    uint8_t timeBytes[8];
    for (int i = 7; i >= 0; i--) {
        timeBytes[i] = timeStep & 0xFF;
        timeStep >>= 8;
    }

    uint8_t hash[20];
    hmacSha1(key, keyLen, timeBytes, 8, hash);

    uint32_t code = dynamicTruncation(hash, 20);
    
    return formatCode(code, 6); // Стандартный 6-значный код
}

int TOTPGenerator::getTimeRemaining(uint16_t period) {
    time_t now;
    time(&now);
    return period - (now % period);
}

// Проверка синхронизации времени
bool TOTPGenerator::isTimeSynced() {
    time_t now;
    time(&now);
    // Проверяем что время >= 1 января 2020 года (1577836800 Unix timestamp)
    // Если время меньше - значит NTP ещё не синхронизирован
    return now >= 1577836800;
}

// === HMAC ФУНКЦИИ (используют mbedtls для аппаратного ускорения ESP32) ===

void TOTPGenerator::hmacSha1(const uint8_t* key, size_t keyLen, const uint8_t* data, size_t dataLen, uint8_t* output) {
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, md_info, 1); // 1 for HMAC
    mbedtls_md_hmac_starts(&ctx, key, keyLen);
    mbedtls_md_hmac_update(&ctx, data, dataLen);
    mbedtls_md_hmac_finish(&ctx, output);
    mbedtls_md_free(&ctx);
}

void TOTPGenerator::hmacSha256(const uint8_t* key, size_t keyLen, const uint8_t* data, size_t dataLen, uint8_t* output) {
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, md_info, 1); // 1 for HMAC
    mbedtls_md_hmac_starts(&ctx, key, keyLen);
    mbedtls_md_hmac_update(&ctx, data, dataLen);
    mbedtls_md_hmac_finish(&ctx, output);
    mbedtls_md_free(&ctx);
}

void TOTPGenerator::hmacSha512(const uint8_t* key, size_t keyLen, const uint8_t* data, size_t dataLen, uint8_t* output) {
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA512);
    
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, md_info, 1); // 1 for HMAC
    mbedtls_md_hmac_starts(&ctx, key, keyLen);
    mbedtls_md_hmac_update(&ctx, data, dataLen);
    mbedtls_md_hmac_finish(&ctx, output);
    mbedtls_md_free(&ctx);
}

// Динамическое отсечение (RFC 4226) с поддержкой разной длины хеша
uint32_t TOTPGenerator::dynamicTruncation(uint8_t* hash, size_t hashLen) {
    // Берем последний байт хеша для определения offset
    int offset = hash[hashLen - 1] & 0x0F;
    
    // Извлекаем 4 байта начиная с offset
    return ((hash[offset] & 0x7F) << 24) |
           ((hash[offset + 1] & 0xFF) << 16) |
           ((hash[offset + 2] & 0xFF) << 8) |
           (hash[offset + 3] & 0xFF);
}

// Форматирование кода с нужным количеством знаков
String TOTPGenerator::formatCode(uint32_t code, uint8_t digits) {
    // Вычисляем модуль для нужного количества знаков
    uint32_t modulo;
    switch (digits) {
        case 6:
            modulo = 1000000;
            break;
        case 8:
            modulo = 100000000;
            break;
        default:
            LOG_WARNING("TOTPGenerator", "Unsupported digits count: " + String(digits) + ", using 6");
            modulo = 1000000;
            digits = 6;
            break;
    }
    
    code %= modulo;
    
    // Форматируем с ведущими нулями
    char codeStr[10]; // Максимум 8 знаков + null terminator
    if (digits == 6) {
        sprintf(codeStr, "%06u", code);
    } else if (digits == 8) {
        sprintf(codeStr, "%08u", code);
    }
    
    return String(codeStr);
}

// Улучшенная реализация декодирования Base32: гибкая и корректная.
size_t TOTPGenerator::base32Decode(const String& base32, uint8_t* output) {
    const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    int buffer = 0;
    int bitsLeft = 0;
    size_t count = 0;

    for (char c : base32) {
        if (isspace(c)) { // Пропускаем пробелы
            continue;
        }
        
        c = toupper(c);

        if (c == '=') { // Достигли конца данных (padding)
            break;
        }

        const char* p = strchr(table, c);
        if (p == nullptr) {
            // Если символ не в таблице (например, дефис), пропускаем его
            continue;
        }

        buffer = (buffer << 5) | (p - table);
        bitsLeft += 5;

        if (bitsLeft >= 8) {
            output[count++] = (buffer >> (bitsLeft - 8)) & 0xFF;
            bitsLeft -= 8;
        }
    }
    return count;
}