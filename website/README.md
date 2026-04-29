# 🌐 SecureGen Website — Documentation

Static website for the SecureGen project, built with **Astro** — a modern framework for creating fast static sites.

---

## 📁 Project Structure

```
website/
├── .astro/                    # Astro service folder (auto-generated)
│   ├── collections/           # Content collections cache
│   ├── content-assets.mjs     # Content assets
│   ├── content-modules.mjs    # Content modules
│   ├── content.d.ts           # TypeScript types for content
│   ├── data-store.json        # Data storage
│   ├── settings.json          # Astro settings
│   └── types.d.ts             # Global TypeScript types
│
├── .vscode/                   # VS Code settings
│   ├── extensions.json        # Recommended extensions
│   └── launch.json            # Debug configuration
│
├── node_modules/              # npm dependencies (not committed to Git)
│
├── public/                    # Static files (copied as-is)
│   ├── assets/                # Media files
│   │   └── screenshots/       # Device and interface screenshots
│   │
│   ├── firmware/              # Firmware files for Web Flasher
│   │   ├── boards.json        # Supported boards configuration
│   │   ├── bootloader.bin     # ESP32 bootloader
│   │   ├── firmware.bin       # Main firmware
│   │   ├── manifest.json      # ESP Web Tools manifest
│   │   └── partitions.bin     # Partition table
│   │
│   ├── _headers               # HTTP headers for Cloudflare Pages/Netlify
│   ├── favicon.ico            # Site icon (ICO format)
│   ├── favicon.svg            # Site icon (SVG format)
│   ├── guide-standalone.html  # Standalone guide (EN) — copy of docs/user/GUIDE.html
│   ├── guide-standalone.ru.html # Standalone guide (RU) — copy of docs/user/GUIDE.ru.html
│   └── tools-app.html         # Standalone tools app — copy from root directory
│
├── src/                       # Website source code
│   ├── layouts/               # Page templates
│   │   └── Layout.astro       # Base layout with navigation and footer
│   │
│   └── pages/                 # Site pages (file-based routing)
│       ├── index.astro        # Home page (/)
│       ├── flash.astro        # Flash page (/flash)
│       ├── guide.astro        # Guide page (/guide)
│       └── tools.astro        # Tools page (/tools)
│
├── .gitignore                 # Git ignored files
├── astro.config.mjs           # Astro configuration
├── package.json               # npm dependencies and scripts
├── package-lock.json          # Dependencies lockfile
├── README.md                  # This file
└── tsconfig.json              # TypeScript configuration
```

---

## 🎯 Files and Folders Purpose

### 📂 `.astro/` — Astro Service Folder
**Auto-generated.** Do not edit manually!

- **`collections/`** — Cache for Content Collections (if used)
- **`content-assets.mjs`** — Module for processing content assets
- **`content-modules.mjs`** — Content modules for import
- **`content.d.ts`** — TypeScript types for Content Collections
- **`data-store.json`** — Astro data storage
- **`settings.json`** — Astro project settings
- **`types.d.ts`** — Global TypeScript types

---

### 📂 `public/` — Static Files

All files from this folder are copied to the site root **without processing**.

#### 📁 `public/assets/screenshots/`
Device images, interface screenshots, board icons.

**Examples:**
- `icon-t-disp.png` — T-Display ESP32 icon
- `icon-s3-disp.png` — T-Display-S3 icon
- Web interface screenshots, device screens

#### 📁 `public/firmware/`
Firmware files for **ESP Web Tools** (Web Flasher).

- **`boards.json`** — Supported boards configuration:
  ```json
  {
    "boards": [
      {
        "id": "lilygo-t-display",
        "name": "T-Display ESP32",
        "chip": "ESP32"
      },
      {
        "id": "lilygo-t-display-s3",
        "name": "T-Display-S3",
        "chip": "ESP32-S3"
      }
    ]
  }
  ```

- **`bootloader.bin`** — ESP32 bootloader (address 0x1000)
- **`firmware.bin`** — Main firmware (address 0x10000)
- **`partitions.bin`** — Partition table (address 0x8000)
- **`manifest.json`** — ESP Web Tools manifest:
  ```json
  {
    "name": "SecureGen",
    "version": "1.0.0",
    "builds": [
      {
        "chipFamily": "ESP32",
        "parts": [
          { "path": "bootloader.bin", "offset": 4096 },
          { "path": "partitions.bin", "offset": 32768 },
          { "path": "firmware.bin", "offset": 65536 }
        ]
      }
    ]
  }
  ```

#### 📄 `public/_headers`
HTTP headers for hosting (Cloudflare Pages, Netlify).

**Example:**
```
/*
  X-Frame-Options: DENY
  X-Content-Type-Options: nosniff
  Referrer-Policy: strict-origin-when-cross-origin
```

#### 📄 `public/favicon.ico` and `public/favicon.svg`
Site icons in different formats.

#### 📄 `public/guide-standalone.html`
**Standalone guide in English.** Works offline, all styles and scripts are embedded.

**Source:** This is an exact copy of `docs/user/GUIDE.html` from the project documentation.

**Purpose:** Provides offline access to the user guide directly from the website without requiring internet connectivity.

#### 📄 `public/guide-standalone.ru.html`
**Standalone guide in Russian.** Works offline, all styles and scripts are embedded.

**Source:** This is an exact copy of `docs/user/GUIDE.ru.html` from the project documentation.

**Purpose:** Provides offline access to the Russian user guide directly from the website.

#### 📄 `public/tools-app.html`
**Standalone tools application** (QR code generator, Base32 converter, etc.). Works offline.

**Source:** This is an exact copy of `tools.html` from the project root directory.

**Purpose:** Provides offline access to utility tools including:
- TOTP QR code generator
- Base32 ↔ Hex converter
- TOTP code calculator
- Configuration export/import

---

### 📂 `src/` — Source Code

#### 📁 `src/layouts/`
Page templates (layouts).

**`Layout.astro`** — Base layout for all pages:
- Navigation menu (Home, Flash, Guide, Tools)
- Footer with GitHub link
- Global styles (dark theme, IBM Plex fonts)
- Multi-language navigation (EN, RU, ZH)
- Responsive design (mobile hamburger menu)

**Usage:**
```astro
---
import Layout from '../layouts/Layout.astro';
---

<Layout title="Page Title" page="home">
  <h1>Page Content</h1>
</Layout>
```

#### 📁 `src/pages/`
Site pages. **Routing is file-based:**

| File | URL | Description |
|------|-----|-------------|
| `index.astro` | `/` | Home page |
| `flash.astro` | `/flash` | Flash page (ESP Web Tools) |
| `guide.astro` | `/guide` | User guide |
| `tools.astro` | `/tools` | Tools (QR generator, Base32) |

**Page Features:**

##### 🏠 `index.astro` — Home Page
- Hero section with call-to-action
- Features list (TOTP, Password Manager, Web Interface)
- Board comparison (T-Display ESP32 vs T-Display-S3)
- Quick start (Git commands)
- Links to YouTube, Hackster.io, Dev.to, Product Hunt
- **Multi-language:** EN, RU, ZH (switch via flags)
- **Interactive:** Board tabs switching with animation

##### ⚡ `flash.astro` — Flash Page
- **ESP Web Tools** integration for browser-based flashing
- Board selection (ESP32 / ESP32-S3)
- Connection instructions
- Chrome, Edge, Opera support (WebSerial API)

##### 📖 `guide.astro` — Guide
- Step-by-step setup instructions
- Adding TOTP keys
- Password configuration
- Using BLE/USB HID
- DS3231 RTC module setup
- Security and encryption

##### 🛠️ `tools.astro` — Tools
- TOTP QR code generator
- Base32 ↔ Hex converter
- TOTP code calculator
- Configuration export/import

---

## ⚙️ Configuration

### 📄 `astro.config.mjs`
Main Astro configuration:

```javascript
import { defineConfig } from 'astro/config';

export default defineConfig({
  site: 'https://makepkg.github.io',  // Site URL
  base: '/SecureGen',                 // Base path (for GitHub Pages)
  output: 'static',                   // Static generation
  build: {
    assets: 'assets',                 // Assets folder
  },
});
```

**Parameters:**
- **`site`** — Full site URL (for sitemap, RSS)
- **`base`** — Base path (for subdomains or GitHub Pages)
- **`output: 'static'`** — Static site generation (SSG)
- **`build.assets`** — Folder for compiled assets

### 📄 `package.json`
Dependencies and scripts:

```json
{
  "name": "website",
  "type": "module",
  "version": "0.0.1",
  "scripts": {
    "dev": "astro dev",       // Start dev server
    "build": "astro build",   // Build for production
    "preview": "astro preview", // Preview build
    "astro": "astro"          // Astro CLI
  },
  "dependencies": {
    "astro": "^5.17.1"        // Astro framework
  }
}
```

### 📄 `tsconfig.json`
TypeScript configuration for Astro:

```json
{
  "extends": "astro/tsconfigs/strict",
  "compilerOptions": {
    "jsx": "react-jsx",
    "jsxImportSource": "astro"
  }
}
```

---

## 🚀 Commands

### Development
```bash
npm run dev
```
Starts dev server at `http://localhost:4321`

**Features:**
- Hot Module Replacement (HMR) — instant updates on changes
- Automatic page reloading
- Detailed browser errors

### Build
```bash
npm run build
```
Creates optimized build in `dist/` folder

**Optimizations:**
- HTML, CSS, JS minification
- Image optimization
- Static page generation
- Tree-shaking unused code

### Preview
```bash
npm run preview
```
Starts local server to preview build at `http://localhost:4321`

---

## 🎨 Design System

### Color Palette
```css
:root {
  --bg: #0a0a0f;           /* Page background */
  --surface: #111118;      /* Card background */
  --border: #1e1e2e;       /* Borders */
  --accent: #00e5ff;       /* Accent color (cyan) */
  --text: #c9d1d9;         /* Main text */
  --text-bright: #e6edf3;  /* Bright text (headings) */
  --text-dim: #586069;     /* Dimmed text */
}
```

### Fonts
- **IBM Plex Sans** — Body text
- **IBM Plex Mono** — Code, headings, navigation

**Import:**
```html
<link href="https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:wght@400;500;600&family=IBM+Plex+Sans:wght@400;500;600&display=swap" rel="stylesheet">
```

### Components

#### Buttons
```html
<!-- Primary button -->
<a href="/flash" class="btn-primary">⚡ Flash your device</a>

<!-- Secondary button -->
<a href="https://github.com/..." class="btn-secondary">GitHub →</a>
```

#### Cards
```html
<div class="feature-card">
  <div class="icon">🔐</div>
  <h3>TOTP & HOTP</h3>
  <p>RFC 6238 and RFC 4226...</p>
</div>
```

#### Terminal
```html
<div class="terminal">
  <div class="terminal-bar">
    <span class="dot dot-red"></span>
    <span class="dot dot-yellow"></span>
    <span class="dot dot-green"></span>
  </div>
  <pre class="terminal-code"><code>$ git clone ...</code></pre>
</div>
```

---

## 🌍 Multi-language Support

The site supports **3 languages:**
- 🇬🇧 **English** (default)
- 🇷🇺 **Russian**
- 🇨🇳 **Chinese**

### Implementation
Translations are stored in **inline JavaScript objects** within pages:

```javascript
const INDEX_I18N = {
  en: {
    'hero.h1': 'Your secrets stay on the device',
    'hero.sub': 'TOTP authenticator and password manager...',
    // ...
  },
  ru: {
    'hero.h1': 'Ваши секреты остаются на устройстве',
    'hero.sub': 'TOTP-аутентификатор и менеджер паролей...',
    // ...
  },
  zh: {
    'hero.h1': '您的秘密保留在设备上',
    'hero.sub': 'ESP32的TOTP验证器和密码管理器...',
    // ...
  }
};
```

### Language Switching
```javascript
function setIndexLang(lang) {
  localStorage.setItem('site_lang', lang);
  document.querySelectorAll('[data-i18n]').forEach(el => {
    el.textContent = INDEX_I18N[lang][el.dataset.i18n];
  });
}
```

**HTML:**
```html
<h1 data-i18n="hero.h1">Your secrets stay on the device</h1>
```

---

## 📱 Responsive Design

### Breakpoints
- **Desktop:** > 768px
- **Mobile:** ≤ 768px

### Mobile Navigation
On mobile devices, navigation collapses into a **hamburger menu**:

```javascript
hamburger.addEventListener('click', () => {
  navLinks.classList.toggle('show');
});
```

### Responsive Grid
```css
.feature-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr); /* Desktop */
  gap: 1rem;
}

@media (max-width: 768px) {
  .feature-grid {
    grid-template-columns: 1fr; /* Mobile */
  }
}
```

---

## 🔧 Technologies

| Technology | Version | Purpose |
|------------|---------|---------|
| **Astro** | 5.17.1 | Static site framework |
| **TypeScript** | Latest | Type safety |
| **IBM Plex Fonts** | Latest | Typography |
| **ESP Web Tools** | Latest | Browser-based flashing |

---

## 📦 Deployment

### GitHub Pages
1. Build the site:
   ```bash
   npm run build
   ```

2. Upload `dist/` contents to `gh-pages` branch

3. Configure GitHub Pages:
   - Settings → Pages
   - Source: Deploy from a branch
   - Branch: `gh-pages` / `root`

### Cloudflare Pages
1. Connect repository
2. Build command: `npm run build`
3. Build output directory: `dist`
4. Automatic deployment on push

### Netlify
1. Connect repository
2. Build command: `npm run build`
3. Publish directory: `dist`
4. Add `_headers` for security

---

## 🛠️ Development

### Adding a New Page
1. Create file in `src/pages/`, e.g., `about.astro`
2. Use Layout:
   ```astro
   ---
   import Layout from '../layouts/Layout.astro';
   ---

   <Layout title="About" page="about">
     <h1>About SecureGen</h1>
   </Layout>
   ```
3. Add link to navigation (`Layout.astro`)

### Adding Images
1. Place in `public/assets/`
2. Use in code:
   ```html
   <img src="/SecureGen/assets/my-image.png" alt="Description" />
   ```

### Adding Styles
**Global styles** — in `Layout.astro`:
```astro
<style is:global>
  .my-class { color: red; }
</style>
```

**Local styles** — in component:
```astro
<style>
  .my-class { color: blue; }
</style>
```

---

## 📋 Standalone Files Source

The following files in `public/` are exact copies from the project documentation:

| Website File | Source File | Purpose |
|--------------|-------------|---------|
| `guide-standalone.html` | `docs/user/GUIDE.html` | English user guide (offline) |
| `guide-standalone.ru.html` | `docs/user/GUIDE.ru.html` | Russian user guide (offline) |
| `tools-app.html` | `tools.html` (root) | Utility tools application (offline) |

**Note:** These files are self-contained HTML documents with embedded CSS and JavaScript, allowing them to work completely offline without any external dependencies.

---

## 📝 License

MIT License — see root `LICENSE` file of the project.

---

## 🔗 Useful Links

- [Astro Documentation](https://docs.astro.build)
- [ESP Web Tools](https://esphome.github.io/esp-web-tools/)
- [IBM Plex Fonts](https://github.com/IBM/plex)
- [GitHub Pages](https://pages.github.com/)

---

**Author:** makepkg  
**Project:** [SecureGen](https://github.com/makepkg/SecureGen)
