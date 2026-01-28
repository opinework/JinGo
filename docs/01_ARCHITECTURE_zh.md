# JinGo VPN - 架构说明

## 概述

JinGo VPN 是一个跨平台 VPN 客户端，采用 Qt 6 + QML 构建现代化用户界面，基于 JinDoCore 核心库实现 VPN 功能。

## 系统架构

```
┌─────────────────────────────────────────────────────────────┐
│                        JinGo 应用                           │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────────────────────────────────────────────┐  │
│  │                    QML 界面层                         │  │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐    │  │
│  │  │ 连接页面 │ │ 服务器  │ │ 订阅    │ │ 设置    │    │  │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘    │  │
│  └──────────────────────────────────────────────────────┘  │
│                            │                                │
│                            ▼                                │
│  ┌──────────────────────────────────────────────────────┐  │
│  │                 JinDoCore 静态库                      │  │
│  │  ┌────────────┐ ┌────────────┐ ┌────────────┐       │  │
│  │  │ VPNManager │ │ AuthManager│ │ ConfigMgr  │       │  │
│  │  │ 连接管理   │ │ 认证管理   │ │ 配置管理   │       │  │
│  │  └────────────┘ └────────────┘ └────────────┘       │  │
│  │  ┌────────────┐ ┌────────────┐ ┌────────────┐       │  │
│  │  │ PanelMgr   │ │ SubsMgr    │ │ DatabaseMgr│       │  │
│  │  │ 面板管理   │ │ 订阅管理   │ │ 数据存储   │       │  │
│  │  └────────────┘ └────────────┘ └────────────┘       │  │
│  └──────────────────────────────────────────────────────┘  │
│                            │                                │
│                            ▼                                │
│  ┌──────────────────────────────────────────────────────┐  │
│  │                    平台适配层                         │  │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐    │  │
│  │  │ Android │ │  iOS    │ │ macOS   │ │ Windows │    │  │
│  │  │  Linux  │ │         │ │         │ │         │    │  │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘    │  │
│  └──────────────────────────────────────────────────────┘  │
│                            │                                │
│                            ▼                                │
│  ┌──────────────────────────────────────────────────────┐  │
│  │                 SuperRay (VPN 引擎)                   │  │
│  │     TUN 设备管理、Xray 核心、流量转发、DNS 管理       │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## 目录结构

```
JinGo/
├── src/
│   ├── main.cpp              # 应用入口
│   ├── viewmodels/           # MVVM 视图模型
│   │   ├── ConnectionViewModel.cpp/h
│   │   ├── LoginViewModel.cpp/h
│   │   ├── ServerListViewModel.cpp/h
│   │   └── SettingsViewModel.cpp/h
│   ├── models/               # QML 数据模型
│   │   └── SubscriptionListModel.cpp/h
│   ├── ui/                   # UI 组件
│   │   └── SystemTrayManager.cpp/h
│   └── panel/                # 面板扩展 (可选)
│       └── V2BoardProvider.cpp/h
├── resources/
│   ├── qml/                  # QML 界面
│   │   ├── Main.qml          # 主界面
│   │   ├── pages/            # 页面组件
│   │   ├── components/       # 通用组件
│   │   └── dialogs/          # 对话框
│   ├── icons/                # 图标资源
│   ├── translations/         # 翻译文件 (*.ts/*.qm)
│   └── geoip/                # GeoIP 数据
├── platform/
│   ├── android/              # Android 配置
│   ├── ios/                  # iOS 配置
│   ├── macos/                # macOS 配置
│   └── windows/              # Windows 配置
├── third_party/
│   ├── jindo/                # JinDoCore 静态库
│   ├── superray/             # VPN 引擎
│   └── *_openssl/            # OpenSSL 库
├── scripts/build/            # 构建脚本
└── white-labeling/           # 白标配置
```

## 核心组件

### JinDoCore (核心库)

JinDoCore 是预编译的静态库，包含所有核心业务逻辑：

| 模块 | 功能 |
|------|------|
| **VPNManager** | VPN 连接管理、状态控制 |
| **ConfigManager** | Xray 配置生成、路由规则 |
| **AuthManager** | 用户认证、Token 管理 |
| **SubscriptionManager** | 订阅解析、服务器管理 |
| **PanelManager** | 多面板支持 (XBoard/V2Board) |
| **DatabaseManager** | SQLite 数据持久化 |

### 路由模式

JinGo 支持四种路由模式：

| 模式 | 说明 |
|------|------|
| **全局 (Global)** | 所有流量走代理 |
| **规则 (Rule)** | 根据 GeoIP 分流，国内直连 |
| **直连 (Direct)** | 所有流量直连 |
| **订阅 (Subscription)** | 使用订阅中的路由规则 |

### 流量嗅探 (Sniffing)

启用 sniffing 可以从 TLS 握手中识别真实域名，提升路由准确性：

```json
{
  "sniffing": {
    "enabled": true,
    "destOverride": ["http", "tls", "quic"],
    "metadataOnly": false
  }
}
```

## 平台适配

### Android
- 使用 `VpnService` 创建 VPN
- 通过 JNI 调用 SuperRay
- Socket 保护防止流量回环

### iOS
- 使用 `NEPacketTunnelProvider` 扩展
- App Groups 共享数据
- XPC 进程间通信

### macOS
- JinGoCore 服务进程 (TUN 设备管理)
- JinGoHelper 辅助工具 (路由/DNS 配置)
- 需要管理员权限

### Windows
- WinTun 驱动
- 需要管理员权限安装驱动

### Linux
- `/dev/net/tun` 设备
- 需要 `CAP_NET_ADMIN` 权限或 root

## VPN 连接流程

```
用户点击连接
      │
      ▼
┌─────────────┐
│ QML 界面    │
└──────┬──────┘
       │ connect()
       ▼
┌─────────────┐
│ VPNManager  │ ◄── 状态: Disconnected → Connecting
└──────┬──────┘
       │ 1. 生成 Xray 配置
       │ 2. 创建 TUN 设备
       │ 3. 启动 Xray 引擎
       │ 4. 配置系统路由
       ▼
┌─────────────┐
│  SuperRay   │ ◄── 处理流量转发
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ VPNManager  │ ◄── 状态: Connecting → Connected
└─────────────┘
```

## 依赖关系

```
JinGo (Qt 应用)
    │
    ├── JinDoCore (静态库) ─── third_party/jindo/
    │   └── 核心业务逻辑、API、VPN 管理
    │
    ├── SuperRay (动态库) ──── third_party/superray/
    │   └── Xray 核心封装、TUN 管理
    │
    └── OpenSSL (静态库) ───── third_party/*_openssl/
        └── 加密支持
```

所有依赖库已预编译，位于 `third_party/` 目录。

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

## 相关文档

- [构建指南](02_BUILD_GUIDE.md)
- [开发指南](03_DEVELOPMENT.md)
- [面板扩展](07_PANEL_EXTENSION.md)
