# Security Model

**Last updated:** March 2026
**Audience:** Developers, security auditors

---

## Two independent security domains

The system has two distinct security domains that operate independently:

**Device security** — protects data stored on the device. Relevant when the device is physically accessed or the filesystem is extracted. Keys: PIN, device key, AES-256-CBC encryption at rest.

**Web security** — protects communication between the device and the browser. Relevant when the local network is monitored. Keys: ECDH session key, AES-256-GCM transport encryption, authentication, obfuscation layers.

Compromise of one domain does not automatically compromise the other. An attacker who extracts the filesystem still needs the PIN. An attacker who intercepts web traffic still cannot read the encrypted files without the device key.

---

## Device Security

### Device Key and PIN (Layer 1)

The system uses a two-level key hierarchy:

```
PIN
 └─► PBKDF2-HMAC-SHA256 ─► PIN-derived key (32 bytes, never stored)
                                 └─► AES-256-CBC decrypt
                                          └─► Device key (32 bytes, in RAM only)
                                                   └─► AES-256-CBC encrypt/decrypt
                                                            └─► all data files
```

The device key is a 32-byte random value generated on first boot. It encrypts all data files (Layer 2). The device key itself is encrypted with a key derived from the user PIN via PBKDF2-HMAC-SHA256 and stored as an encrypted file on the filesystem.

On boot, the user enters the PIN. The device derives the PIN key, decrypts the device key file, verifies the embedded checksum, and loads the device key into RAM. If the PIN is wrong the derived key is wrong, the checksum fails, and no data can be accessed.

Neither the PIN nor the PIN-derived key is ever stored on the device.

Key file format versions (backward compatible):

| Size | Version | Encryption |
|------|---------|-----------|
| 81 bytes | v3 (current) | AES-256-CBC with random IV |
| 65 bytes | v2 (legacy) | AES-256-ECB |
| 49 bytes | v1 (legacy) | AES-256-ECB, no checksum |
| ≤33 bytes | v0/ancient | Unencrypted |

**Implementation:** `src/crypto_manager.cpp`

### Data at Rest (Layer 2)

All sensitive files are encrypted with the device key using AES-256-CBC. A unique random IV is generated per write operation.

Encrypted files: TOTP/HOTP secrets, passwords, WiFi credentials, web session data, BLE PIN.

> Note: `isDeviceBlePinConfigured()` checks only for file existence
> (`/ble_pin.json.enc`), it does not attempt decryption. Actual
> decryption happens only in `verifyDeviceBlePin()`,
> `isDeviceBlePinEnabled()`, and `setDeviceBlePinEnabled()`.

Filesystem access without the PIN yields only ciphertext.

**Implementation:** `src/crypto_manager.cpp`

### PBKDF2 Parameters

Defined in `include/config.h`:

```cpp
PBKDF2_ITERATIONS_PIN    // key unlock, once per boot     (~2.7s)
PBKDF2_ITERATIONS_LOGIN  // web password hash/verify      (~2.7s)
PBKDF2_ITERATIONS_EXPORT // export/import encryption      (~1.6s)
```

Hardware limit: watchdog timeout above ~30,000 iterations on ESP32 @ 240MHz. Values are below OWASP 2023 server recommendations (600,000+) due to this constraint. See `docs/security/pbkdf2-security.md`.

---

## Web Security

The web interface runs over HTTP. TLS is impractical on ESP32 given RAM constraints alongside the rest of the firmware (~40–60KB required for a TLS handshake). The following layers compensate at the application level.

### Transport Encryption (Layer 4)

**Key exchange:** Client and server perform ECDH on P-256. The shared secret is processed through HKDF-HMAC-SHA256 with a server-generated random salt to produce a 32-byte AES session key. The session key is never transmitted — each side derives it independently. The client generates a new ephemeral key pair per connection; the private key is discarded after derivation, providing forward secrecy.

**Message encryption:** Every request and response body is encrypted with AES-256-GCM. GCM provides both confidentiality and authenticated integrity — any modification to ciphertext in transit causes tag validation to fail and the message is rejected. There is no plaintext fallback: if no valid encrypted session exists, the server returns an error.

**Client-side crypto:** Both the ECDH and AES-GCM implementations are embedded inline in the served HTML page (no CDN). This ensures Layer 4 works in AP mode and without internet access.

**Known limitation:** Without a server certificate, a network-level active MITM attacker who intercepts and replaces the initial key exchange cannot be detected. Passive interception and ciphertext tampering are mitigated. The server ECDH key pair is regenerated at boot and reused within a boot cycle.

> Two separate DRBG contexts are maintained intentionally:
> `CryptoManager` owns the general-purpose DRBG (device key, IVs,
> salts, session IDs, CSRF tokens). `SecureLayerManager` owns its
> own DRBG for ECDH keypair generation and GCM nonces. They are
> kept separate because mbedTLS DRBG is not thread-safe, and the
> two managers have different lifecycles and security contexts.

**Implementation:** `src/secure_layer_manager.cpp`, `include/web_pages/page_index.h`

### Web Authentication (Layer 3)

Passwords are hashed with PBKDF2-HMAC-SHA256 with a unique random salt. Plaintext passwords are never stored.

Sessions use 32-byte cryptographically random IDs and CSRF tokens. Sessions are stored encrypted on the filesystem. All state-changing operations require server-side CSRF validation. Session cookie is set `HttpOnly; SameSite=Strict`.

**Implementation:** `src/web_admin_manager.cpp`, `src/crypto_manager.cpp`

### Traffic Analysis Resistance (Layers 5–7)

These layers do not provide cryptographic security. They make it harder for a passive observer on the local network to determine what the user is doing.

**URL obfuscation (Layer 5):** API paths are mapped to randomized opaque identifiers. Mappings rotate every 30 reboots. A single public bootstrap endpoint returns only the paths needed to initiate a session; all other mappings require authentication.

**Method tunneling (Layer 6):** All HTTP methods are tunneled through a single obfuscated endpoint. All API traffic appears as requests to one path.

**Traffic obfuscation (Layer 7):** The device sends decoy requests at randomized intervals, indistinguishable from real traffic at the network level.

**Implementation:** `src/url_obfuscation_manager.cpp`, `src/method_tunneling_manager.cpp`, `src/traffic_obfuscation_manager.cpp`

### Header Obfuscation and Input Validation (Layer 8)

**Header obfuscation:** Two real headers are renamed to innocuous names. Five fake decoy headers are injected to make traffic resemble standard browser requests. Some metadata is embedded in the User-Agent string. Traffic analysis resistance only — not cryptographic protection.

**Input validation:** All API inputs are validated before processing (JSON schema, length limits, character whitelists). PIN entry is rate-limited to 5 attempts.

**Implementation:** `src/header_obfuscation_manager.cpp`, `src/web_server.cpp`

---

## Threat Model

### In Scope

| Threat | Primary defense |
|--------|----------------|
| Filesystem extraction from device | AES-256-CBC at rest + PIN |
| PIN brute-force | PBKDF2 iteration cost |
| Passive network sniffing | AES-256-GCM transport encryption |
| Ciphertext tampering in transit | GCM authentication tag |
| Web password brute-force | PBKDF2 + rate limiting |
| CSRF attacks | CSRF token on all mutations |
| Session hijacking | HttpOnly cookie + encrypted session storage |
| Replay attacks | GCM IV uniqueness + session counters |
| Endpoint enumeration | URL obfuscation |
| Traffic pattern analysis | URL/method/traffic obfuscation |

### Out of Scope

| Threat | Reason |
|--------|--------|
| Active MITM on local network | No server certificate |
| Malicious firmware | No secure boot |
| Physical hardware tampering | No tamper detection hardware |
| Side-channel attacks | Hardware constraints |

### Assumptions

- Device is physically secured when not in use
- Firmware is installed from a trusted source
- User selects a PIN and web password of adequate strength
- Local network is not under active man-in-the-middle attack

---

## Developer Guidelines

### Adding a New API Endpoint

- [ ] Verify authentication (`isAuthenticated`)
- [ ] Validate CSRF token on state-changing operations
- [ ] Register in URL obfuscation
- [ ] Register in method tunnel dispatcher
- [ ] Register in `shouldTunnelEndpoint()` (client)
- [ ] Register in `shouldSecureEndpoint()` if response contains sensitive data
- [ ] Validate all inputs before processing
- [ ] Use `sendSecureResponse()` for sensitive data — never send directly
- [ ] No sensitive values in log output

### Encryption Rules

- Sensitive data only through `sendSecureResponse()`
- No plaintext fallback if secure session is absent — return error
- New crypto operations must use existing `CryptoManager` and `SecureLayerManager` — no ad-hoc crypto