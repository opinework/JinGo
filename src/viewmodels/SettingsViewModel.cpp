/**
 * @file SettingsViewModel.cpp
 * @brief 设置视图模型实现文件
 * @details 实现应用程序设置的管理、加载、保存和重置功能
 * @author JinGo VPN Team
 * @date 2025
 */

#include "SettingsViewModel.h"
#include "core/ConfigManager.h"
#include "core/Logger.h"
#include "platform/PlatformInterface.h"
#include <QSettings>
#include <QNetworkInterface>

/**
 * @brief 构造函数
 * @param parent 父对象
 *
 * @details 初始化所有设置为默认值，获取ConfigManager和PlatformInterface实例，
 * 然后从配置文件加载实际设置值
 */
SettingsViewModel::SettingsViewModel(QObject* parent)
    : QObject(parent)
    , m_autoConnect(false)
    , m_autoStart(false)
    , m_systemProxy(false)
    , m_language("zh_CN")
    , m_theme("light")
    , m_themeName("JinGO")
    , m_allowLAN(false)
    , m_logLevel(1)
    , m_latencyTestMethod(0)
    , m_selectedNetworkInterface("auto")
    , m_configManager(&ConfigManager::instance())
    , m_platformInterface(PlatformInterface::create(this))
{
    loadSettings();
    refreshNetworkInterfaces();
}

/**
 * @brief 设置是否自动连接
 * @param enabled true表示启用，false表示禁用
 *
 * @details 如果值发生变化，更新成员变量，发出信号并自动保存到配置文件
 */
void SettingsViewModel::setAutoConnect(bool enabled)
{
    if (m_autoConnect != enabled) {
        m_autoConnect = enabled;
        emit autoConnectChanged();
        saveSettings();
    } else {
    }
}

/**
 * @brief 设置是否开机自启动
 * @param enabled true表示启用，false表示禁用
 *
 * @details 如果值发生变化：
 * 1. 更新成员变量并发出信号
 * 2. 调用平台接口设置系统开机启动
 * 3. 保存到配置文件
 */
void SettingsViewModel::setAutoStart(bool enabled)
{
    if (m_autoStart != enabled) {
        m_autoStart = enabled;
        emit autoStartChanged();

        // 应用系统设置
        if (m_platformInterface) {
            m_platformInterface->setAutoStart(enabled);
        }

        saveSettings();
    }
}

/**
 * @brief 设置是否启用系统代理
 * @param enabled true表示启用，false表示禁用
 *
 * @details 如果值发生变化，更新成员变量，发出信号并自动保存到配置文件
 */
void SettingsViewModel::setSystemProxy(bool enabled)
{
    if (m_systemProxy != enabled) {
        m_systemProxy = enabled;
        emit systemProxyChanged();
        saveSettings();
    }
}

/**
 * @brief 设置界面语言
 * @param language 语言代码（如"zh_CN"、"en_US"）
 *
 * @details 如果值发生变化，更新成员变量，发出信号并自动保存到配置文件
 */
void SettingsViewModel::setLanguage(const QString& language)
{
    if (m_language != language) {
        m_language = language;
        emit languageChanged();
        saveSettings();
    }
}

/**
 * @brief 设置主题
 * @param theme 主题字符串："light"、"dark"或"auto"
 *
 * @details 如果值发生变化，更新成员变量，发出信号并自动保存到配置文件
 * - themeChanged信号会触发isDarkMode和isAutoTheme的更新
 */
void SettingsViewModel::setTheme(const QString& theme)
{
    if (m_theme != theme) {
        m_theme = theme;
        emit themeChanged();
        saveSettings();
    }
}

/**
 * @brief 设置主题名称
 * @param themeName 主题名称："JinGO"或"StarDust"
 *
 * @details 如果值发生变化，更新成员变量，发出信号并自动保存到配置文件
 */
void SettingsViewModel::setThemeName(const QString& themeName)
{
    if (m_themeName != themeName) {
        m_themeName = themeName;
        emit themeNameChanged();
        saveSettings();
    }
}

/**
 * @brief 判断是否为深色模式
 * @return true表示深色模式，false表示浅色模式
 *
 * @details 当主题设置为"dark"或"auto"时返回true
 */
bool SettingsViewModel::isDarkMode() const
{
    // 如果主题设置为 "dark" 或 "auto"，则认为是深色模式
    return m_theme.toLower() == "dark" || m_theme.toLower() == "auto";
}

/**
 * @brief 判断是否为自动主题
 * @return true表示自动跟随系统，false表示手动设置
 *
 * @details 当主题设置为"auto"时返回true
 */
bool SettingsViewModel::isAutoTheme() const
{
    // 检查是否启用了系统自动主题
    return m_theme.toLower() == "auto";
}

/**
 * @brief 设置是否允许局域网访问
 * @param enabled true表示允许，false表示禁止
 *
 * @details 如果值发生变化，更新成员变量，发出信号并自动保存到配置文件
 */
void SettingsViewModel::setAllowLAN(bool enabled)
{
    if (m_allowLAN != enabled) {
        m_allowLAN = enabled;
        emit allowLANChanged();
        saveSettings();
    }
}

/**
 * @brief 设置日志级别
 * @param level 日志级别（0=Debug, 1=Warning, 2=Error）
 *
 * @details 如果值发生变化，更新成员变量，发出信号并自动保存到配置文件
 */
void SettingsViewModel::setLogLevel(int level)
{
    if (m_logLevel != level) {
        m_logLevel = level;
        emit logLevelChanged();
        saveSettings();
    }
}

/**
 * @brief 设置延迟测试方法
 * @param method 测试方法（0=ConnectTest，1=PingTest）
 *
 * @details 如果值发生变化，更新成员变量，发出信号并自动保存到配置文件
 */
void SettingsViewModel::setLatencyTestMethod(int method)
{
    if (m_latencyTestMethod != method) {
        m_latencyTestMethod = method;
        emit latencyTestMethodChanged();
        saveSettings();
    }
}

/**
 * @brief 从ConfigManager加载设置
 *
 * @details 加载流程：
 * 1. 从ConfigManager读取所有配置项
 * 2. 更新成员变量
 * 3. 发出所有changed信号通知UI更新
 */
void SettingsViewModel::loadSettings()
{
    LOG_DEBUG("Loading settings");

    // 使用 ConfigManager 的 getSetting() 方法
    m_autoConnect = m_configManager->autoConnect();
    m_autoStart = m_configManager->autoStart();
    m_systemProxy = m_configManager->systemProxy();
    m_language = m_configManager->language();
    m_theme = m_configManager->theme();
    m_allowLAN = m_configManager->allowLAN();

    // LogLevel 是枚举，需要转换
    m_logLevel = static_cast<int>(m_configManager->logLevel());
    m_latencyTestMethod = static_cast<int>(m_configManager->latencyTestMethod());

    // 加载主题名称（暂时使用默认值，后续可集成到ConfigManager）
    // 这里可以使用 QSettings 直接读取
    QSettings settings;
    m_themeName = settings.value("ui/themeName", "JinGO").toString();

    // 加载网络接口选择
    m_selectedNetworkInterface = settings.value("network/selectedInterface", "auto").toString();

    emit autoConnectChanged();
    emit autoStartChanged();
    emit systemProxyChanged();
    emit languageChanged();
    emit themeChanged();
    emit themeNameChanged();
    emit allowLANChanged();
    emit logLevelChanged();
    emit latencyTestMethodChanged();
    emit selectedNetworkInterfaceChanged();
}

/**
 * @brief 保存设置到ConfigManager
 *
 * @details 保存流程：
 * 1. 将所有成员变量写入ConfigManager
 * 2. 调用ConfigManager的save()方法持久化
 * 3. 发出settingsSaved信号
 */
void SettingsViewModel::saveSettings()
{
    LOG_DEBUG("Saving settings");

    // 使用 ConfigManager 的专用方法
    m_configManager->setAutoConnect(m_autoConnect);
    m_configManager->setAutoStart(m_autoStart);
    m_configManager->setSystemProxy(m_systemProxy);
    m_configManager->setLanguage(m_language);
    m_configManager->setTheme(m_theme);
    m_configManager->setAllowLAN(m_allowLAN);

    // LogLevel 需要转换为枚举
    m_configManager->setLogLevel(static_cast<ConfigManager::LogLevel>(m_logLevel));
    m_configManager->setLatencyTestMethod(static_cast<ConfigManager::LatencyTestMethod>(m_latencyTestMethod));

    // 保存主题名称（暂时使用QSettings直接保存）
    QSettings settings;
    settings.setValue("ui/themeName", m_themeName);

    m_configManager->save();
    emit settingsSaved();
}

/**
 * @brief 重置所有设置为默认值
 *
 * @details 默认值：
 * - autoConnect: false
 * - autoStart: false
 * - systemProxy: true
 * - language: "zh_CN"
 * - theme: "light"
 * - allowLAN: false
 * - logLevel: Warning
 *
 * 重置后会自动保存到配置文件
 */
void SettingsViewModel::resetToDefaults()
{

    setAutoConnect(false);
    setAutoStart(false);
    setSystemProxy(true);  // 默认启用系统代理
    setLanguage("zh_CN");
    setTheme("light");
    setAllowLAN(false);
    setLogLevel(static_cast<int>(ConfigManager::Warning));  // 默认 Warning 级别
}

/**
 * @brief 清除缓存
 */
void SettingsViewModel::clearCache()
{
    // 缓存清理由 LogManager 处理
}

/**
 * @brief 导出日志
 */
void SettingsViewModel::exportLogs()
{
    // 日志导出由 LogManager 处理
}

/**
 * @brief 刷新可用网络接口列表
 *
 * @details 枚举系统中所有有效的网络接口，过滤掉：
 * - 回环接口 (loopback)
 * - VPN隧道接口 (utun)
 * - 无IP地址的接口
 */
void SettingsViewModel::refreshNetworkInterfaces()
{
    LOG_DEBUG("Refreshing network interfaces");

    QStringList interfaces;
    // 不再添加"auto"选项，直接列出物理网卡

    const auto allInterfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& iface : allInterfaces) {
        // 跳过无效、回环和非运行状态的接口
        if (!iface.isValid() ||
            (iface.flags() & QNetworkInterface::IsLoopBack) ||
            !(iface.flags() & QNetworkInterface::IsUp) ||
            !(iface.flags() & QNetworkInterface::IsRunning)) {
            continue;
        }

        QString ifaceName = iface.humanReadableName();

        // 跳过 VPN 隧道接口
        if (ifaceName.startsWith("utun") ||
            ifaceName.startsWith("tun") ||
            ifaceName.startsWith("tap") ||
            ifaceName.startsWith("bridge") ||
            ifaceName.startsWith("awdl") ||
            ifaceName.startsWith("llw")) {
            continue;
        }

        // 获取 IPv4 地址
        const auto addresses = iface.addressEntries();
        for (const QNetworkAddressEntry& entry : addresses) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                QString ip = entry.ip().toString();
                // 跳过 VPN 内部地址
                if (ip.startsWith("172.19.") || ip.startsWith("127.")) {
                    continue;
                }
                // 格式: "接口名 (IP地址)"
                QString displayName = QString("%1 (%2)").arg(ifaceName, ip);
                interfaces.append(displayName);
                LOG_DEBUG(QString("Found network interface: ") + displayName);
                break;  // 每个接口只取一个 IPv4 地址
            }
        }
    }

    bool interfacesChanged = (m_availableNetworkInterfaces != interfaces);
    if (interfacesChanged) {
        m_availableNetworkInterfaces = interfaces;
        emit availableNetworkInterfacesChanged();
        LOG_INFO(QString("Network interfaces refreshed, count: %1").arg(interfaces.size()));
    }

    // 检查当前选择的接口是否仍然可用
    bool currentSelectionValid = false;
    if (!m_selectedNetworkInterface.isEmpty() && m_selectedNetworkInterface != "auto") {
        currentSelectionValid = interfaces.contains(m_selectedNetworkInterface);
    }

    // 如果当前选择无效或为空，自动选择第一个物理网卡
    if (!currentSelectionValid && !interfaces.isEmpty()) {
        QString newSelection = interfaces.first();
        LOG_INFO(QString("Auto-selecting first physical interface: %1").arg(newSelection));
        setSelectedNetworkInterface(newSelection);
    } else if (interfaces.isEmpty()) {
        // 没有可用网卡时清空选择
        if (!m_selectedNetworkInterface.isEmpty()) {
            m_selectedNetworkInterface.clear();
            emit selectedNetworkInterfaceChanged();
            LOG_WARNING("No network interfaces available, cleared selection");
        }
    }
}

/**
 * @brief 设置选中的网络接口
 * @param interface 网络接口显示名称，"auto"表示自动选择
 *
 * @details 解析IP地址并保存到配置文件和SharedDefaults（供Extension读取）
 */
void SettingsViewModel::setSelectedNetworkInterface(const QString& interfaceName)
{
    if (m_selectedNetworkInterface != interfaceName) {
        m_selectedNetworkInterface = interfaceName;
        emit selectedNetworkInterfaceChanged();

        // 从显示名称中提取IP地址
        QString ipAddress = "auto";
        if (interfaceName != "auto") {
            // 格式: "接口名 (IP地址)"
            qsizetype start = interfaceName.lastIndexOf('(');
            qsizetype end = interfaceName.lastIndexOf(')');
            if (start != -1 && end != -1 && end > start) {
                ipAddress = interfaceName.mid(start + 1, end - start - 1);
            }
        }

        LOG_INFO(QString("Selected network interface: %1 (IP: %2)").arg(interfaceName, ipAddress));

        // 保存到 QSettings
        QSettings settings;
        settings.setValue("network/selectedInterface", interfaceName);
        settings.setValue("network/selectedInterfaceIP", ipAddress);

        // 同步到 SharedDefaults 供 Network Extension 读取
        // 通过 PlatformInterface 来保存，避免在 .cpp 文件中使用 Objective-C
        if (m_platformInterface) {
            m_platformInterface->saveToSharedDefaults("network/preferredInterfaceIP", ipAddress);
            LOG_INFO(QString("Saved preferredInterfaceIP to SharedDefaults: %1").arg(ipAddress));
        }
    }
}
