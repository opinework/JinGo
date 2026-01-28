# JinGo VPN - White-labeling Guide

[中文文档](04_WHITE_LABELING_zh.md)

## Overview

JinGo VPN supports white-labeling, allowing creation of independently branded VPN applications:

- Brand name and identity
- Application icons
- Color themes
- API server configuration

## White-label Directory Structure

```
white-labeling/
├── 1/                         # Brand 1 (default)
│   ├── bundle_config.json     # Brand configuration
│   └── icons/                 # Icon resources
│       ├── app.png            # Main icon (1024x1024)
│       ├── app.icns           # macOS
│       ├── app.ico            # Windows
│       ├── ios/               # iOS sizes
│       └── android/           # Android densities
├── 2/                         # Brand 2
│   └── ...
└── 3/                         # Brand 3
    └── ...
```

## Brand Configuration File

### bundle_config.json

```json
{
  "brand_id": "mybrand",
  "brand_name": "MyBrand VPN",
  "api_base_url": "https://api.mybrand.com",

  "app_store": {
    "android_package": "com.mybrand.vpn",
    "ios_bundle_id": "com.mybrand.vpn",
    "macos_bundle_id": "com.mybrand.vpn.macos"
  },

  "theme": {
    "primary_color": "#007AFF",
    "accent_color": "#34C759"
  },

  "legal": {
    "privacy_policy_url": "https://mybrand.com/privacy",
    "terms_of_service_url": "https://mybrand.com/terms"
  }
}
```

## Creating a New Brand

### 1. Copy Template

```bash
cd white-labeling
cp -r 1 4  # Create brand 4
```

### 2. Modify Configuration

Edit `4/bundle_config.json` and update brand information.

### 3. Replace Icons

Place brand icons in `4/icons/` directory:

| File | Purpose |
|------|---------|
| `app.png` | Source icon (1024x1024) |
| `app.icns` | macOS |
| `app.ico` | Windows |
| `ios/` | iOS sizes |
| `android/mipmap-*/` | Android densities |

## Building White-label Applications

```bash
# macOS
./scripts/build/build-macos.sh --brand 4

# iOS
./scripts/build/build-ios.sh --brand 4 --team-id YOUR_TEAM_ID

# Android
./scripts/build/build-android.sh --brand 4 --abi arm64-v8a

# Linux
./scripts/build/build-linux.sh --brand 4
```

## Theme Customization

### Using Brand Colors in QML

```qml
Rectangle {
    color: bundleConfig.primaryColor
}

Button {
    background: Rectangle {
        color: bundleConfig.accentColor
    }
}
```

## Common Issues

### Icons Not Displaying Correctly

1. Check if icon sizes are complete
2. Clean build cache: `--clean`
3. Uninstall old version before installing

### Theme Colors Not Applied

1. Check `bundle_config.json` format
2. Ensure color values are correct (#RRGGBB)
3. Rebuild application

## Related Documentation

- [Build Guide](02_BUILD_GUIDE.md)
- [Troubleshooting](05_TROUBLESHOOTING.md)
