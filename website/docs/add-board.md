# Adding a New Board

This guide explains how to add support for a new ESP32 board to the SecureGen firmware flasher.

---

## Overview

Board configuration lives in a single file:

```
website/public/firmware/boards.json
```

The flash page reads this file on load and builds the firmware manifest dynamically in the browser. Adding a board means adding one object to this JSON array — no code changes required.

---

## Step 1 — Build firmware for the new board

Add a new `[env:]` section to `platformio.ini` for your board, then build:

```bash
pio run -e your-board-env
```

Verify the output files exist:

```
.pio/build/your-board-env/bootloader.bin
.pio/build/your-board-env/partitions.bin
.pio/build/your-board-env/firmware.bin
```

---

## Step 2 — Choose an asset prefix

When you publish a GitHub Release, all boards share the same release. To avoid filename conflicts, use a prefix for boards other than the default.

| Board | Prefix | File names in release |
|-------|--------|-----------------------|
| LILYGO T-Display (default) | *(empty)* | `bootloader.bin`, `firmware.bin`, `partitions.bin` |
| Future board example | `s3-` | `s3-bootloader.bin`, `s3-firmware.bin`, `s3-partitions.bin` |

The default board uses no prefix — its files keep the original names.

---

## Step 3 — Update the GitHub Actions workflow

In `.github/workflows/release-firmware.yml`, add the merge and upload steps for the new board environment.

Find the existing `esptool.py merge_bin` command and add a new one below it:

```yaml
- name: Merge binaries for your-board-env
  run: |
    esptool.py --chip esp32 merge_bin \
      -o s3-merged-firmware.bin \
      --flash_mode dio \
      --flash_freq 40m \
      --flash_size 4MB \
      0x1000 .pio/build/your-board-env/bootloader.bin \
      0x8000 .pio/build/your-board-env/partitions.bin \
      0x10000 .pio/build/your-board-env/firmware.bin
```

Then add the new files to the `softprops/action-gh-release` files list:

```yaml
files: |
  ...existing files...
  s3-merged-firmware.bin
  .pio/build/your-board-env/bootloader.bin    # will be uploaded as-is
  .pio/build/your-board-env/partitions.bin
  .pio/build/your-board-env/firmware.bin
```

> **Note:** GitHub Actions uploads files with their original names. If two boards produce `firmware.bin`, the second upload overwrites the first. Always use a prefix for non-default boards.

To rename files during upload, use the `rename` syntax supported by `softprops/action-gh-release`:

```yaml
files: |
  .pio/build/your-board-env/bootloader.bin#s3-bootloader.bin
  .pio/build/your-board-env/partitions.bin#s3-partitions.bin
  .pio/build/your-board-env/firmware.bin#s3-firmware.bin
```

---

## Step 4 — Add the board to boards.json

Open `website/public/firmware/boards.json` and append a new object to the `boards` array:

```json
{
  "boards": [
    {
      "id": "lilygo-t-display",
      "name": "LILYGO TTGO T-Display",
      "chip": "ESP32",
      "description": "ESP32 with 1.14\" TFT display",
      "asset_prefix": "",
      "offsets": {
        "bootloader": 4096,
        "partitions": 32768,
        "firmware": 65536
      }
    },
    {
      "id": "your-board-id",
      "name": "Your Board Display Name",
      "chip": "ESP32",
      "description": "Short description shown below the dropdown",
      "asset_prefix": "s3-",
      "offsets": {
        "bootloader": 4096,
        "partitions": 32768,
        "firmware": 65536
      }
    }
  ]
}
```

### Field reference

| Field | Type | Description |
|-------|------|-------------|
| `id` | string | Unique identifier, lowercase with hyphens |
| `name` | string | Display name shown in the board dropdown |
| `chip` | string | Chip family passed to esp-web-tools: `ESP32`, `ESP32-S2`, `ESP32-S3`, `ESP32-C3` |
| `description` | string | Short description shown below the board selector |
| `asset_prefix` | string | Prefix prepended to binary filenames in the GitHub Release. Empty string for default board. |
| `offsets` | object | Flash offsets in decimal. Standard ESP32: bootloader=4096, partitions=32768, firmware=65536 |

### Common offsets by chip

| Chip | bootloader | partitions | firmware |
|------|-----------|------------|---------|
| ESP32 | `0x1000` = 4096 | `0x8000` = 32768 | `0x10000` = 65536 |
| ESP32-S2 / S3 / C3 | `0x0` = 0 | `0x8000` = 32768 | `0x10000` = 65536 |

---

## Step 5 — Test locally

```bash
cd website
npm run dev
```

Open `http://localhost:4321/SecureGen/flash` — your new board should appear in the device model dropdown. Select it, pick a version, and verify the flash button activates correctly.

---

## Step 6 — Release

```bash
git add .
git commit -m "feat: add support for Your Board Name"
git tag v2.1.0
git push origin main
git push origin v2.1.0
```

GitHub Actions will build firmware for all configured environments, upload the prefixed binaries to the release, and the flash page will serve the correct files automatically.

---

## Checklist

- [ ] New `[env:]` added to `platformio.ini`
- [ ] Firmware builds successfully with `pio run -e your-board-env`
- [ ] Asset prefix chosen and documented
- [ ] `.github/workflows/release-firmware.yml` updated with new build + upload steps
- [ ] Board object added to `website/public/firmware/boards.json`
- [ ] Tested locally — board appears in dropdown and flash button works
- [ ] Version tag pushed — GitHub Actions creates release with all binaries
