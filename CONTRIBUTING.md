# Contributing to ESP32 T-Display TOTP

Thank you for your interest in contributing! This document provides guidelines for contributing to the project.

## 📋 Table of Contents

- [Code of Conduct](#code-of-conduct)
- [How to Contribute](#how-to-contribute)
- [Development Setup](#development-setup)
- [Coding Standards](#coding-standards)
- [Security Considerations](#security-considerations)
- [Pull Request Process](#pull-request-process)

## Code of Conduct

- Be respectful and inclusive
- Focus on constructive feedback
- Help others learn and grow
- Follow the project's guidelines

## How to Contribute

### Reporting Bugs

1. Check if the bug is already reported in [Issues](https://github.com/makepkg/SecureGen/issues)
2. Create a new issue with:
   - Clear title and description
   - Steps to reproduce
   - Expected vs actual behavior
   - Firmware version and hardware details
   - Logs or screenshots if applicable

### Suggesting Features

1. Check [Discussions](https://github.com/makepkg/SecureGen/discussions) for similar ideas
2. Create a new discussion with:
   - Clear description of the feature
   - Use cases and benefits
   - Potential implementation approach
   - Any security implications

### Contributing Code

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Test thoroughly
5. Commit with clear messages
6. Push to your fork
7. Open a Pull Request

## Development Setup

### Prerequisites

- [PlatformIO IDE](https://platformio.org/platformio-ide)
- LILYGO® T-Display ESP32 **or** T-Display-S3 board
- Git for version control

### Setup Steps

```bash
# Clone your fork
git clone https://github.com/YOUR_USERNAME/SecureGen.git
cd SecureGen

# Add upstream remote
git remote add upstream https://github.com/makepkg/SecureGen.git

# Install dependencies (PlatformIO handles this automatically)
pio lib install

# Build the project for your board
# For T-Display ESP32:
pio run -e lilygo-t-display

# For T-Display-S3:
pio run -e lilygo-t-display-s3

# Upload to device
# For T-Display ESP32:
pio run -e lilygo-t-display -t upload

# For T-Display-S3:
pio run -e lilygo-t-display-s3 -t upload
```

## Coding Standards

### C++ Code Style

- Use 4 spaces for indentation (no tabs)
- Follow existing code style
- Use meaningful variable names
- Add comments for complex logic
- Keep functions focused and small

### File Organization

```
src/          - Source files (.cpp)
include/      - Header files (.h)
lib/          - External libraries
docs/         - Documentation
test/         - Test files
```

### Naming Conventions

- Classes: `PascalCase` (e.g., `CryptoManager`)
- Functions: `camelCase` (e.g., `encryptData`)
- Variables: `camelCase` (e.g., `sessionKey`)
- Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_SESSIONS`)
- Private members: `_camelCase` (e.g., `_deviceKey`)

### Comments

```cpp
// Single line comment for brief explanations

/**
 * Multi-line comment for function documentation
 * @param data Input data to process
 * @return Processed result
 */
```

## Security Considerations

### Critical Rules

⚠️ **NEVER commit:**
- Passwords or API keys
- Private keys or certificates
- Personal data or credentials
- Internal security documentation

⚠️ **ALWAYS:**
- Review security implications of changes
- Test cryptographic code thoroughly
- Follow secure coding practices
- Document security-related changes

### Security-Sensitive Areas

When modifying these areas, extra care is required:
- `src/crypto_manager.cpp` - Cryptographic operations
- `src/secure_layer_manager.cpp` - Web security
- `src/web_admin_manager.cpp` - Authentication
- `src/pin_manager.cpp` - PIN management
- `src/ble_keyboard_manager.cpp` - BLE security

### Testing Security Changes

1. Test with different scenarios
2. Verify no data leakage
3. Check for timing attacks
4. Validate input sanitization
5. Test error handling

## Pull Request Process

### Before Submitting

- [ ] Code compiles without errors
- [ ] Tested on actual hardware
- [ ] No security vulnerabilities introduced
- [ ] Documentation updated if needed
- [ ] Commit messages are clear
- [ ] Code follows project style

### PR Description Template

```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation update

## Testing
How was this tested?

## Checklist
- [ ] Code compiles
- [ ] Tested on hardware
- [ ] Documentation updated
- [ ] No security issues
```

### Review Process

1. Maintainer reviews the PR
2. Feedback is provided if needed
3. Make requested changes
4. PR is approved and merged
5. You're credited in release notes!

## Documentation

### When to Update Docs

- New features added
- Behavior changes
- API modifications
- Security updates
- Bug fixes affecting usage

### Documentation Files

- `README.md` - Main project documentation
- `docs/` - Detailed documentation
- Code comments - Inline documentation
- Commit messages - Change documentation

## Testing

### Manual Testing

1. Build and upload firmware
2. Test affected functionality
3. Verify no regressions
4. Test edge cases
5. Check error handling

### Test Checklist

- [ ] Device boots correctly
- [ ] WiFi connection works
- [ ] Web interface accessible
- [ ] TOTP codes generate correctly
- [ ] Password manager functions
- [ ] BLE transmission works
- [ ] PIN protection works
- [ ] Factory reset works

## Questions?

- Open a [Discussion](https://github.com/makepkg/SecureGen/discussions)
- Check existing [Issues](https://github.com/makepkg/SecureGen/issues)
- Read the [Documentation](docs/)

## Recognition

Contributors are recognized in:
- Release notes
- GitHub contributors page
- Project acknowledgments

Thank you for contributing! 🎉
