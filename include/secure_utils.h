#pragma once
#include <stddef.h>
#include <stdint.h>
#include <WString.h>

// Secure memory zeroing — prevents compiler dead-store elimination at -O3
static inline void secure_memzero(void* ptr, size_t len) {
    volatile uint8_t* p = static_cast<volatile uint8_t*>(ptr);
    while (len--) *p++ = 0;
}

// Secure String wiping — zeroes internal buffer before clearing
static inline void secureWipeString(String& s) {
    volatile char* p = const_cast<volatile char*>(s.c_str());
    for (size_t i = 0; i < s.length(); i++) p[i] = 0;
    s = "";
}
