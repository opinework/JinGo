# JinGo VPN - Troubleshooting Guide

[中文文档](05_TROUBLESHOOTING_zh.md)

## Build Issues

### Qt Not Found

**Error:**
```
CMake Error: Could not find Qt6
```

**Solution:**

Ensure Qt path is correct in build scripts:
```bash
# macOS
QT_MACOS_PATH="/path/to/Qt/6.10.0/macos"

# Linux
QT_DIR="/path/to/Qt/6.10.0/gcc_64"

# iOS
QT_IOS_PATH="/path/to/Qt/6.10.0/ios"
```

### Android NDK Version Mismatch

**Error:**
```
CMake Error: Android NDK not found or version mismatch
```

**Solution:**

Edit NDK version in `build-android.sh`:
```bash
ANDROID_NDK_VERSION="27.2.12479018"
```

## Runtime Issues

### VPN Connection Failed

**Possible causes:**

1. **Server unreachable** - Check network connection
2. **Insufficient permissions** - See platform-specific solutions below
3. **Configuration error** - Check server configuration

### Linux: TUN Device Creation Failed

**Error:**
```
Failed to create TUN device: Operation not permitted
```

**Solution:**
```bash
# Method 1: Set capability
sudo setcap cap_net_admin+eip ./JinGo

# Method 2: Run as root
sudo ./JinGo
```

### macOS: Requires Root Permission

macOS TUN device requires administrator privileges:
```bash
sudo open build-macos/bin/Debug/JinGo.app
```

### Windows: WinTun Driver Installation Failed

Run application as Administrator, driver will install automatically.

### Android: VPN Permission Denied

1. Go to System Settings → Apps → JinGo
2. Clear app data
3. Reopen app and grant VPN permission

## Network Issues

### API Request Failed

**Troubleshooting steps:**

1. Check network connection
2. Verify API server address
3. Enable verbose logging:
```bash
QT_LOGGING_RULES="*.debug=true" ./JinGo
```

### SSL Certificate Error

1. Check if system time is correct
2. Update system CA certificates

## UI Issues

### QML Loading Failed

**Solution:**

Check if Qt QML modules are fully installed.

### High DPI Display Issues

```bash
# Set scale factor
export QT_SCALE_FACTOR=1.5

# Or auto detect
export QT_AUTO_SCREEN_SCALE_FACTOR=1
```

### Linux Wayland Display Issues

```bash
# Force X11
QT_QPA_PLATFORM=xcb ./JinGo
```

## Data Issues

### Clear Application Data

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

## Logging and Debugging

### Enable Verbose Logging

```bash
# All debug logs
QT_LOGGING_RULES="*.debug=true" ./JinGo

# Android
adb logcat -s JinGo:V
```

### Log File Locations

| Platform | Location |
|----------|----------|
| Linux | `~/.local/share/JinGo/logs/` |
| macOS | `~/Library/Logs/JinGo/` |
| Windows | `%APPDATA%\JinGo\logs\` |

## Platform-Specific Quick Reference

### Android

| Issue | Solution |
|-------|----------|
| Background killed | Add to battery optimization whitelist |
| Notifications not showing | Check notification permission |

### iOS

| Issue | Solution |
|-------|----------|
| VPN configuration failed | Check Network Extension permission |
| Certificate error | Re-sign application |

### macOS

| Issue | Solution |
|-------|----------|
| Permission denied | Run with sudo |

### Windows

| Issue | Solution |
|-------|----------|
| DLL missing | Re-run windeployqt |
| Driver installation failed | Run as Administrator |

### Linux

| Issue | Solution |
|-------|----------|
| Permission denied | Set CAP_NET_ADMIN or use sudo |
| Library not found | Set LD_LIBRARY_PATH |

## Related Documentation

- [Build Guide](02_BUILD_GUIDE.md)
- [Development Guide](03_DEVELOPMENT.md)
