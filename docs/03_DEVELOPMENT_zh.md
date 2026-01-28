# JinGo VPN - 开发指南

## 开发环境

### 推荐 IDE

- **Qt Creator** (推荐) - 提供最佳的 Qt/QML 开发体验
- **VS Code** + Qt 插件 - 轻量级选择
- **CLion** - 适合 C++ 开发

### Qt Creator 配置

1. 打开项目: `File → Open File or Project → CMakeLists.txt`
2. 选择 Kit:
   - Android: Android Qt 6.8 Clang arm64-v8a
   - macOS: Desktop Qt 6.8 clang 64bit
   - iOS: iOS Qt 6.8
   - Linux: Desktop Qt 6.8 GCC 64bit
   - Windows: Desktop Qt 6.8 MinGW 64-bit

## 项目结构

```
JinGo/
├── src/
│   ├── main.cpp              # 应用入口
│   ├── viewmodels/           # MVVM 视图模型
│   │   ├── ConnectionViewModel.cpp/h   # 连接状态管理
│   │   ├── LoginViewModel.cpp/h        # 登录逻辑
│   │   ├── RegisterViewModel.cpp/h     # 注册逻辑
│   │   ├── ServerListViewModel.cpp/h   # 服务器列表
│   │   └── SettingsViewModel.cpp/h     # 设置管理
│   ├── models/
│   │   └── SubscriptionListModel.cpp/h # QML 列表模型
│   └── ui/
│       └── SystemTrayManager.cpp/h     # 系统托盘
├── resources/
│   ├── qml/
│   │   ├── Main.qml                    # 主界面
│   │   ├── pages/
│   │   │   ├── ConnectionPage.qml      # 连接页面
│   │   │   ├── ServerListPage.qml      # 服务器列表
│   │   │   ├── SubscriptionPage.qml    # 订阅管理
│   │   │   ├── SettingsPage.qml        # 设置页面
│   │   │   └── ProfilePage.qml         # 个人中心
│   │   ├── components/                 # 通用组件
│   │   └── dialogs/                    # 对话框
│   └── translations/                   # 翻译文件
└── third_party/                        # 预编译依赖库
```

## QML 开发

### QML 与 C++ 交互

核心类通过 `main.cpp` 注册到 QML 上下文：

```cpp
// main.cpp
QQmlContext* rootContext = engine.rootContext();
rootContext->setContextProperty("vpnManager", &VPNManager::instance());
rootContext->setContextProperty("authManager", &AuthManager::instance());
rootContext->setContextProperty("configManager", &ConfigManager::instance());
rootContext->setContextProperty("subscriptionManager", &SubscriptionManager::instance());
```

### 在 QML 中使用

```qml
// ConnectionPage.qml
import QtQuick

Item {
    Connections {
        target: vpnManager

        function onConnectionStateChanged() {
            console.log("状态变化:", vpnManager.connectionState)
        }
    }

    Button {
        text: vpnManager.isConnected ? qsTr("Disconnect") : qsTr("Connect")
        onClicked: {
            if (vpnManager.isConnected) {
                vpnManager.disconnect()
            } else {
                vpnManager.connect()
            }
        }
    }
}
```

### 常用 QML 属性

```qml
// VPNManager
vpnManager.isConnected           // bool: 是否已连接
vpnManager.connectionState       // enum: 连接状态
vpnManager.currentServer         // Server: 当前服务器
vpnManager.uploadSpeed           // qint64: 上传速度 (bytes/s)
vpnManager.downloadSpeed         // qint64: 下载速度 (bytes/s)
vpnManager.currentDelay          // int: 当前延迟 (ms)

// ConfigManager
configManager.vpnMode            // enum: VPN 模式 (TUN/Proxy)
configManager.routingMode        // enum: 路由模式 (Global/Rule/Subscription)
configManager.localSocksPort     // int: 本地 SOCKS 端口
configManager.localHttpPort      // int: 本地 HTTP 端口

// AuthManager
authManager.isAuthenticated      // bool: 是否已登录
authManager.currentUser          // User: 当前用户

// SubscriptionManager
subscriptionManager.subscriptions    // list: 订阅列表
subscriptionManager.isUpdating       // bool: 是否正在更新
```

## 多语言支持

### 添加新翻译

1. 在 QML 中使用 `qsTr()` 包裹文本：
```qml
Text {
    text: qsTr("Connect")
}
```

2. 更新翻译文件：
```bash
# 使用构建脚本更新翻译
./scripts/build/build-linux.sh --translate
```

3. 使用 Qt Linguist 编辑 `resources/translations/jingo_*.ts`

4. 编译翻译：
```bash
lrelease resources/translations/*.ts
```

### 支持的语言

| 文件 | 语言 |
|------|------|
| `jingo_en_US.ts` | English |
| `jingo_zh_CN.ts` | 简体中文 |
| `jingo_zh_TW.ts` | 繁體中文 |
| `jingo_vi_VN.ts` | Tiếng Việt |
| `jingo_km_KH.ts` | ភាសាខ្មែរ |
| `jingo_my_MM.ts` | မြန်မာဘာသာ |
| `jingo_ru_RU.ts` | Русский |
| `jingo_fa_IR.ts` | فارسی |

## 调试

### 启用详细日志

```bash
# Linux/macOS
QT_LOGGING_RULES="*.debug=true" ./JinGo

# 或指定模块
QT_LOGGING_RULES="jingo.*.debug=true" ./JinGo
```

### 日志文件位置

| 平台 | 位置 |
|------|------|
| Linux | `~/.local/share/JinGo/logs/` |
| macOS | `~/Library/Application Support/Opine Work/JinGo/logs/` |
| Windows | `%APPDATA%\Opine Work\JinGo\logs\` |

### QML 调试

```qml
// 输出调试信息
console.log("变量值:", someVariable)
console.warn("警告信息")
console.error("错误信息")

// 输出对象属性
console.log(JSON.stringify(someObject, null, 2))
```

### Android 远程调试

```bash
# 查看日志
adb logcat -s JinGo:V SuperRay-JNI:V Qt:V

# 筛选特定标签
adb logcat | grep -E "JinGo|SuperRay|VPN"

# 部署并运行
./scripts/build/build-android.sh --debug --install
```

## 代码风格

### C++ 风格

```cpp
// 类名: PascalCase
class VPNManager {
public:
    // 方法名: camelCase
    void connectToServer(const Server& server);

    // 常量: UPPER_CASE
    static const int MAX_RETRY_COUNT = 3;

private:
    // 成员变量: m_ 前缀
    QString m_serverAddress;
    int m_retryCount;
};
```

### QML 风格

```qml
// 文件名: PascalCase.qml
Item {
    id: root

    // 属性声明在前
    property string title: ""
    property bool isActive: false

    // 信号声明
    signal clicked()
    signal valueChanged(int newValue)

    // 子组件
    Rectangle {
        id: background
        anchors.fill: parent
    }

    // 函数在后
    function doSomething() {
        // ...
    }
}
```

## 常见开发任务

### 添加新页面

1. 创建 QML 文件: `resources/qml/pages/NewPage.qml`
2. 在 `resources/qml/CMakeLists.txt` 或 `qml.qrc` 添加文件
3. 在 `Main.qml` 添加导航入口

### 添加新设置项

1. 在 `ConfigManager` 中添加属性 (JinDo)
2. 在 `SettingsPage.qml` 添加 UI 控件
3. 绑定到 configManager 属性

### 添加新翻译键

1. 在 QML 中使用 `qsTr("New Text")`
2. 运行 `lupdate` 更新 .ts 文件
3. 使用 Qt Linguist 翻译
4. 运行 `lrelease` 编译

## 发布检查清单

- [ ] 更新版本号 (`CMakeLists.txt` 中的 `PROJECT_VERSION`)
- [ ] 更新翻译文件
- [ ] Release 模式编译
- [ ] 测试所有平台
- [ ] 检查日志无敏感信息
- [ ] 更新 CHANGELOG

## 相关文档

- [架构说明](01_ARCHITECTURE.md)
- [构建指南](02_BUILD_GUIDE.md)
- [故障排除](05_TROUBLESHOOTING.md)
- [面板扩展](07_PANEL_EXTENSION.md)
