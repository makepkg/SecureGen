# decrypt_export.html — User Guide

## What is this tool

`decrypt_export.html` is an offline editor for the encrypted backup files produced by SecureGen. Open it in any browser — no internet connection required, nothing is ever uploaded anywhere.

It works with two types of backup files:

| File type | Contains | Exported from |
|-----------|----------|---------------|
| TOTP keys backup | TOTP and HOTP keys with all parameters | Web cabinet → Keys page |
| Passwords backup | Password names and values | Web cabinet → Passwords page |

With the tool you can: decrypt and view backups, edit any field, add or delete entries, re-encrypt and save, create new files from scratch, and change the file password.

> **Fully offline.** Your data never leaves your computer. The tool runs entirely in the browser with no network requests.
The interface is available in five languages: English, Russian, German, Chinese (Simplified), and Spanish. Use the language selector in the top-right corner to switch.

---

## Opening the tool

Find `decrypt_export.html` in the root of the SecureGen project folder. Double-click it — it opens in your default browser. It can also be copied anywhere and used without the project folder. No installation needed.

> **Password:** The tool uses your web cabinet admin password to decrypt files — the same password you use to log into the web interface.

---

## Decrypt a backup

1. Drag and drop the backup file into the drop zone — or click the zone to browse for it. Accepted files: `encrypted_keys_backup.json` or `encrypted_passwords_backup.json`
2. Enter your web cabinet admin password
3. Click **🔓 Decrypt**
4. The data appears in an editable table below

### TOTP / HOTP keys — table columns

| Column | Description |
|--------|-------------|
| Name | Service name (e.g. Google, GitHub) |
| Secret | Base32 secret key |
| Type | TOTP (time-based) or HOTP (counter-based) |
| Algorithm | SHA1, SHA256, or SHA512 |
| Digits | 6 or 8 digit code |
| Period / Counter | TOTP: interval in seconds (30 or 60). HOTP: current counter value |

### Passwords — table columns

| Column | Description |
|--------|-------------|
| Name | Entry label |
| Password | Hidden by default — click 👁️ to reveal |

---

## Create a new file

Useful for adding keys manually before importing to a new device, without needing an existing backup.

1. Click **➕ Create TOTP Keys File** or **➕ Create Passwords File**
2. Enter a password and confirm it. Use the same password as your device admin password so you can import the file later
3. Click **Create** — an empty encrypted file downloads and the table opens
4. Add entries using **➕ Add Row** and fill in the fields
5. Save with **🔒 Save Encrypted**

---

## Editing entries

**Modify a field** — click any cell to edit it directly. Dropdowns (Type, Algorithm, Digits) open a selector. Changes are held in memory until you save.

**Add a row** — click **➕ Add Row**. A blank row appears at the bottom. Fill in all fields before saving.

**Delete an entry** — click **🗑️** at the end of the row and confirm. The row is removed immediately.

**Copy an entry** — click **📋** to copy the row data to clipboard, useful for pasting into a spreadsheet or another app.

> **Unsaved changes are lost.** Always click **🔒 Save Encrypted** after editing, or your changes will be gone when you close the tab.

---

## Saving & exporting

| Button | What it does | When to use |
|--------|-------------|-------------|
| **🔒 Save Encrypted** | Re-encrypts the data with your password and downloads a `.json` file | To import back into the device, or keep as a backup |
| **⬇️ Download JSON** | Downloads the data as plain unencrypted JSON | Migration to other apps, manual inspection |
| **📋 Copy CSV** | Copies all data as CSV to clipboard | Pasting into Excel or Google Sheets |

> **Warning:** The file from **⬇️ Download JSON** contains your secrets in plain text. Do not store it long-term. Delete it from your Downloads folder after use.

---

## Change file password

If you changed your web cabinet admin password, re-encrypt your backup files with the new password so they can still be imported to the device.

1. Decrypt the file with the current password
2. Click **🔑 Change Password**
3. Enter the current password, then the new password twice
4. Click **Apply** — the file is immediately re-encrypted and downloads with the new password

> The file password must match the admin password on your device at the time of import.

---

## Common scenarios

**Backup verification** — confirm your backup decrypts correctly
1. Export from web cabinet
2. Open tool, drop in the file
3. Decrypt and check the entries
4. Close — no changes needed

**Migrate to new device** — transfer all keys and passwords
1. Export from old device
2. Decrypt and verify in tool
3. Change password if needed
4. Import on new device

**Add keys manually** — prepare a file without the device
1. Click Create TOTP Keys File
2. Set password (same as device admin password)
3. Add rows and fill in data
4. Save Encrypted → import to device

**Clean up old entries** — remove unused keys or passwords
1. Decrypt the backup
2. Delete unwanted rows
3. Save Encrypted
4. Import back to device

---

## Security

| Property | Detail |
|----------|--------|
| Runs offline | No network requests, no data sent anywhere |
| Encryption | AES-256-CBC, same algorithm as the device |
| Key derivation | PBKDF2-SHA256 with 15,000 iterations — matches `PBKDF2_ITERATIONS_EXPORT` in `config.h` |
| Password storage | Not saved — cleared when the tab is closed |

Do not open the tool on a shared or public computer. Delete downloaded plain JSON files immediately after use. Store encrypted backup files in a safe location — a password manager vault or encrypted drive.

> **If `PBKDF2_ITERATIONS_EXPORT` is ever changed in `config.h`**, the `iterations` value inside `decrypt_export.html` must be updated to match. Otherwise the tool will not decrypt backup files produced by the updated firmware.

---

## Troubleshooting

| Error | Likely cause | Fix |
|-------|-------------|-----|
| Wrong password or corrupted file | Incorrect password (case-sensitive), or file was modified | Re-check the password. Try a different copy of the backup |
| Unrecognized file format | Wrong file selected — not a SecureGen backup | Use only files exported from the web cabinet or saved by this tool |
| File won't import on device | Password mismatch between file and current device admin password | Use Change Password in the tool to re-encrypt with the current device password |
| Missing fields after decryption | Old backup format (only name and secret stored) | Normal — defaults applied (TOTP, SHA1, 6 digits, 30s). Edit if needed and save |
