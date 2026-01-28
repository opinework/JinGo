# JinGo VPN

[English](README.md)

跨平台 VPN 客户端，基于 Qt 6 和 Xray 核心构建。

## 特性

- **跨平台支持**: Android、iOS、macOS、Windows、Linux
- **现代化界面**: Qt 6 QML 构建的流畅用户界面
- **多协议支持**: 基于 Xray 核心，支持 VMess、VLESS、Trojan、Shadowsocks 等
- **多语言**: 支持 8 种语言（中文、英文、越南语、高棉语、缅甸语、俄语、波斯语等）
- **白标定制**: 支持品牌定制和多租户部署

## 截图

<p align="center">
  <img src="images/connect.png" width="280" alt="连接界面" />
  <img src="images/servers.png" width="280" alt="服务器列表" />
  <img src="images/subscription.png" width="280" alt="订阅管理" />
</p>

<p align="center">
  <img src="images/setting.png" width="280" alt="设置界面" />
  <img src="images/profile.png" width="280" alt="个人资料" />
</p>

## 目录

- [特性](#特性)
- [截图](#截图)
- [快速开始](#快速开始)
- [平台支持](#平台支持)
- [文档](#文档)
- [多语言支持](#多语言支持)
- [技术栈](#技术栈)
- [构建选项](#构建选项)
- [开发](#开发)
- [联系方式](#联系方式)
- [许可证](#许可证)

## 快速开始

### 前置条件

- **Qt**: 6.10.0+（推荐 6.10.0 或更高版本）
- **CMake**: 3.21+
- **编译器**:
  - macOS/iOS: Xcode 15+
  - Android: NDK 27.2+
  - Windows: MinGW 13+ 或 Visual Studio 2022
  - Linux: GCC 11+ 或 Clang 14+

### 编译步骤

#### 1. Fork 仓库并配置白标

```bash
# 1. Fork 本仓库到自己的 GitHub 账号

# 2. Clone 你的 fork
git clone https://github.com/YOUR_USERNAME/JinGo.git
cd JinGo

# 3. 创建你的白标配置
cp -r white-labeling/1 white-labeling/YOUR_BRAND

# 4. 修改白标配置文件
# 编辑 white-labeling/YOUR_BRAND/bundle_config.json
{
    "panel_url": "https://your-api-server.com",
    "app_name": "YourApp",
    "support_email": "support@your-domain.com",
    ...
}

# 5. 替换应用图标
# 将你的图标放入 white-labeling/YOUR_BRAND/icons/
```

#### 2. 编译应用

所有构建脚本都在 `scripts/build/` 目录下：

```bash
# Android APK
./scripts/build/build-android.sh --release --abi arm64-v8a

# macOS App (Universal Binary: arm64 + x86_64)
./scripts/build/build-macos.sh --release

# iOS App (需要 Apple 开发者团队 ID)
./scripts/build/build-ios.sh --release --team-id YOUR_TEAM_ID

# Linux
./scripts/build/build-linux.sh --release

# Windows (PowerShell)
.\scripts\build\build-windows.ps1
```

#### 3. 指定白标品牌编译

```bash
./scripts/build/build-macos.sh --release --brand YOUR_BRAND
./scripts/build/build-android.sh --release --brand YOUR_BRAND
./scripts/build/build-ios.sh --release --brand YOUR_BRAND --team-id YOUR_TEAM_ID
```

### 输出位置

| 平台 | 输出文件 | 位置 |
|------|---------|------|
| Android | APK | `release/jingo-*-android.apk` |
| macOS | DMG | `release/jingo-*-macos.dmg` |
| iOS | IPA | `release/jingo-*-ios.ipa` |
| Windows | EXE/MSI | `release/jingo-*-windows.exe` |
| Linux | tar.gz | `release/jingo-*-linux.tar.gz` |

## 平台支持

| 平台 | 架构 | 最低版本 | 状态 |
|------|------|---------|------|
| Android | arm64-v8a, armeabi-v7a, x86_64 | API 28 (Android 9) | ✅ |
| iOS | arm64 | iOS 15.0 | ✅ |
| macOS | arm64, x86_64 | macOS 12.0 | ✅ |
| Windows | x64 | Windows 10 | ✅ |
| Linux | x64 | Ubuntu 20.04+ | ✅ |

## 文档

- [架构说明](docs/01_ARCHITECTURE_zh.md)
- [构建指南](docs/02_BUILD_GUIDE_zh.md)
- [开发指南](docs/03_DEVELOPMENT_zh.md)
- [白标定制](docs/04_WHITE_LABELING_zh.md)
- [故障排除](docs/05_TROUBLESHOOTING_zh.md)
- [平台指南](docs/06_PLATFORMS_zh.md)
- [面板扩展](docs/07_PANEL_EXTENSION_zh.md)

## 多语言支持

| 语言 | 代码 | 状态 |
|------|------|------|
| English | en_US | ✅ |
| 简体中文 | zh_CN | ✅ |
| 繁體中文 | zh_TW | ✅ |
| Tiếng Việt | vi_VN | ✅ |
| ភាសាខ្មែរ | km_KH | ✅ |
| မြန်မာဘာသာ | my_MM | ✅ |
| Русский | ru_RU | ✅ |
| فارسی | fa_IR | ✅ |

## 技术栈

- **UI 框架**: Qt 6.10.0+ (QML/Quick)
- **VPN 核心**: Xray-core (通过 SuperRay 封装)
- **网络**: Qt Network + OpenSSL
- **存储**: SQLite (Qt SQL)
- **安全存储**:
  - macOS/iOS: Keychain
  - Android: EncryptedSharedPreferences
  - Windows: DPAPI
  - Linux: libsecret

## 构建选项

### CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `USE_JINDO_LIB` | ON | 使用 JinDoCore 静态库 |
| `JINDO_ROOT` | `../JinDo` | JinDo 项目路径 |
| `CMAKE_BUILD_TYPE` | Debug | 构建类型 (Debug/Release) |

### 构建脚本选项

```bash
# 通用选项
--clean          # 清理构建目录
--release        # Release 模式
--debug          # Debug 模式

# Android 特定
--abi <ABI>      # 指定架构 (arm64-v8a/armeabi-v7a/x86_64/all)
--sign           # 签名 APK

# macOS 特定
--sign           # 启用代码签名（需要 Team ID）
--team-id ID     # Apple 开发者团队 ID

# iOS 特定（必须签名）
--team-id ID     # Apple 开发者团队 ID（必须）

# Linux 特定
--deploy         # 部署 Qt 依赖
--package        # 创建安装包
```

## 开发

### 代码风格

- C++17 标准
- Qt 编码规范
- 使用 `clang-format` 格式化

### 调试

```bash
# 启用详细日志
QT_LOGGING_RULES="*.debug=true" ./JinGo

# Android logcat
adb logcat -s JinGo:V SuperRay-JNI:V
```

## 联系方式

- Telegram 频道: [@OpineWorkPublish](https://t.me/OpineWorkPublish)
- Telegram 群组: [@OpineWorkOfficial](https://t.me/OpineWorkOfficial)

## 许可证

MIT License

---

**版本**: 1.0.0
**Qt 版本**: 6.10.0+
**最后更新**: 2026-01
