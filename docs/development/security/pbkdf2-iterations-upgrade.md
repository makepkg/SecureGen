# PBKDF2 Configuration

**Last updated:** March 2026
**Status:** Implemented

---

## Current Values

Defined in `include/config.h`:

```cpp
#define PBKDF2_ITERATIONS_PIN    25000  // device key unlock, once per boot
#define PBKDF2_ITERATIONS_LOGIN  25000  // web login, each authentication
#define PBKDF2_ITERATIONS_EXPORT 15000  // export / import encryption
```

---

## Where Each Constant Is Used

| Constant | Functions | Frequency |
|----------|-----------|-----------|
| `PBKDF2_ITERATIONS_PIN` | `encryptDeviceKeyWithPin()`, `decryptDeviceKeyWithPin()` | Once per boot |
| `PBKDF2_ITERATIONS_LOGIN` | `hashPassword()`, `verifyPassword()` | Each login |
| `PBKDF2_ITERATIONS_EXPORT` | `encryptWithPassword()`, `decryptWithPassword()` | Rare |

---

## Measured Performance (ESP32 @ 240MHz, software SHA256)

| Constant | Iterations | Measured time | Note |
|----------|-----------|---------------|------|
| `PBKDF2_ITERATIONS_PIN` | 25,000 | ~2.7s | Once per boot — acceptable |
| `PBKDF2_ITERATIONS_LOGIN` | 25,000 | ~2.7s | Per login — session persists after |
| `PBKDF2_ITERATIONS_EXPORT` | 15,000 | ~1.6s | Rare operation — acceptable |

**Hardware limit:** 50,000+ iterations triggers ESP32 watchdog timeout. Do not exceed 30,000 for any operation.

---

## Security vs Standards

| Standard | Recommendation | Our value | Status |
|----------|---------------|-----------|--------|
| OWASP 2023 | 600,000+ | 25,000 | Below — ESP32 hardware constraint |
| NIST SP 800-63B | 10,000+ | 25,000 | Meets |
| OWASP 2021 | 10,000+ | 25,000 | Meets |

OWASP 2023 recommendations target server-grade hardware. On ESP32 @ 240MHz, 50,000 iterations causes watchdog resets. 25,000 is the practical ceiling for this platform.

### Brute-force resistance (NVIDIA RTX 4090 estimate)

| Iterations | Attempts/sec | Time to exhaust 8-char alphanumeric¹ |
|-----------|-------------|--------------------------------------|
| 5,000 (old) | 20,000 | ~4 days |
| 10,000 (old) | 10,000 | ~8 days |
| **25,000 (current)** | **4,000** | **~20 days** |
| 600,000 (OWASP) | 166 | ~480 days |

¹ 62^8 ≈ 218 trillion combinations (a-z, A-Z, 0-9)

The current value is a known compromise between security and hardware limits. A strong password compensates significantly — longer passwords make brute-force time grow exponentially.

---

## Compatibility: decrypt_export.html

The offline tool uses `iterations: 15000` to match `PBKDF2_ITERATIONS_EXPORT`.

**If `PBKDF2_ITERATIONS_EXPORT` is changed in `config.h`**, the `iterations` value in `decrypt_export.html` must be updated to match — otherwise the tool will fail to decrypt any backup files produced by the updated firmware.

Location in `decrypt_export.html`:
```javascript
const bits = await crypto.subtle.deriveBits(
    { name: 'PBKDF2', salt: b64(salt), iterations: 15000, hash: 'SHA-256' },
    ...
```

---

## Changing Values

To adjust iterations, edit `include/config.h` only — all call sites use the constants.

**If `PBKDF2_ITERATIONS_LOGIN` is changed:**
- Existing stored password hashes become invalid
- The web cabinet password must be reset after flashing

**If `PBKDF2_ITERATIONS_EXPORT` is changed:**
- Update `iterations` in `decrypt_export.html` to match
- All existing backup files encrypted with the old value will not decrypt — users must re-export from the device