# JinGo VPN - 故障排除指南

## 编译问题

### Qt 找不到

**错误信息:**
```
CMake Error: Could not find Qt6
```

**解决方法:**

确保构建脚本开头的 Qt 路径正确：
```bash
# macOS
QT_MACOS_PATH="/path/to/Qt/6.x.x/macos"

# Linux
QT_DIR="/path/to/Qt/6.x.x/gcc_64"

# iOS
QT_IOS_PATH="/path/to/Qt/6.x.x/ios"
```

### Android NDK 版本不匹配

**错误信息:**
```
CMake Error: Android NDK not found or version mismatch
```

**解决方法:**

编辑 `build-android.sh` 中的 NDK 版本：
```bash
ANDROID_NDK_VERSION="27.2.12479018"
```

## 运行时问题

### VPN 连接失败

**可能原因:**

1. **服务器不可达** - 检查网络连接
2. **权限不足** - 见下方平台特定解决方案
3. **配置错误** - 检查服务器配置

### Linux: TUN 设备创建失败

**错误信息:**
```
Failed to create TUN device: Operation not permitted
```

**解决方法:**
```bash
# 方法 1: 设置权限
sudo setcap cap_net_admin+eip ./JinGo

# 方法 2: 以 root 运行
sudo ./JinGo
```

### macOS: 需要 Root 权限

macOS 使用 TUN 设备需要管理员权限：
```bash
sudo open build-macos/bin/Debug/JinGo.app
```

### Windows: WinTun 驱动安装失败

以管理员身份运行应用，驱动会自动安装。

### Android: VPN 权限被拒绝

1. 进入系统设置 → 应用 → JinGo
2. 清除应用数据
3. 重新打开应用并授权 VPN 权限

## 网络问题

### API 请求失败

**排查步骤:**

1. 检查网络连接
2. 验证 API 服务器地址
3. 启用详细日志：
```bash
QT_LOGGING_RULES="*.debug=true" ./JinGo
```

### SSL 证书错误

1. 检查系统时间是否正确
2. 更新系统 CA 证书

## UI 问题

### QML 加载失败

**解决方法:**

检查 Qt QML 模块是否安装完整。

### 高 DPI 显示问题

```bash
# 设置缩放因子
export QT_SCALE_FACTOR=1.5

# 或自动检测
export QT_AUTO_SCREEN_SCALE_FACTOR=1
```

### Linux Wayland 显示问题

```bash
# 强制使用 X11
QT_QPA_PLATFORM=xcb ./JinGo
```

## 数据问题

### 清理应用数据

```bash
# Linux
rm -rf ~/.local/share/JinGo/
rm -rf ~/.cache/JinGo/

# macOS
rm -rf ~/Library/Application\ Support/JinGo/
rm -rf ~/Library/Caches/JinGo/

# Windows
rmdir /s %APPDATA%\JinGo
rmdir /s %LOCALAPPDATA%\JinGo
```

## 日志和调试

### 启用详细日志

```bash
# 全部调试日志
QT_LOGGING_RULES="*.debug=true" ./JinGo

# Android
adb logcat -s JinGo:V
```

### 日志文件位置

| 平台 | 位置 |
|------|------|
| Linux | `~/.local/share/JinGo/logs/` |
| macOS | `~/Library/Logs/JinGo/` |
| Windows | `%APPDATA%\JinGo\logs\` |

## 平台特定问题速查

### Android

| 问题 | 解决方法 |
|------|----------|
| 后台被杀 | 添加到电池优化白名单 |
| 通知不显示 | 检查通知权限 |

### iOS

| 问题 | 解决方法 |
|------|----------|
| VPN 配置失败 | 检查 Network Extension 权限 |
| 证书错误 | 重新签名应用 |

### macOS

| 问题 | 解决方法 |
|------|----------|
| 权限不足 | 以 sudo 运行 |

### Windows

| 问题 | 解决方法 |
|------|----------|
| DLL 缺失 | 重新运行 windeployqt |
| 驱动安装失败 | 以管理员运行 |

### Linux

| 问题 | 解决方法 |
|------|----------|
| 权限不足 | 设置 CAP_NET_ADMIN 或 sudo |
| 库找不到 | 设置 LD_LIBRARY_PATH |

## 相关文档

- [构建指南](02_BUILD_GUIDE.md)
- [开发指南](03_DEVELOPMENT.md)
