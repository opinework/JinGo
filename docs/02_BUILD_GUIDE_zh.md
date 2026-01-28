# JinGo VPN - 构建指南

## 快速开始

JinGo 的构建非常简单，只需要：

1. **安装 Qt 6.10+**（推荐 6.10.0 或更高版本）
2. **修改构建脚本中的 Qt 路径**（或使用自动检测）
3. **运行构建脚本**

所有依赖库（JinDoCore、OpenSSL、Superray 等）已预编译并包含在 `third_party/` 目录中。

> **注意**：Windows 平台的构建脚本已支持自动环境检测，无需手动配置路径。

## 依赖关系

```
JinGo (Qt 应用)
├── JinDoCore (静态库)      → third_party/jindo/
│   └── 核心业务逻辑、API 客户端、VPN 管理
├── Superray (动态库)       → third_party/superray/
│   └── VPN 核心引擎 (Xray)
├── OpenSSL (静态库)        → third_party/*_openssl/
│   └── 加密支持
└── WinTun (Windows)        → third_party/wintun/
    └── Windows TUN 驱动
```

**所有依赖库已预编译，无需手动编译。**

## 目录结构

```
JinGo/
├── third_party/
│   ├── jindo/                    # JinDoCore 静态库 (核心)
│   │   ├── android/              # Android 各架构
│   │   ├── apple/                # macOS/iOS
│   │   ├── linux/
│   │   └── windows/
│   ├── superray/                 # VPN 引擎
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
└── src/                          # 源代码
```

## 应用配置

配置文件位于 `resources/bundle_config.json`，用于定义应用的基本信息和服务端点。

### 配置项说明

```json
{
    "config": {
        "panelUrl": "https://cp.jingo.cfd",        // 控制面板 URL（用户订阅）
        "appName": "JinGo",                         // 应用名称
        "supportEmail": "support@jingo.cfd",        // 支持邮箱
        "privacyPolicyUrl": "https://...",          // 隐私政策链接
        "termsOfServiceUrl": "https://...",         // 服务条款链接
        "telegramUrl": "https://t.me/...",          // Telegram 群组
        "discordUrl": "https://discord.gg/...",     // Discord 服务器
        "docsUrl": "https://docs.opine.work",       // 文档链接
        "issuesUrl": "https://opine.work/issues",   // 问题反馈链接
        "latencyTestUrl": "https://www.google.com/generate_204",  // 延迟测试
        "ipInfoUrl": "https://ipinfo.io/json",      // IP 信息查询
        "speedTestBaseUrl": "https://speed.cloudflare.com/__down?bytes=",  // 测速
        "hideSubscriptionBlock": true,              // 隐藏订阅区块
        "updateCheckUrl": "https://..."             // 更新检查
    }
}
```

### 主要配置项

| 配置项 | 说明 |
|--------|------|
| `panelUrl` | 用户订阅面板地址，用于获取节点配置 |
| `appName` | 应用显示名称 |
| `supportEmail` | 用户支持邮箱 |
| `hideSubscriptionBlock` | 是否隐藏界面中的订阅区块 |
| `latencyTestUrl` | 节点延迟测试使用的 URL |
| `ipInfoUrl` | 获取当前 IP 信息的 API |
| `speedTestBaseUrl` | 测速下载基础 URL |

### 授权验证（已禁用）

当前版本已移除授权验证功能，配置文件中的以下字段保留但不生效：

```json
{
    "license": { ... },        // 授权信息（已禁用）
    "licenseServer": { ... }   // 授权服务器（已禁用）
}
```

应用启动时不再进行授权检查，可直接使用全部功能。

---

## 环境搭建

### 第一步：安装 Qt

**版本要求：Qt 6.10+**（推荐 Qt 6.10.0 或更高版本）

1. 下载 Qt 在线安装器：https://www.qt.io/download-qt-installer
2. 运行安装器，选择 Qt 6.10 或更高版本
3. 根据目标平台选择组件：

| 目标平台 | 需要安装的 Qt 组件 |
|---------|-------------------|
| macOS | Qt 6.10+ → macOS |
| iOS | Qt 6.10+ → iOS |
| Android | Qt 6.10+ → Android (arm64-v8a, armeabi-v7a) |
| Linux | Qt 6.10+ → Desktop gcc 64-bit |
| Windows | Qt 6.10+ → MinGW 64-bit |

> **注意**：项目使用 Qt 6.10.0/6.10.1。Windows 平台构建脚本支持自动检测 Qt 和 MinGW 环境。

### 第二步：平台特定工具

| 平台 | 额外需要 |
|------|---------|
| macOS | Xcode (App Store 安装) |
| iOS | Xcode + Apple Developer 账号 |
| Android | Android Studio (SDK + NDK) |
| Linux | GCC: `sudo apt install build-essential cmake` |
| Windows | 无（Qt 自带 MinGW） |

## macOS 构建

### 1. 配置 Qt 路径

编辑 `scripts/build/build-macos.sh` 第 28 行：

```bash
QT_MACOS_PATH="/your/path/to/Qt/6.x.x/macos"
```

### 2. 构建

```bash
# 构建（跳过签名，开发测试用）
./scripts/build/build-macos.sh --skip-sign

# 清理后重新构建
./scripts/build/build-macos.sh --clean --skip-sign

# Release 版本
./scripts/build/build-macos.sh --release --skip-sign
```

### 3. 运行

```bash
# 以 root 权限运行（TUN 设备需要）
sudo open build-macos/bin/Debug/JinGo.app
```

## iOS 构建

### 1. 配置

编辑 `scripts/build/build-ios.sh` 开头：

```bash
QT_IOS_PATH="/your/path/to/Qt/6.x.x/ios"
TEAM_ID="YOUR_TEAM_ID"
```

### 2. 构建

```bash
# 生成 Xcode 项目后在 Xcode 中构建
./scripts/build/build-ios.sh --xcode
open build-ios/JinGo.xcodeproj
```

## Android 构建

### 1. 安装 Android SDK/NDK

通过 Android Studio 安装：
- SDK Platform: Android 14 (API 34)
- NDK: 27.2.12479018

### 2. 配置

编辑 `scripts/build/build-android.sh` 开头：

```bash
QT_BASE_PATH="/your/path/to/Qt/6.x.x"
ANDROID_SDK_ROOT="/path/to/Android/sdk"
ANDROID_NDK_VERSION="27.2.12479018"
```

### 3. 构建

```bash
./scripts/build/build-android.sh --abi arm64-v8a
```

## Linux 构建

### 1. 安装依赖

```bash
sudo apt install -y build-essential cmake ninja-build \
    libgl1-mesa-dev libxcb1-dev libxcb-*-dev \
    libxkbcommon-dev libxkbcommon-x11-dev \
    libglib2.0-dev libsecret-1-dev
```

### 2. 配置 Qt 路径

#### 方法一：设置环境变量（推荐）

```bash
export QT_DIR="/mnt/dev/Qt/6.10.1/gcc_64"
# 或
export Qt6_DIR="/mnt/dev/Qt/6.10.1/gcc_64"
```

#### 方法二：修改构建脚本

编辑 `scripts/build/build-linux.sh` 第 35 行：

```bash
QT_DIR="/mnt/dev/Qt/6.10.1/gcc_64"
```

### 3. 使用脚本构建

#### 基本构建命令

```bash
# Debug 模式构建（默认）
./scripts/build/build-linux.sh

# Release 模式构建
./scripts/build/build-linux.sh --release

# 清理后重新构建
./scripts/build/build-linux.sh --clean --release

# 部署 Qt 依赖库
./scripts/build/build-linux.sh --release --deploy

# 创建安装包（DEB/RPM/TGZ）
./scripts/build/build-linux.sh --release --package
```

#### 构建选项说明

| 选项 | 说明 |
|------|------|
| `-c, --clean` | 清理构建目录后重新构建 |
| `-d, --debug` | Debug 模式构建（默认） |
| `-r, --release` | Release 模式构建 |
| `-p, --package` | 打包 DEB/RPM/TGZ |
| `--deploy` | 部署 Qt 依赖库和插件 |
| `-t, --translate` | 更新翻译 |
| `-b, --brand NAME` | 应用白标定制 |
| `-v, --verbose` | 显示详细输出 |

#### 构建示例

```bash
# 示例1: 编译 Release 版本
./scripts/build/build-linux.sh --release

# 示例2: 清理并编译 Release 版本
./scripts/build/build-linux.sh --clean --release

# 示例3: 编译 Release 版本并部署依赖
./scripts/build/build-linux.sh --release --deploy

# 示例4: 编译并打包
./scripts/build/build-linux.sh --release --package

# 示例5: 使用白标定制编译
./scripts/build/build-linux.sh --brand jingo --release --package
```

### 4. 构建输出

构建成功后，文件位于：

```
build-linux/
├── bin/
│   ├── JinGo                    # 主可执行文件
│   └── lib/                     # OpenSSL 依赖库
│       ├── libssl.so.3
│       └── libcrypto.so.3
└── build.log                    # 构建日志
```

Release 模式会额外生成：

```
release/
└── jingo-1.0.0-20260126-linux.tar.gz    # 发布包
```

### 5. 设置 TUN 权限

Linux 需要网络管理权限才能创建 TUN 设备：

```bash
# 方式一：设置 capability（推荐，无需 root 运行）
sudo setcap cap_net_admin+eip ./build-linux/bin/JinGo

# 方式二：使用 root 权限运行
sudo ./build-linux/bin/JinGo
```

### 6. 运行应用

```bash
# 如果已设置 capability
./build-linux/bin/JinGo

# 或使用 sudo
sudo ./build-linux/bin/JinGo
```

### 7. 构建故障排查

#### Qt 找不到

```bash
# 检查 Qt 是否安装
ls -la /mnt/dev/Qt/6.10.1/gcc_64/bin/qmake

# 设置环境变量
export QT_DIR=/mnt/dev/Qt/6.10.1/gcc_64
```

#### 缺少依赖库

```bash
# Ubuntu/Debian
sudo apt install -y libglib2.0-dev libsecret-1-dev

# Fedora/RHEL
sudo dnf install -y glib2-devel libsecret-devel
```

#### 编译速度慢

```bash
# 安装 Ninja 构建工具（推荐）
sudo apt install ninja-build

# 脚本会自动检测并使用 Ninja
```

## Windows 构建

### 1. 环境要求

- **Qt**: 6.10.0 或更高版本（MinGW 64-bit）
- **MinGW**: 13.1.0 或更高版本（通常随 Qt 安装）
- **CMake**: 3.21+ （可选，脚本会自动使用 Qt 自带的 CMake）
- **PowerShell**: Windows PowerShell 5.1+ 或 PowerShell Core 7+
- **WiX Toolset 6.0**: （可选，用于生成 MSI 安装包）

### 2. 自动环境检测

构建脚本会自动检测以下路径（按优先级）：

**Qt 安装路径**：
- `D:\Qt\6.10.1\mingw_64`
- `D:\Qt\6.10.0\mingw_64`
- `C:\Qt\6.10.1\mingw_64`
- `C:\Qt\6.10.0\mingw_64`
- `%USERPROFILE%\Qt\6.10.x\mingw_64`

**MinGW 编译器路径**：
- `D:\Qt\Tools\mingw1400_64`
- `D:\Qt\Tools\mingw1310_64`
- `C:\Qt\Tools\mingw1400_64`
- `%USERPROFILE%\Qt\Tools\mingw13xx_64`

**CMake 路径**（如果系统 PATH 中没有）：
- `D:\Qt\Tools\CMake_64\bin`
- `C:\Qt\Tools\CMake_64\bin`

> **提示**：如果 Qt 安装在非标准位置，可以设置环境变量：
> ```powershell
> $env:Qt6_DIR = "D:\CustomPath\Qt\6.10.0\mingw_64"
> $env:MINGW_DIR = "D:\CustomPath\Qt\Tools\mingw1310_64"
> ```

### 3. 构建方法

#### 方法一：使用 PowerShell 脚本（推荐）

```powershell
# 基本编译（Release 模式）
.\scripts\build\build-windows.ps1

# 清理后重新编译
.\scripts\build\build-windows.ps1 -Clean

# Debug 模式编译
.\scripts\build\build-windows.ps1 -DebugBuild

# 清理并编译 Debug 版本
.\scripts\build\build-windows.ps1 -Clean -DebugBuild

# 指定品牌编译（白标定制）
.\scripts\build\build-windows.ps1 -Clean -Brand "1"

# 仅更新翻译文件
.\scripts\build\build-windows.ps1 -UpdateTranslations

# 仅编译翻译
.\scripts\build\build-windows.ps1 -TranslationsOnly
```

#### 方法二：使用批处理包装脚本

```batch
# 基本编译
scripts\build\build-windows-wrapper.bat

# 清理后重新编译
scripts\build\build-windows-wrapper.bat --clean

# Debug 模式编译
scripts\build\build-windows-wrapper.bat --debug

# 清理并编译 Debug 版本
scripts\build\build-windows-wrapper.bat --clean --debug

# 仅编译翻译
scripts\build\build-windows-wrapper.bat --translations
```

### 4. 构建参数说明

| 参数 | PowerShell | 批处理 | 说明 |
|------|-----------|--------|------|
| 清理构建 | `-Clean` | `--clean` / `-c` | 清理 build-windows 目录后重新编译 |
| Debug 模式 | `-DebugBuild` | `--debug` / `-d` | 编译 Debug 版本 |
| 仅翻译 | `-TranslationsOnly` | `--translations` / `-t` | 仅编译翻译文件 |
| 更新翻译 | `-UpdateTranslations` | - | 运行 lupdate/lrelease 更新翻译 |
| 指定品牌 | `-Brand "1"` | - | 使用指定白标品牌配置 |
| 帮助 | - | `--help` / `-h` | 显示帮助信息 |

### 5. 构建流程

脚本会自动执行以下步骤：

1. **[0/4] 复制白标资源**
   - 从 `white-labeling/<Brand>/` 复制品牌配置
   - 复制图标文件（app.png, app.ico, app.icns）
   - 替换授权公钥（如果有）

2. **[1/4] 配置 CMake**
   - 检测 Qt 和 MinGW 环境
   - 生成 MinGW Makefiles
   - 配置构建类型（Debug/Release）

3. **[2/4] 编译应用**
   - 编译 C++ 源代码
   - 处理 QML 文件
   - 编译翻译文件

4. **[3/4] 部署依赖**
   - 自动运行 windeployqt
   - 复制 Qt DLLs 和插件
   - 复制 MinGW 运行时 DLLs
   - 复制 superray.dll 和 wintun.dll

5. **[4/4] 创建打包**
   - 生成 ZIP 便携版：`pkg/jingo-{version}-{date}-windows.zip`
   - 生成 MSI 安装包（如果安装了 WiX）：`pkg/jingo-{version}-{date}-windows.msi`
   - 复制到 `release/` 目录（Release 模式）

### 6. 输出文件

构建成功后，输出文件位于：

| 类型 | 路径 | 说明 |
|------|------|------|
| 可执行文件 | `build-windows/bin/JinGo.exe` | 构建输出（需要 DLLs） |
| 部署目录 | `pkg/JinGo-1.0.0/` | 包含所有文件的完整部署 |
| ZIP 包 | `pkg/jingo-1.0.0-{date}-windows.zip` | 便携版压缩包 |
| MSI 安装包 | `pkg/jingo-1.0.0-{date}-windows.msi` | Windows 安装程序 |
| Release 目录 | `release/jingo-1.0.0-{date}-windows.zip` | Release 版本输出 |

### 7. 安装 WiX Toolset（可选）

如需生成 MSI 安装包，需要安装 WiX Toolset 6.0：

```powershell
# 使用 .NET CLI 安装（推荐）
dotnet tool install --global wix

# 验证安装
wix --version
```

安装后脚本会自动检测并生成 MSI 安装包。

### 8. 运行测试

```powershell
# 直接运行构建输出
.\build-windows\bin\JinGo.exe

# 或从部署目录运行
.\pkg\JinGo-1.0.0\JinGo.exe

# 或解压 ZIP 包后运行
Expand-Archive -Path .\pkg\jingo-*.zip -DestinationPath .\test
.\test\JinGo-1.0.0\JinGo.exe
```

## 构建选项速查

| 选项 | 说明 |
|------|------|
| `--clean` / `-c` | 清理后重新构建 |
| `--release` / `-r` | Release 模式 |
| `--debug` / `-d` | Debug 模式（默认） |
| `--skip-sign` / `-s` | 跳过签名（macOS） |
| `--xcode` / `-x` | 生成 Xcode 项目 |
| `--abi <ABI>` | Android 架构 |

## 输出位置

| 平台 | Debug | Release |
|------|-------|---------|
| macOS | `build-macos/bin/Debug/JinGo.app` | `build-macos/bin/Release/JinGo.app` |
| iOS | `build-ios/bin/Debug-iphoneos/` | `build-ios/bin/Release-iphoneos/` |
| Android | `build-android/android-build/` | 同左 |
| Linux | `build-linux/bin/JinGo` | 同左 |
| Windows | `build-windows/bin/JinGo.exe` | 同左 |

## 常见问题

### Q: Qt 找不到？

确保构建脚本开头的 Qt 路径正确，并且该目录存在。

### Q: macOS 运行闪退？

需要 root 权限运行：`sudo open build-macos/bin/Debug/JinGo.app`

### Q: Linux 网络不工作？

需要 root 权限或设置 capabilities：
```bash
sudo setcap cap_net_admin+eip build-linux/bin/JinGo
```

### Q: Android NDK 错误？

确保 `ANDROID_NDK_VERSION` 与实际安装的 NDK 版本一致。

---

## GitHub Actions 自动编译

项目支持通过 GitHub Actions 手动触发全平台自动编译，无需本地环境配置。

### 准备工作

#### 1. 配置白标品牌

在 `white-labeling/` 目录下创建或修改品牌配置：

```
white-labeling/
├── 1/                         # 品牌 1
│   ├── bundle_config.json     # 品牌配置
│   └── icons/                 # 品牌图标
│       ├── app.png            # 主图标 (1024x1024)
│       ├── app.icns           # macOS
│       ├── app.ico            # Windows
│       ├── ios/               # iOS 各尺寸
│       └── android/           # Android 各密度
```

#### 2. 编辑 bundle_config.json

```json
{
    "config": {
        "panelUrl": "https://your-panel.com",
        "appName": "YourBrand VPN",
        "supportEmail": "support@yourbrand.com",
        "privacyPolicyUrl": "https://yourbrand.com/privacy",
        "termsOfServiceUrl": "https://yourbrand.com/terms",
        "telegramUrl": "https://t.me/yourbrand",
        "docsUrl": "https://docs.yourbrand.com",
        "latencyTestUrl": "https://www.google.com/generate_204",
        "ipInfoUrl": "https://ipinfo.io/json",
        "speedTestBaseUrl": "https://speed.cloudflare.com/__down?bytes=",
        "hideSubscriptionBlock": false
    }
}
```

#### 3. 准备图标资源

| 文件/目录 | 说明 | 尺寸 |
|-----------|------|------|
| `app.png` | 源图标 | 1024x1024 |
| `app.icns` | macOS 图标 | 多尺寸打包 |
| `app.ico` | Windows 图标 | 多尺寸打包 |
| `ios/` | iOS 图标集 | 20-1024px |
| `android/mipmap-*/` | Android 图标 | mdpi-xxxhdpi |

### 触发编译

#### 方式一：GitHub 网页界面

1. 打开 GitHub 仓库页面
2. 点击 **Actions** 标签
3. 选择 **Build All Platforms** 工作流
4. 点击 **Run workflow**
5. 选择参数：
   - **Brand ID**: 选择白标品牌编号 (1-5)
   - **Build Type**: Debug 或 Release
   - **Platforms**: 选择构建平台 (all/单平台)
6. 点击 **Run workflow** 开始构建

#### 方式二：GitHub CLI

```bash
# 构建所有平台，品牌 1，Release 版本
gh workflow run build.yml \
  -f brand_id=1 \
  -f build_type=Release \
  -f platforms=all

# 仅构建 macOS，品牌 2，Debug 版本
gh workflow run build.yml \
  -f brand_id=2 \
  -f build_type=Debug \
  -f platforms=macos
```

### 构建参数

| 参数 | 说明 | 选项 |
|------|------|------|
| `brand_id` | 白标品牌编号 | 1, 2, 3, 4, 5 |
| `build_type` | 构建类型 | Debug, Release |
| `platforms` | 目标平台 | all, macos, windows, linux, android, ios |

### 产物下载

构建完成后，在 Actions 运行页面的 **Artifacts** 部分下载编译好的程序：

| 平台 | 产物名称 | 文件类型 | 说明 |
|------|----------|----------|------|
| macOS | `JinGo-macOS-BrandX` | `.app` (zip) | 可直接运行的应用程序 |
| Windows | `JinGo-Windows-BrandX` | `.exe` + DLLs (zip) | 解压后运行 JinGo.exe |
| Linux | `JinGo-Linux-BrandX` | 可执行文件 (tar.gz) | 解压后 `sudo ./JinGo` |
| Android | `JinGo-Android-BrandX` | `.apk` | 可直接安装的安装包 |
| iOS | `JinGo-iOS-BrandX` | `.ipa` (未签名) | 需使用 AltStore/Sideloadly 签名安装 |

### 构建时间参考

| 平台 | 预计时间 |
|------|----------|
| macOS | 10-15 分钟 |
| Windows | 15-20 分钟 |
| Linux | 10-15 分钟 |
| Android | 15-20 分钟 |
| iOS | 10-15 分钟 |
| **全平台** | **20-30 分钟** (并行) |

### 注意事项

1. **iOS 签名**：
   - GitHub 可以编译 iOS（使用 macOS runner + Xcode）
   - 产出的 .ipa 为**未签名**版本
   - 安装方法：使用 [AltStore](https://altstore.io/) 或 [Sideloadly](https://sideloadly.io/) 自签名安装
   - 如需分发：需配置 Apple Developer 证书到 GitHub Secrets

2. **Android 签名**：
   - 产出的 APK 为 debug 签名，可直接安装测试
   - 发布到应用商店需使用发布密钥重新签名

3. **macOS 公证**：
   - 产出的 .app 未经公证
   - 首次运行：右键 → 打开，或 `sudo xattr -rd com.apple.quarantine JinGo.app`

4. **Windows**：
   - 产出的 .exe 未签名，可能触发 SmartScreen 警告
   - 点击"更多信息" → "仍要运行"即可

5. **存储限制**：GitHub Actions 产物保留 90 天，请及时下载

---

## 下一步

- [开发指南](03_DEVELOPMENT.md)
- [白标定制](04_WHITE_LABELING.md)
- [故障排除](05_TROUBLESHOOTING.md)
