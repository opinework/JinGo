# JinGo VPN - Development Guide

[中文文档](03_DEVELOPMENT_zh.md)

## Development Environment

### Recommended IDEs

- **Qt Creator** (Recommended) - Best Qt/QML development experience
- **VS Code** + Qt plugin - Lightweight option
- **CLion** - Good for C++ development

### Qt Creator Configuration

1. Open project: `File → Open File or Project → CMakeLists.txt`
2. Select Kit:
   - Android: Android Qt 6.10 Clang arm64-v8a
   - macOS: Desktop Qt 6.10 clang 64bit
   - iOS: iOS Qt 6.10
   - Linux: Desktop Qt 6.10 GCC 64bit
   - Windows: Desktop Qt 6.10 MinGW 64-bit

## Project Structure

```
JinGo/
├── src/
│   ├── main.cpp              # Application entry
│   ├── viewmodels/           # MVVM view models
│   │   ├── ConnectionViewModel.cpp/h   # Connection state management
│   │   ├── LoginViewModel.cpp/h        # Login logic
│   │   ├── RegisterViewModel.cpp/h     # Registration logic
│   │   ├── ServerListViewModel.cpp/h   # Server list
│   │   └── SettingsViewModel.cpp/h     # Settings management
│   ├── models/
│   │   └── SubscriptionListModel.cpp/h # QML list model
│   └── ui/
│       └── SystemTrayManager.cpp/h     # System tray
├── resources/
│   ├── qml/
│   │   ├── Main.qml                    # Main interface
│   │   ├── pages/
│   │   │   ├── ConnectionPage.qml      # Connection page
│   │   │   ├── ServerListPage.qml      # Server list
│   │   │   ├── SubscriptionPage.qml    # Subscription management
│   │   │   ├── SettingsPage.qml        # Settings page
│   │   │   └── ProfilePage.qml         # User profile
│   │   ├── components/                 # Common components
│   │   └── dialogs/                    # Dialogs
│   └── translations/                   # Translation files
└── third_party/                        # Pre-compiled dependencies
```

## QML Development

### QML and C++ Interaction

Core classes are registered to QML context in `main.cpp`:

```cpp
// main.cpp
QQmlContext* rootContext = engine.rootContext();
rootContext->setContextProperty("vpnManager", &VPNManager::instance());
rootContext->setContextProperty("authManager", &AuthManager::instance());
rootContext->setContextProperty("configManager", &ConfigManager::instance());
rootContext->setContextProperty("subscriptionManager", &SubscriptionManager::instance());
```

### Using in QML

```qml
// ConnectionPage.qml
import QtQuick

Item {
    Connections {
        target: vpnManager

        function onConnectionStateChanged() {
            console.log("State changed:", vpnManager.connectionState)
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

### Common QML Properties

```qml
// VPNManager
vpnManager.isConnected           // bool: Connected status
vpnManager.connectionState       // enum: Connection state
vpnManager.currentServer         // Server: Current server
vpnManager.uploadSpeed           // qint64: Upload speed (bytes/s)
vpnManager.downloadSpeed         // qint64: Download speed (bytes/s)
vpnManager.currentDelay          // int: Current latency (ms)

// ConfigManager
configManager.vpnMode            // enum: VPN mode (TUN/Proxy)
configManager.routingMode        // enum: Routing mode (Global/Rule/Subscription)
configManager.localSocksPort     // int: Local SOCKS port
configManager.localHttpPort      // int: Local HTTP port

// AuthManager
authManager.isAuthenticated      // bool: Login status
authManager.currentUser          // User: Current user

// SubscriptionManager
subscriptionManager.subscriptions    // list: Subscription list
subscriptionManager.isUpdating       // bool: Is updating
```

## Internationalization

### Adding New Translations

1. Wrap text with `qsTr()` in QML:
```qml
Text {
    text: qsTr("Connect")
}
```

2. Update translation files:
```bash
# Use build script to update translations
./scripts/build/build-linux.sh --translate
```

3. Edit `resources/translations/jingo_*.ts` with Qt Linguist

4. Compile translations:
```bash
lrelease resources/translations/*.ts
```

### Supported Languages

| File | Language |
|------|----------|
| `jingo_en_US.ts` | English |
| `jingo_zh_CN.ts` | Simplified Chinese |
| `jingo_zh_TW.ts` | Traditional Chinese |
| `jingo_vi_VN.ts` | Vietnamese |
| `jingo_km_KH.ts` | Khmer |
| `jingo_my_MM.ts` | Burmese |
| `jingo_ru_RU.ts` | Russian |
| `jingo_fa_IR.ts` | Persian |

## Debugging

### Enable Verbose Logging

```bash
# Linux/macOS
QT_LOGGING_RULES="*.debug=true" ./JinGo

# Or specify module
QT_LOGGING_RULES="jingo.*.debug=true" ./JinGo
```

### Log File Locations

| Platform | Location |
|----------|----------|
| Linux | `~/.local/share/JinGo/logs/` |
| macOS | `~/Library/Application Support/Opine Work/JinGo/logs/` |
| Windows | `%APPDATA%\Opine Work\JinGo\logs\` |

### QML Debugging

```qml
// Output debug information
console.log("Variable value:", someVariable)
console.warn("Warning message")
console.error("Error message")

// Output object properties
console.log(JSON.stringify(someObject, null, 2))
```

### Android Remote Debugging

```bash
# View logs
adb logcat -s JinGo:V SuperRay-JNI:V Qt:V

# Filter specific tags
adb logcat | grep -E "JinGo|SuperRay|VPN"

# Deploy and run
./scripts/build/build-android.sh --debug --install
```

## Code Style

### C++ Style

```cpp
// Class name: PascalCase
class VPNManager {
public:
    // Method name: camelCase
    void connectToServer(const Server& server);

    // Constants: UPPER_CASE
    static const int MAX_RETRY_COUNT = 3;

private:
    // Member variables: m_ prefix
    QString m_serverAddress;
    int m_retryCount;
};
```

### QML Style

```qml
// Filename: PascalCase.qml
Item {
    id: root

    // Properties first
    property string title: ""
    property bool isActive: false

    // Signal declarations
    signal clicked()
    signal valueChanged(int newValue)

    // Child components
    Rectangle {
        id: background
        anchors.fill: parent
    }

    // Functions last
    function doSomething() {
        // ...
    }
}
```

## Common Development Tasks

### Adding a New Page

1. Create QML file: `resources/qml/pages/NewPage.qml`
2. Add file to `resources/qml/CMakeLists.txt` or `qml.qrc`
3. Add navigation entry in `Main.qml`

### Adding a New Setting

1. Add property in `ConfigManager` (JinDo)
2. Add UI control in `SettingsPage.qml`
3. Bind to configManager property

### Adding a New Translation Key

1. Use `qsTr("New Text")` in QML
2. Run `lupdate` to update .ts files
3. Translate with Qt Linguist
4. Run `lrelease` to compile

## Release Checklist

- [ ] Update version number (`PROJECT_VERSION` in `CMakeLists.txt`)
- [ ] Update translation files
- [ ] Compile in Release mode
- [ ] Test all platforms
- [ ] Check logs for sensitive information
- [ ] Update CHANGELOG

## Related Documentation

- [Architecture](01_ARCHITECTURE.md)
- [Build Guide](02_BUILD_GUIDE.md)
- [Troubleshooting](05_TROUBLESHOOTING.md)
- [Panel Extension](07_PANEL_EXTENSION.md)
