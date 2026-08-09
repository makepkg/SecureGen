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

### Hidden Space (Layer 1 — dual-slot extension) 

The device key file supports two independent encrypted slots. Each slot 
holds a separate 32-byte device key protected by its own PIN and PBKDF2 
salt. Slots share the same file but are cryptographically independent. 
device.key (161 bytes total) 
├── Slot A [0..80]  — v3: [0x03][salt:16][IV:16][AES-CBC(key_A):48] 
└── Slot B [81..160] —     [salt:16][IV:16][AES-CBC(key_B):48] 

At boot, `unlockDeviceKeyWithPin()` tries PIN against Slot A first, 
then Slot B. The space that decrypts successfully determines which device 
key loads into RAM. The other slot's content is never read again during 
that session. 

**File path isolation** 

Space B data files use HMAC-SHA256 derived names: 
path = "/" + hex(HMAC-SHA256(device_key_B, logical_name)[0:4]) + ".enc" 

Example: `logical_name = "keys"` → `/d7ba9eec.enc` 

These paths are computed at runtime after unlock. Without `device_key_B` 
an observer cannot link any file to Space B or determine which logical 
name it corresponds to. 

Logical names covered: `keys`, `passwords`, `wifi`, `session`, `ble_pin`, 
`device_ble_pin`, `duress_pin`, `web_admin`, `pin_config`, `sentinel`. 

**Provisioning detection** 

Hidden space provisioning is detected via a sentinel file at the 
HMAC-derived path for `"sentinel"`. No explicit flag or counter is stored. 
The sentinel path is computable only from Space A's device key — 
indistinguishable from other HMAC files to anyone without `device_key_A`. 

**Wipe** 

`removeHiddenSpaceWithPin()` performs a complete, ordered cleanup, using
Space B's own key (obtained non-destructively via a dry-run PIN check)
to correctly address Space B's files rather than the caller's active
Space A key: 

1. Overwrite Slot B (80 bytes) with CSPRNG random data 
2. Derive and delete all 10 HMAC-path files 
3. Delete `/.conn_cache` (shared WiFi cache) 
4. Delete `/.net_prefs` (WiFi sharing preference) 
5. Clear `_hiddenSpaceProvisioned` and `_shareWifiWithHiddenSpace` in RAM 

**Known limitations** 

| Limitation | Notes | 
|------------|-------| 
| `/.net_prefs` reveals WiFi sharing state | Does not reveal Space B existence; encrypted with device key A | 
| Space A PIN holder can infer Space B exists | Via sentinel path derivation; not detectable without device key A | 
| Shared global config | Theme, BLE name, mDNS, RTC — by design; device appears uniform | 

**Implementation:** `src/crypto_manager.cpp` — `tryDecryptSlot()`, 
`createHiddenSpace()`, `removeHiddenSpaceWithPin()`, `initSpacePaths()`, 
`deriveSpaceBSentinelPath()`

### Wildcard Passwords (singleton constraint, session-based generation)

**Data model:** Wildcard passwords are system-managed singleton entries where the actual password value is randomly generated on-device during HID transmission and never persisted to flash or transmitted over the network.

**Singleton constraint:**
- Maximum one wildcard entry allowed system-wide
- Enforced at creation: `PasswordManager::addPassword()` rejects if wildcard entry already exists
- Enforced at import: `PasswordManager::replaceAllPasswords()` downgrades subsequent wildcard-flagged entries to normal entries (first wildcard wins, extras logged as downgraded)
- Immutable: wildcard status cannot be changed via `updatePassword()` after creation

**Entry properties:**
- `name`: Always system-managed value `"random"` (not user-editable)
- `password`: Stores placeholder `"[wildcard]"` in encrypted file (actual value never persisted)
- `category`: Intentionally unavailable (disabled in UI and not stored) — wildcard entries have no category assignment
- `wildcard_len`: User-configurable (4-64, default 16), stored per-entry, editable via update endpoint

**Session-based generation:**
- Actual password generated on-demand via `generateWildcardPassword(int len)` in `src/main.cpp`
- Uses `CryptoManager::getInstance().secureRandom()` with modulo-bias protection
- Character set: `a-zA-Z0-9` + special symbols `!@#$%^&*()_+-=[]{}|;:,.<>?~`
- **Session cache:** `_wildcardSessionValue` (static String) stores generated value for current HID session
- **Session owner:** `_wildcardSessionOwnerIndex` tracks which password entry owns current session
- **Session reuse:** Same generated password returned for repeated calls within same session (fixes password/confirm-password mismatch)
- **Session invalidation:** New value generated when switching to different password entry OR entering HID mode

**RAM zeroing (best-effort, not secure_memzero equivalent):**

Session cache is zeroed via `String::setCharAt(i, '\0')` at:
1. Before generating new value (in `getWildcardSessionPassword()` — prevents old buffer freed with password still readable)
2. On secure shutdown (`secureShutdown()` → `wipeWildcardSession()`)
3. On HID mode entry (before BLE/USB choice — forces fresh generation)
4. After HID transmission (local `password` copy zeroed immediately after `sendPassword()` in both BLE and USB blocks)

**Known limitations (documented residual risks):**
- All password/PIN String zeroing now uses `secureWipeString()` (volatile-pointer based, `include/secure_utils.h`), which provides the same compiler-optimization guarantee as `secure_memzero()`. This applies to wildcard session cache, HID transmission buffers, local password copies, and Hidden Space setup PIN strings.
- Arduino `String` internal reallocation may leave fragments in heap (WString.h behavior, not patchable without core modification) — this is a platform constraint independent of the zeroing method used
- `generateWildcardPassword()` return value optimizations (NRVO/move-semantics) not explicitly controlled — potential temporary copies
- Intermediate buffers during `String` growth via `result += char` may persist until next heap allocation overwrites them

**Hidden Space isolation:**
- Wildcard session cache (`_wildcardSessionValue`, `_wildcardSessionOwnerIndex`) is RAM-only global state
- Space A/Space B switching requires full device reboot — all RAM is cleared, including session cache
- No cross-space leakage possible (architectural property, not explicit wipe — reboot guarantees clean state)

**Implementation:** `src/main.cpp` (generation, session management, zeroing), `src/PasswordManager.cpp` (singleton guards), `src/web_server.cpp` (API handlers)

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

**Encrypted envelope format:** All encrypted requests and responses use
a JSON envelope where binary cryptographic parameters are Base64-encoded
(previously Hex, migrated to reduce transport overhead by ~30%):

```json
{
  "type": "secure",
  "counter": 12,
  "data": "Base64(ciphertext)",
  "iv": "Base64(12-byte GCM IV)",
  "tag": "Base64(16-byte GCM tag)"
}
```

The `counter` field provides replay protection (monotonically increasing
per session, rejected if not strictly greater than the last seen value).

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
| Wrong-file-type import (passwords file submitted to keys import, or vice versa) | Schema validation on decrypted payload (required-field check) before write, in Secure Import/Export Mode |

### Out of Scope

| Threat | Reason |
|--------|--------|
| Active MITM on local network | No server certificate |
| Malicious firmware | No secure boot |
| Physical hardware tampering | No tamper detection hardware |
| Side-channel attacks | Hardware constraints |
| PIN length disclosure via timing | Auto-submit reveals length to live observer; accepted UX trade-off |

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