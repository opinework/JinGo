# JinGo VPN - 平台构建指南

## 1. macOS

### 系统要求

| 项目 | 要求 |
|------|------|
| macOS | 12.0+ (Monterey) |
| Xcode | 15.0+ |
| Qt | 6.5+ (macOS 组件) |

### 支持架构

- **arm64**: Apple Silicon (M1/M2/M3/M4)
- **x86_64**: Intel Mac

默认构建 Universal Binary (同时支持两种架构)。

### 环境配置

```bash
# 安装 Xcode 命令行工具
xcode-select --install
```

使用 Qt 在线安装器安装 Qt 6.5 → macOS。

### 构建

1. 编辑 `scripts/build/build-macos.sh` 第 28 行配置 Qt 路径：

```bash
QT_MACOS_PATH="/your/path/to/Qt/6.x.x/macos"
```

2. 编译：

```bash
# Debug 构建
./scripts/build/build-macos.sh --skip-sign

# Release 构建
./scripts/build/build-macos.sh --release --skip-sign

# 清理后重新构建
./scripts/build/build-macos.sh --clean --skip-sign
```

3. 输出：`build-macos/bin/Debug/JinGo.app` 或 `build-macos/bin/Release/JinGo.app`

### 运行

macOS 使用 TUN 设备需要管理员权限：

```bash
sudo open build-macos/bin/Debug/JinGo.app
```

---

## 2. Windows

### 系统要求

| 项目 | 要求 |
|------|------|
| Windows | 10/11 (64位) |
| Qt | 6.5+ (MinGW 64-bit 组件) |

### 环境配置

使用 Qt 在线安装器安装 Qt 6.5 → MinGW 64-bit。Qt 安装程序会自动安装所需的 MinGW 编译器。

### 构建

1. 编辑 `scripts/build/build-windows.ps1` 开头的 Qt 路径。

2. 编译：

```powershell
# 在 PowerShell 中运行
.\scripts\build\build-windows.ps1 -Release

# 或使用 wrapper
.\scripts\build\build-windows-wrapper.bat
```

3. 输出：`build-windows\bin\JinGo.exe`

### 运行

首次运行需要以管理员身份运行以安装 WinTun 驱动。

---

## 3. Linux

### 系统要求

| 项目 | 要求 |
|------|------|
| 发行版 | Ubuntu 20.04+, Debian 11+, Fedora 35+ |
| 架构 | x86_64 (64位) |
| Qt | 6.5+ |

### 依赖安装

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

### 构建

1. 编辑 `scripts/build/build-linux.sh` 第 28 行配置 Qt 路径：

```bash
QT_DIR="/your/path/to/Qt/6.x.x/gcc_64"
```

2. 编译：

```bash
# Debug
./scripts/build/build-linux.sh

# Release
./scripts/build/build-linux.sh --release
```

3. 输出：`build-linux/bin/JinGo`

### 运行

```bash
# 方法 1: 设置 capabilities
sudo setcap cap_net_admin+eip build-linux/bin/JinGo

# 方法 2: 以 root 运行
sudo ./build-linux/bin/JinGo
```

---

## 4. Android

### 系统要求

| 项目 | 要求 |
|------|------|
| Android SDK | API 28+ (Android 9.0) |
| Android NDK | 27.2.12479018 |
| Java | JDK 17+ |
| Qt | 6.5+ (Android 组件) |

### 支持架构

- **arm64-v8a**: 64位 ARM (主流设备)
- **armeabi-v7a**: 32位 ARM (旧设备)

### 环境配置

1. 安装 Android Studio: https://developer.android.com/studio
2. 通过 SDK Manager 安装：
   - SDK Platform: Android 14 (API 34)
   - NDK: 27.2.12479018
3. 使用 Qt 在线安装器安装 Qt 6.5 → Android

### 构建

1. 编辑 `scripts/build/build-android.sh` 开头：

```bash
QT_BASE_PATH="/your/path/to/Qt/6.5"
ANDROID_SDK_ROOT="/path/to/Android/sdk"
ANDROID_NDK_VERSION="27.2.12479018"
```

2. 编译：

```bash
# 构建 arm64
./scripts/build/build-android.sh --abi arm64-v8a

# 构建所有架构
./scripts/build/build-android.sh --abi all

# 安装到设备
./scripts/build/build-android.sh --abi arm64-v8a --install
```

3. 输出：`build-android/android-build/build/outputs/apk/`

### 调试

```bash
adb logcat -s JinGo:V
```

---

## 5. iOS

### 系统要求

| 项目 | 要求 |
|------|------|
| macOS | 12.0+ |
| Xcode | 15.0+ |
| iOS 目标 | iOS 15.0+ |
| Qt | 6.5+ (iOS 组件) |
| Apple Developer | 需要开发者账号 |

### 环境配置

```bash
# 安装 Xcode 命令行工具
xcode-select --install

# 接受许可
sudo xcodebuild -license accept
```

使用 Qt 在线安装器安装 Qt 6.5 → iOS。

### 构建

1. 编辑 `scripts/build/build-ios.sh` 开头：

```bash
QT_IOS_PATH="/your/path/to/Qt/6.x.x/ios"
TEAM_ID="YOUR_TEAM_ID"
CODE_SIGN_IDENTITY="Apple Development"
```

2. 编译：

```bash
# 生成 Xcode 项目（推荐）
./scripts/build/build-ios.sh --xcode
open build-ios/JinGo.xcodeproj

# 命令行构建
./scripts/build/build-ios.sh --release

# 模拟器构建
./scripts/build/build-ios.sh --simulator
```

3. 输出：`build-ios/bin/Debug-iphoneos/JinGo.app`

### 签名配置

1. 在 Apple Developer 创建 App ID
2. 创建 Provisioning Profile
3. 在 Xcode 中配置签名

---

## 相关文档

- [构建指南](02_BUILD_GUIDE.md)
- [故障排除](05_TROUBLESHOOTING.md)
