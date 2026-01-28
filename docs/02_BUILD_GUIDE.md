# JinGo VPN - Build Guide

[中文文档](02_BUILD_GUIDE_zh.md)

## Quick Start

Building JinGo is straightforward:

1. **Install Qt 6.10.0+** (recommended 6.10.0 or higher)
2. **Configure Qt path in build scripts** (or use auto-detection)
3. **Run the build script**

All dependencies (JinDoCore, OpenSSL, SuperRay, etc.) are pre-compiled and included in the `third_party/` directory.

> **Note**: Windows build scripts support automatic environment detection, no manual path configuration required.

## Dependencies

```
JinGo (Qt Application)
├── JinDoCore (Static Library)  → third_party/jindo/
│   └── Core business logic, API client, VPN management
├── SuperRay (Dynamic Library)  → third_party/superray/
│   └── VPN core engine (Xray)
├── OpenSSL (Static Library)    → third_party/*_openssl/
│   └── Encryption support
└── WinTun (Windows)            → third_party/wintun/
    └── Windows TUN driver
```

**All dependencies are pre-compiled. No manual compilation required.**

## Directory Structure

```
JinGo/
├── third_party/
│   ├── jindo/                    # JinDoCore static library (core)
│   │   ├── android/              # Android architectures
│   │   ├── apple/                # macOS/iOS
│   │   ├── linux/
│   │   └── windows/
│   ├── superray/                 # VPN engine
│   ├── android_openssl/
│   ├── apple_openssl/
│   ├── linux_openssl/
│   ├── windows_openssl/
│   └── wintun/
├── scripts/build/
│   ├── build-macos.sh
│   ├── build-ios.sh
│   ├── build-android.sh
│   ├── build-linux.sh
│   └── build-windows.ps1
└── src/                          # Source code
```

## Application Configuration

Configuration file is located at `resources/bundle_config.json`, defining app information and service endpoints.

### Configuration Options

```json
{
    "config": {
        "panelUrl": "https://cp.jingo.cfd",        // Control panel URL (user subscription)
        "appName": "JinGo",                         // Application name
        "supportEmail": "support@jingo.cfd",        // Support email
        "privacyPolicyUrl": "https://...",          // Privacy policy link
        "termsOfServiceUrl": "https://...",         // Terms of service link
        "telegramUrl": "https://t.me/...",          // Telegram group
        "discordUrl": "https://discord.gg/...",     // Discord server
        "docsUrl": "https://docs.opine.work",       // Documentation link
        "issuesUrl": "https://opine.work/issues",   // Issue tracking link
        "latencyTestUrl": "https://www.google.com/generate_204",  // Latency test
        "ipInfoUrl": "https://ipinfo.io/json",      // IP info query
        "speedTestBaseUrl": "https://speed.cloudflare.com/__down?bytes=",  // Speed test
        "hideSubscriptionBlock": true,              // Hide subscription block
        "updateCheckUrl": "https://..."             // Update check
    }
}
```

### Main Configuration Items

| Option | Description |
|--------|-------------|
| `panelUrl` | User subscription panel address for node configuration |
| `appName` | Application display name |
| `supportEmail` | User support email |
| `hideSubscriptionBlock` | Whether to hide subscription block in UI |
| `latencyTestUrl` | URL for node latency testing |
| `ipInfoUrl` | API for current IP info |
| `speedTestBaseUrl` | Speed test download base URL |

### License Verification (Disabled)

License verification has been removed in the current version. The following fields are preserved but not functional:

```json
{
    "license": { ... },        // License info (disabled)
    "licenseServer": { ... }   // License server (disabled)
}
```

The application starts without license checks, all features are available.

---

## Environment Setup

### Step 1: Install Qt

**Version requirement: Qt 6.10.0+** (recommended Qt 6.10.0 or higher)

1. Download Qt Online Installer: https://www.qt.io/download-qt-installer
2. Run installer, select Qt 6.10.0 or higher
3. Select components based on target platform:

| Target Platform | Required Qt Components |
|----------------|------------------------|
| macOS | Qt 6.10.0+ → macOS |
| iOS | Qt 6.10.0+ → iOS |
| Android | Qt 6.10.0+ → Android (arm64-v8a, armeabi-v7a) |
| Linux | Qt 6.10.0+ → Desktop gcc 64-bit |
| Windows | Qt 6.10.0+ → MinGW 64-bit |

> **Note**: Project uses Qt 6.10.0/6.10.1. Windows build scripts support automatic Qt and MinGW environment detection.

### Step 2: Platform-Specific Tools

| Platform | Additional Requirements |
|----------|------------------------|
| macOS | Xcode (install from App Store) |
| iOS | Xcode + Apple Developer Account |
| Android | Android Studio (SDK + NDK) |
| Linux | GCC: `sudo apt install build-essential cmake` |
| Windows | None (Qt includes MinGW) |

## macOS Build

### 1. Configure Qt Path

Edit `scripts/build/build-macos.sh` line 28:

```bash
QT_MACOS_PATH="/your/path/to/Qt/6.10.0/macos"
```

### 2. Build

```bash
# Build (no signing by default, for development)
./scripts/build/build-macos.sh

# Clean and rebuild
./scripts/build/build-macos.sh --clean

# Release version
./scripts/build/build-macos.sh --release

# With code signing
./scripts/build/build-macos.sh --release --sign --team-id YOUR_TEAM_ID
```

### 3. Run

```bash
# Run with root privileges (required for TUN device)
sudo open build-macos/bin/Debug/JinGo.app
```

## iOS Build

### 1. Configure

Edit `scripts/build/build-ios.sh`:

```bash
QT_IOS_PATH="/your/path/to/Qt/6.10.0/ios"
```

Or use command line option:

```bash
./scripts/build/build-ios.sh --team-id YOUR_TEAM_ID
```

### 2. Build

```bash
# Generate Xcode project and build in Xcode
./scripts/build/build-ios.sh --xcode --team-id YOUR_TEAM_ID
open build-ios/JinGo.xcodeproj

# Or command line build
./scripts/build/build-ios.sh --release --team-id YOUR_TEAM_ID
```

> **Note**: iOS builds require Apple Developer Team ID for signing.

## Android Build

### 1. Install Android SDK/NDK

Install via Android Studio:
- SDK Platform: Android 14 (API 34)
- NDK: 27.2.12479018

### 2. Configure

Edit `scripts/build/build-android.sh`:

```bash
QT_BASE_PATH="/your/path/to/Qt/6.10.0"
ANDROID_SDK_ROOT="/path/to/Android/sdk"
ANDROID_NDK_VERSION="27.2.12479018"
```

### 3. Build

```bash
./scripts/build/build-android.sh --abi arm64-v8a
```

## Linux Build

### 1. Install Dependencies

```bash
sudo apt install -y build-essential cmake ninja-build \
    libgl1-mesa-dev libxcb1-dev libxcb-*-dev \
    libxkbcommon-dev libxkbcommon-x11-dev \
    libglib2.0-dev libsecret-1-dev
```

### 2. Configure Qt Path

#### Method 1: Environment Variable (Recommended)

```bash
export QT_DIR="/mnt/dev/Qt/6.10.0/gcc_64"
# or
export Qt6_DIR="/mnt/dev/Qt/6.10.0/gcc_64"
```

#### Method 2: Edit Build Script

Edit `scripts/build/build-linux.sh` line 35:

```bash
QT_DIR="/mnt/dev/Qt/6.10.0/gcc_64"
```

### 3. Build

```bash
# Debug mode (default)
./scripts/build/build-linux.sh

# Release mode
./scripts/build/build-linux.sh --release

# Clean and rebuild
./scripts/build/build-linux.sh --clean --release

# Deploy Qt dependencies
./scripts/build/build-linux.sh --release --deploy

# Create installation package (DEB/RPM/TGZ)
./scripts/build/build-linux.sh --release --package
```

### 4. Build Options

| Option | Description |
|--------|-------------|
| `-c, --clean` | Clean build directory before building |
| `-d, --debug` | Debug mode build (default) |
| `-r, --release` | Release mode build |
| `-p, --package` | Package DEB/RPM/TGZ |
| `--deploy` | Deploy Qt dependencies and plugins |
| `-t, --translate` | Update translations |
| `-b, --brand NAME` | Apply white-label customization |
| `-v, --verbose` | Show detailed output |

### 5. Build Output

After successful build, files are located at:

```
build-linux/
├── bin/
│   ├── JinGo                    # Main executable
│   └── lib/                     # OpenSSL dependencies
│       ├── libssl.so.3
│       └── libcrypto.so.3
└── build.log                    # Build log
```

Release mode generates:

```
release/
└── jingo-1.0.0-20260126-linux.tar.gz    # Release package
```

### 6. Set TUN Permissions

Linux requires network admin permission to create TUN devices:

```bash
# Method 1: Set capability (recommended, no root required to run)
sudo setcap cap_net_admin+eip ./build-linux/bin/JinGo

# Method 2: Run with root privileges
sudo ./build-linux/bin/JinGo
```

## Windows Build

### 1. Requirements

- **Qt**: 6.10.0 or higher (MinGW 64-bit)
- **MinGW**: 13.1.0 or higher (usually installed with Qt)
- **CMake**: 3.21+ (optional, script uses Qt's bundled CMake)
- **PowerShell**: Windows PowerShell 5.1+ or PowerShell Core 7+
- **WiX Toolset 6.0**: (optional, for MSI installer generation)

### 2. Auto Environment Detection

Build script automatically detects these paths (by priority):

**Qt Installation Path**:
- `D:\Qt\6.10.1\mingw_64`
- `D:\Qt\6.10.0\mingw_64`
- `C:\Qt\6.10.1\mingw_64`
- `C:\Qt\6.10.0\mingw_64`

### 3. Build Methods

#### Method 1: PowerShell Script (Recommended)

```powershell
# Basic build (Release mode)
.\scripts\build\build-windows.ps1

# Clean and rebuild
.\scripts\build\build-windows.ps1 -Clean

# Debug mode
.\scripts\build\build-windows.ps1 -DebugBuild
```

#### Method 2: Batch Wrapper Script

```batch
scripts\build\build-windows-wrapper.bat
scripts\build\build-windows-wrapper.bat --clean
scripts\build\build-windows-wrapper.bat --debug
```

### 4. Output Files

| Type | Path |
|------|------|
| Executable | `build-windows/bin/JinGo.exe` |
| Deploy directory | `pkg/JinGo-1.0.0/` |
| ZIP package | `pkg/jingo-1.0.0-{date}-windows.zip` |
| MSI installer | `pkg/jingo-1.0.0-{date}-windows.msi` |

## Build Options Quick Reference

| Option | Description |
|--------|-------------|
| `--clean` / `-c` | Clean before rebuild |
| `--release` / `-r` | Release mode |
| `--debug` / `-d` | Debug mode (default) |
| `--sign` | Enable code signing (macOS) |
| `--team-id ID` | Apple Development Team ID |
| `--xcode` / `-x` | Generate Xcode project |
| `--abi <ABI>` | Android architecture |

## Output Locations

| Platform | Debug | Release |
|----------|-------|---------|
| macOS | `build-macos/bin/Debug/JinGo.app` | `build-macos/bin/Release/JinGo.app` |
| iOS | `build-ios/bin/Debug-iphoneos/` | `build-ios/bin/Release-iphoneos/` |
| Android | `build-android/android-build/` | Same |
| Linux | `build-linux/bin/JinGo` | Same |
| Windows | `build-windows/bin/JinGo.exe` | Same |

## FAQ

### Q: Qt not found?

Ensure the Qt path in build script is correct and the directory exists.

### Q: macOS app crashes on launch?

Root privileges required: `sudo open build-macos/bin/Debug/JinGo.app`

### Q: Linux network not working?

Root privileges or capabilities required:
```bash
sudo setcap cap_net_admin+eip build-linux/bin/JinGo
```

### Q: Android NDK error?

Ensure `ANDROID_NDK_VERSION` matches the actually installed NDK version.

---

## GitHub Actions Auto Build

Project supports GitHub Actions for cross-platform automated builds without local environment setup.

### Trigger Build

1. Open GitHub repository page
2. Click **Actions** tab
3. Select **Build All Platforms** workflow
4. Click **Run workflow**
5. Select parameters:
   - **Brand ID**: Select white-label brand number (1-5)
   - **Build Type**: Debug or Release
   - **Platforms**: Select build platform (all/single platform)

### Download Artifacts

After build completion, download from **Artifacts** section:

| Platform | File Type |
|----------|-----------|
| macOS | `.app` (zip) |
| Windows | `.exe` + DLLs (zip) |
| Linux | executable (tar.gz) |
| Android | `.apk` |
| iOS | `.ipa` (unsigned) |

---

## Next Steps

- [Development Guide](03_DEVELOPMENT.md)
- [White-labeling](04_WHITE_LABELING.md)
- [Troubleshooting](05_TROUBLESHOOTING.md)
