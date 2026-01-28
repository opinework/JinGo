/**
 * @file SettingsViewModel.h
 * @brief 设置视图模型头文件
 * @details 管理应用程序设置，包括自动连接、系统代理、主题、语言等配置
 * @author JinGo VPN Team
 * @date 2025
 */

#ifndef SETTINGSVIEWMODEL_H
#define SETTINGSVIEWMODEL_H

#include <QObject>
#include <QString>

class ConfigManager;
class PlatformInterface;

/**
 * @class SettingsViewModel
 * @brief 设置视图模型
 * @details 提供应用程序设置的管理功能，包括：
 * - 自动连接和自动启动设置
 * - 系统代理和局域网访问控制
 * - 语言和主题切换
 * - 日志级别配置
 * - 缓存清理和日志导出
 *
 * 设置更改会自动保存到ConfigManager并持久化
 */
class SettingsViewModel : public QObject
{
    Q_OBJECT

    /// 是否自动连接VPN（可读写）
    Q_PROPERTY(bool autoConnect READ autoConnect WRITE setAutoConnect NOTIFY autoConnectChanged)

    /// 是否开机自启动（可读写）
    Q_PROPERTY(bool autoStart READ autoStart WRITE setAutoStart NOTIFY autoStartChanged)

    /// 是否启用系统代理（可读写）
    Q_PROPERTY(bool systemProxy READ systemProxy WRITE setSystemProxy NOTIFY systemProxyChanged)

    /// 界面语言（可读写）
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)

    /// 主题设置（可读写）："light"、"dark"或"auto"
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)

    /// 主题名称（可读写）："JinGO"或"StarDust"
    Q_PROPERTY(QString themeName READ themeName WRITE setThemeName NOTIFY themeNameChanged)

    /// 是否为深色模式（只读，根据theme自动判断）
    Q_PROPERTY(bool isDarkMode READ isDarkMode NOTIFY themeChanged)

    /// 是否为自动主题（只读，根据theme自动判断）
    Q_PROPERTY(bool isAutoTheme READ isAutoTheme NOTIFY themeChanged)

    /// 是否允许局域网访问（可读写）
    Q_PROPERTY(bool allowLAN READ allowLAN WRITE setAllowLAN NOTIFY allowLANChanged)

    /// 日志级别（可读写）
    Q_PROPERTY(int logLevel READ logLevel WRITE setLogLevel NOTIFY logLevelChanged)

    /// 延迟测试方法（0=ConnectTest，1=PingTest）
    Q_PROPERTY(int latencyTestMethod READ latencyTestMethod WRITE setLatencyTestMethod NOTIFY latencyTestMethodChanged)

    /// 可用网络接口列表（只读）
    Q_PROPERTY(QStringList availableNetworkInterfaces READ availableNetworkInterfaces NOTIFY availableNetworkInterfacesChanged)

    /// 选择的网络接口（可读写）
    Q_PROPERTY(QString selectedNetworkInterface READ selectedNetworkInterface WRITE setSelectedNetworkInterface NOTIFY selectedNetworkInterfaceChanged)

    /// 网络接口选择是否启用（只有多个接口时才启用）
    Q_PROPERTY(bool networkInterfaceSelectionEnabled READ networkInterfaceSelectionEnabled NOTIFY availableNetworkInterfacesChanged)

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     *
     * @details 初始化视图模型并从ConfigManager加载设置
     */
    explicit SettingsViewModel(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~SettingsViewModel() override = default;

    /**
     * @brief 获取是否自动连接
     * @return true表示启用自动连接，false表示禁用
     */
    bool autoConnect() const { return m_autoConnect; }

    /**
     * @brief 获取是否开机自启动
     * @return true表示启用开机自启动，false表示禁用
     */
    bool autoStart() const { return m_autoStart; }

    /**
     * @brief 获取是否启用系统代理
     * @return true表示启用系统代理，false表示禁用
     */
    bool systemProxy() const { return m_systemProxy; }

    /**
     * @brief 获取界面语言
     * @return 语言代码（如"zh_CN"、"en_US"）
     */
    QString language() const { return m_language; }

    /**
     * @brief 获取主题设置
     * @return 主题字符串："light"、"dark"或"auto"
     */
    QString theme() const { return m_theme; }

    /**
     * @brief 获取主题名称
     * @return 主题名称："JinGO"或"StarDust"
     */
    QString themeName() const { return m_themeName; }

    /**
     * @brief 判断是否为深色模式
     * @return true表示深色模式，false表示浅色模式
     *
     * @details 当主题设置为"dark"或"auto"时返回true
     */
    bool isDarkMode() const;

    /**
     * @brief 判断是否为自动主题
     * @return true表示自动跟随系统，false表示手动设置
     *
     * @details 当主题设置为"auto"时返回true
     */
    bool isAutoTheme() const;

    /**
     * @brief 获取是否允许局域网访问
     * @return true表示允许，false表示禁止
     */
    bool allowLAN() const { return m_allowLAN; }

    /**
     * @brief 获取日志级别
     * @return 日志级别（0=Debug, 1=Warning, 2=Error）
     */
    int logLevel() const { return m_logLevel; }

    /**
     * @brief 获取延迟测试方法
     * @return 测试方法（0=ConnectTest，1=PingTest）
     */
    int latencyTestMethod() const { return m_latencyTestMethod; }

    /**
     * @brief 获取可用的网络接口列表
     * @return 网络接口名称和IP地址的列表（格式："接口名 (IP地址)"）
     */
    QStringList availableNetworkInterfaces() const { return m_availableNetworkInterfaces; }

    /**
     * @brief 获取选中的网络接口
     * @return 选中的网络接口IP地址，"auto"表示自动选择
     */
    QString selectedNetworkInterface() const { return m_selectedNetworkInterface; }

    /**
     * @brief 网络接口选择是否启用
     * @return true表示有多个接口可选，false表示只有一个或没有
     */
    bool networkInterfaceSelectionEnabled() const { return m_availableNetworkInterfaces.size() > 1; }

    /**
     * @brief 设置是否自动连接
     * @param enabled true表示启用，false表示禁用
     *
     * @details 设置后自动保存到配置文件
     */
    void setAutoConnect(bool enabled);

    /**
     * @brief 设置是否开机自启动
     * @param enabled true表示启用，false表示禁用
     *
     * @details 设置后会调用平台接口设置系统开机启动，并保存到配置文件
     */
    void setAutoStart(bool enabled);

    /**
     * @brief 设置是否启用系统代理
     * @param enabled true表示启用，false表示禁用
     *
     * @details 设置后自动保存到配置文件
     */
    void setSystemProxy(bool enabled);

    /**
     * @brief 设置界面语言
     * @param language 语言代码（如"zh_CN"、"en_US"）
     *
     * @details 设置后自动保存到配置文件
     */
    void setLanguage(const QString& language);

    /**
     * @brief 设置主题
     * @param theme 主题字符串："light"、"dark"或"auto"
     *
     * @details 设置后自动保存到配置文件并发出themeChanged信号
     */
    void setTheme(const QString& theme);

    /**
     * @brief 设置主题名称
     * @param themeName 主题名称："JinGO"或"StarDust"
     *
     * @details 设置后自动保存到配置文件并发出themeNameChanged信号
     */
    void setThemeName(const QString& themeName);

    /**
     * @brief 设置是否允许局域网访问
     * @param enabled true表示允许，false表示禁止
     *
     * @details 设置后自动保存到配置文件
     */
    void setAllowLAN(bool enabled);

    /**
     * @brief 设置日志级别
     * @param level 日志级别（0=Debug, 1=Warning, 2=Error）
     *
     * @details 设置后自动保存到配置文件
     */
    void setLogLevel(int level);

    /**
     * @brief 设置延迟测试方法
     * @param method 测试方法（0=ConnectTest，1=PingTest）
     *
     * @details 设置后自动保存到配置文件
     */
    void setLatencyTestMethod(int method);

    /**
     * @brief 设置选中的网络接口
     * @param interfaceName 网络接口IP地址，"auto"表示自动选择
     *
     * @details 设置后保存到配置文件和SharedDefaults
     */
    void setSelectedNetworkInterface(const QString& interfaceName);

public slots:
    /**
     * @brief 刷新可用网络接口列表
     *
     * @details 枚举系统中所有有效的网络接口（排除loopback、utun等）
     */
    Q_INVOKABLE void refreshNetworkInterfaces();
    /**
     * @brief 从ConfigManager加载设置
     *
     * @details 加载所有配置项并发出对应的changed信号通知UI更新
     */
    void loadSettings();

    /**
     * @brief 保存设置到ConfigManager
     *
     * @details 将当前所有设置保存到配置文件并发出settingsSaved信号
     */
    void saveSettings();

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
     */
    void resetToDefaults();

    /**
     * @brief 清除缓存
     *
     * @todo 实现缓存清理功能
     */
    void clearCache();

    /**
     * @brief 导出日志
     *
     * @todo 实现日志导出功能
     */
    void exportLogs();

signals:
    /**
     * @brief 自动连接设置变化信号
     */
    void autoConnectChanged();

    /**
     * @brief 开机自启动设置变化信号
     */
    void autoStartChanged();

    /**
     * @brief 系统代理设置变化信号
     */
    void systemProxyChanged();

    /**
     * @brief 界面语言变化信号
     */
    void languageChanged();

    /**
     * @brief 主题设置变化信号
     */
    void themeChanged();

    /**
     * @brief 主题名称变化信号
     */
    void themeNameChanged();

    /**
     * @brief 局域网访问设置变化信号
     */
    void allowLANChanged();

    /**
     * @brief 日志级别变化信号
     */
    void logLevelChanged();

    /**
     * @brief 延迟测试方法变化信号
     */
    void latencyTestMethodChanged();

    /**
     * @brief 可用网络接口列表变化信号
     */
    void availableNetworkInterfacesChanged();

    /**
     * @brief 选中的网络接口变化信号
     */
    void selectedNetworkInterfaceChanged();

    /**
     * @brief 设置已保存信号
     */
    void settingsSaved();

private:
    bool m_autoConnect;             ///< 是否自动连接VPN
    bool m_autoStart;               ///< 是否开机自启动
    bool m_systemProxy;             ///< 是否启用系统代理
    QString m_language;             ///< 界面语言代码
    QString m_theme;                ///< 主题设置
    QString m_themeName;            ///< 主题名称（JinGO/StarDust）
    bool m_allowLAN;                ///< 是否允许局域网访问
    int m_logLevel;                 ///< 日志级别
    int m_latencyTestMethod;        ///< 延迟测试方法（0=ConnectTest，1=PingTest）
    QStringList m_availableNetworkInterfaces; ///< 可用网络接口列表
    QString m_selectedNetworkInterface;       ///< 选中的网络接口

    ConfigManager* m_configManager;        ///< 配置管理器指针
    PlatformInterface* m_platformInterface; ///< 平台接口指针
};

#endif // SETTINGSVIEWMODEL_H
