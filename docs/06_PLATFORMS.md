# JinGo VPN - Platform Build Guide

[中文文档](06_PLATFORMS_zh.md)

## 1. macOS

### System Requirements

| Item | Requirement |
|------|-------------|
| macOS | 12.0+ (Monterey) |
| Xcode | 15.0+ |
| Qt | 6.10.0+ (macOS component) |

### Supported Architectures

- **arm64**: Apple Silicon (M1/M2/M3/M4)
- **x86_64**: Intel Mac

Default builds Universal Binary (supports both architectures).

### Environment Setup

```bash
# Install Xcode command line tools
xcode-select --install
```

Use Qt Online Installer to install Qt 6.10 → macOS.

### Build

1. Edit `scripts/build/build-macos.sh` line 28 to configure Qt path:

```bash
QT_MACOS_PATH="/your/path/to/Qt/6.10.0/macos"
```

2. Build:

```bash
# Debug build (no signing by default)
./scripts/build/build-macos.sh

# Release build
./scripts/build/build-macos.sh --release

# Clean and rebuild
./scripts/build/build-macos.sh --clean

# With code signing
./scripts/build/build-macos.sh --release --sign --team-id YOUR_TEAM_ID
```

3. Output: `build-macos/bin/Debug/JinGo.app` or `build-macos/bin/Release/JinGo.app`

### Running

macOS TUN device requires administrator privileges:

```bash
sudo open build-macos/bin/Debug/JinGo.app
```

---

## 2. Windows

### System Requirements

| Item | Requirement |
|------|-------------|
| Windows | 10/11 (64-bit) |
| Qt | 6.10.0+ (MinGW 64-bit component) |

### Environment Setup

Use Qt Online Installer to install Qt 6.10 → MinGW 64-bit. Qt installer will automatically install the required MinGW compiler.

### Build

1. Edit `scripts/build/build-windows.ps1` for Qt path.

2. Build:

```powershell
# Run in PowerShell
.\scripts\build\build-windows.ps1 -Release

# Or use wrapper
.\scripts\build\build-windows-wrapper.bat
```

3. Output: `build-windows\bin\JinGo.exe`

### Running

First run requires Administrator privileges to install WinTun driver.

---

## 3. Linux

### System Requirements

| Item | Requirement |
|------|-------------|
| Distribution | Ubuntu 20.04+, Debian 11+, Fedora 35+ |
| Architecture | x86_64 (64-bit) |
| Qt | 6.10.0+ |

### Install Dependencies

**Ubuntu/Debian:**

```bash
sudo apt install -y \
    build-essential cmake ninja-build \
    libgl1-mesa-dev libxcb1-dev libxcb-*-dev \
    libxkbcommon-dev libxkbcommon-x11-dev
```

**Fedora:**

```bash
sudo dnf install -y \
    @development-tools cmake ninja-build \
    mesa-libGL-devel libxcb-devel xcb-util-*-devel \
    libxkbcommon-devel libxkbcommon-x11-devel
```

### Build

1. Edit `scripts/build/build-linux.sh` line 28 to configure Qt path:

```bash
QT_DIR="/your/path/to/Qt/6.10.0/gcc_64"
```

2. Build:

```bash
# Debug
./scripts/build/build-linux.sh

# Release
./scripts/build/build-linux.sh --release
```

3. Output: `build-linux/bin/JinGo`

### Running

```bash
# Method 1: Set capabilities
sudo setcap cap_net_admin+eip build-linux/bin/JinGo

# Method 2: Run as root
sudo ./build-linux/bin/JinGo
```

---

## 4. Android

### System Requirements

| Item | Requirement |
|------|-------------|
| Android SDK | API 28+ (Android 9.0) |
| Android NDK | 27.2.12479018 |
| Java | JDK 17+ |
| Qt | 6.10.0+ (Android component) |

### Supported Architectures

- **arm64-v8a**: 64-bit ARM (mainstream devices)
- **armeabi-v7a**: 32-bit ARM (older devices)

### Environment Setup

1. Install Android Studio: https://developer.android.com/studio
2. Install via SDK Manager:
   - SDK Platform: Android 14 (API 34)
   - NDK: 27.2.12479018
3. Use Qt Online Installer to install Qt 6.10 → Android

### Build

1. Edit `scripts/build/build-android.sh`:

```bash
QT_BASE_PATH="/your/path/to/Qt/6.10"
ANDROID_SDK_ROOT="/path/to/Android/sdk"
ANDROID_NDK_VERSION="27.2.12479018"
```

2. Build:

```bash
# Build arm64
./scripts/build/build-android.sh --abi arm64-v8a

# Build all architectures
./scripts/build/build-android.sh --abi all

# Install to device
./scripts/build/build-android.sh --abi arm64-v8a --install
```

3. Output: `build-android/android-build/build/outputs/apk/`

### Debugging

```bash
adb logcat -s JinGo:V
```

---

## 5. iOS

### System Requirements

| Item | Requirement |
|------|-------------|
| macOS | 12.0+ |
| Xcode | 15.0+ |
| iOS Target | iOS 15.0+ |
| Qt | 6.10.0+ (iOS component) |
| Apple Developer | Developer account required |

### Environment Setup

```bash
# Install Xcode command line tools
xcode-select --install

# Accept license
sudo xcodebuild -license accept
```

Use Qt Online Installer to install Qt 6.10 → iOS.

### Build

1. Edit `scripts/build/build-ios.sh`:

```bash
QT_IOS_PATH="/your/path/to/Qt/6.10.0/ios"
TEAM_ID="YOUR_TEAM_ID"
CODE_SIGN_IDENTITY="Apple Development"
```

2. Build:

```bash
# Generate Xcode project (recommended)
./scripts/build/build-ios.sh --xcode
open build-ios/JinGo.xcodeproj

# Command line build
./scripts/build/build-ios.sh --release --team-id YOUR_TEAM_ID

# Simulator build
./scripts/build/build-ios.sh --simulator
```

3. Output: `build-ios/bin/Debug-iphoneos/JinGo.app`

### Signing Configuration

1. Create App ID in Apple Developer
2. Create Provisioning Profile
3. Configure signing in Xcode

---

## Related Documentation

- [Build Guide](02_BUILD_GUIDE.md)
- [Troubleshooting](05_TROUBLESHOOTING.md)
