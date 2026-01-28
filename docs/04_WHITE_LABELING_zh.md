# JinGo VPN - 白标定制指南

## 概述

JinGo VPN 支持白标定制，允许创建独立品牌的 VPN 应用：

- 品牌名称和标识
- 应用图标
- 颜色主题
- API 服务器配置

## 白标目录结构

```
white-labeling/
├── 1/                         # 品牌 1 (默认)
│   ├── bundle_config.json     # 品牌配置
│   └── icons/                 # 图标资源
│       ├── app.png            # 主图标 (1024x1024)
│       ├── app.icns           # macOS
│       ├── app.ico            # Windows
│       ├── ios/               # iOS 各尺寸
│       └── android/           # Android 各密度
├── 2/                         # 品牌 2
│   └── ...
└── 3/                         # 品牌 3
    └── ...
```

## 品牌配置文件

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

## 创建新品牌

### 1. 复制模板

```bash
cd white-labeling
cp -r 1 4  # 创建品牌 4
```

### 2. 修改配置

编辑 `4/bundle_config.json`，更新品牌信息。

### 3. 替换图标

将品牌图标放入 `4/icons/` 目录：

| 文件 | 用途 |
|------|------|
| `app.png` | 源图标 (1024x1024) |
| `app.icns` | macOS |
| `app.ico` | Windows |
| `ios/` | iOS 各尺寸 |
| `android/mipmap-*/` | Android 各密度 |

## 编译白标应用

```bash
# macOS
./scripts/build/build-macos.sh --brand 4 --skip-sign

# iOS
./scripts/build/build-ios.sh --brand 4

# Android
./scripts/build/build-android.sh --brand 4 --abi arm64-v8a

# Linux
./scripts/build/build-linux.sh --brand 4
```

## 主题定制

### QML 中使用品牌颜色

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

## 常见问题

### 图标显示不正确

1. 检查图标尺寸是否完整
2. 清理构建缓存：`--clean`
3. 卸载旧版本再安装

### 主题颜色不生效

1. 检查 `bundle_config.json` 格式
2. 确认颜色值格式正确（#RRGGBB）
3. 重新编译应用

## 相关文档

- [构建指南](02_BUILD_GUIDE.md)
- [故障排除](05_TROUBLESHOOTING.md)
