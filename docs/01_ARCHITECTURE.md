# JinGo VPN - Architecture

[中文文档](01_ARCHITECTURE_zh.md)

## Overview

JinGo VPN is a cross-platform VPN client built with Qt 6 + QML for modern user interface, powered by JinDoCore library for VPN functionality.

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                       JinGo Application                      │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────────────────────────────────────────────┐  │
│  │                    QML UI Layer                       │  │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌─────────┐  │  │
│  │  │Connection│ │ Servers  │ │Subscript.│ │Settings │  │  │
│  │  └──────────┘ └──────────┘ └──────────┘ └─────────┘  │  │
│  └──────────────────────────────────────────────────────┘  │
│                            │                                │
│                            ▼                                │
│  ┌──────────────────────────────────────────────────────┐  │
│  │                 JinDoCore Static Library              │  │
│  │  ┌────────────┐ ┌────────────┐ ┌────────────┐       │  │
│  │  │ VPNManager │ │ AuthManager│ │ ConfigMgr  │       │  │
│  │  └────────────┘ └────────────┘ └────────────┘       │  │
│  │  ┌────────────┐ ┌────────────┐ ┌────────────┐       │  │
│  │  │ PanelMgr   │ │ SubsMgr    │ │ DatabaseMgr│       │  │
│  │  └────────────┘ └────────────┘ └────────────┘       │  │
│  └──────────────────────────────────────────────────────┘  │
│                            │                                │
│                            ▼                                │
│  ┌──────────────────────────────────────────────────────┐  │
│  │                 Platform Adaptation Layer             │  │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐    │  │
│  │  │ Android │ │  iOS    │ │ macOS   │ │ Windows │    │  │
│  │  │  Linux  │ │         │ │         │ │         │    │  │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘    │  │
│  └──────────────────────────────────────────────────────┘  │
│                            │                                │
│                            ▼                                │
│  ┌──────────────────────────────────────────────────────┐  │
│  │                 SuperRay (VPN Engine)                 │  │
│  │     TUN Device, Xray Core, Traffic Routing, DNS       │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## Directory Structure

```
JinGo/
├── src/
│   ├── main.cpp              # Application entry
│   ├── viewmodels/           # MVVM view models
│   │   ├── ConnectionViewModel.cpp/h
│   │   ├── LoginViewModel.cpp/h
│   │   ├── ServerListViewModel.cpp/h
│   │   └── SettingsViewModel.cpp/h
│   ├── models/               # QML data models
│   │   └── SubscriptionListModel.cpp/h
│   ├── ui/                   # UI components
│   │   └── SystemTrayManager.cpp/h
│   └── panel/                # Panel extensions (optional)
│       └── V2BoardProvider.cpp/h
├── resources/
│   ├── qml/                  # QML UI
│   │   ├── Main.qml          # Main interface
│   │   ├── pages/            # Page components
│   │   ├── components/       # Common components
│   │   └── dialogs/          # Dialogs
│   ├── icons/                # Icon resources
│   ├── translations/         # Translation files (*.ts/*.qm)
│   └── geoip/                # GeoIP data
├── platform/
│   ├── android/              # Android config
│   ├── ios/                  # iOS config
│   ├── macos/                # macOS config
│   └── windows/              # Windows config
├── third_party/
│   ├── jindo/                # JinDoCore static library
│   ├── superray/             # VPN engine
│   └── *_openssl/            # OpenSSL libraries
├── scripts/build/            # Build scripts
└── white-labeling/           # White-label configs
```

## Core Components

### JinDoCore (Core Library)

JinDoCore is a pre-compiled static library containing all core business logic:

| Module | Function |
|--------|----------|
| **VPNManager** | VPN connection management, state control |
| **ConfigManager** | Xray config generation, routing rules |
| **AuthManager** | User authentication, token management |
| **SubscriptionManager** | Subscription parsing, server management |
| **PanelManager** | Multi-panel support (XBoard/V2Board) |
| **DatabaseManager** | SQLite data persistence |

### Routing Modes

JinGo supports four routing modes:

| Mode | Description |
|------|-------------|
| **Global** | All traffic through proxy |
| **Rule** | GeoIP-based routing, domestic direct |
| **Direct** | All traffic direct |
| **Subscription** | Use routing rules from subscription |

### Traffic Sniffing

Enable sniffing to identify real domain names from TLS handshake for better routing accuracy:

```json
{
  "sniffing": {
    "enabled": true,
    "destOverride": ["http", "tls", "quic"],
    "metadataOnly": false
  }
}
```

## Platform Adaptation

### Android
- Uses `VpnService` to create VPN
- JNI calls to SuperRay
- Socket protection to prevent traffic loopback

### iOS
- Uses `NEPacketTunnelProvider` extension
- App Groups for data sharing
- XPC for inter-process communication

### macOS
- JinGoCore service process (TUN device management)
- JinGoHelper auxiliary tool (routing/DNS configuration)
- Requires administrator privileges

### Windows
- WinTun driver
- Requires administrator privileges for driver installation

### Linux
- `/dev/net/tun` device
- Requires `CAP_NET_ADMIN` capability or root

## VPN Connection Flow

```
User clicks Connect
      │
      ▼
┌─────────────┐
│  QML UI     │
└──────┬──────┘
       │ connect()
       ▼
┌─────────────┐
│ VPNManager  │ ◄── State: Disconnected → Connecting
└──────┬──────┘
       │ 1. Generate Xray config
       │ 2. Create TUN device
       │ 3. Start Xray engine
       │ 4. Configure system routing
       ▼
┌─────────────┐
│  SuperRay   │ ◄── Handle traffic routing
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ VPNManager  │ ◄── State: Connecting → Connected
└─────────────┘
```

## Dependencies

```
JinGo (Qt Application)
    │
    ├── JinDoCore (Static Library) ─── third_party/jindo/
    │   └── Core business logic, API, VPN management
    │
    ├── SuperRay (Dynamic Library) ──── third_party/superray/
    │   └── Xray core wrapper, TUN management
    │
    └── OpenSSL (Static Library) ───── third_party/*_openssl/
        └── Encryption support
```

All dependencies are pre-compiled and located in the `third_party/` directory.

## Language Support

| Language | Code | Status |
|----------|------|--------|
| English | en_US | ✅ |
| Simplified Chinese | zh_CN | ✅ |
| Traditional Chinese | zh_TW | ✅ |
| Vietnamese | vi_VN | ✅ |
| Khmer | km_KH | ✅ |
| Burmese | my_MM | ✅ |
| Russian | ru_RU | ✅ |
| Persian | fa_IR | ✅ |

## Related Documentation

- [Build Guide](02_BUILD_GUIDE.md)
- [Development Guide](03_DEVELOPMENT.md)
- [Panel Extension](07_PANEL_EXTENSION.md)
