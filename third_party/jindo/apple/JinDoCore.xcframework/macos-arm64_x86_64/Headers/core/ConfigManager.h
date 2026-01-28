// ============================================================================
// ConfigManager.h - 配置管理器头文件
// ============================================================================
// 描述: 管理应用程序的配置设置和 Xray 配置生成
// 作者: [Your Name]
// 日期: [Date]
// 版本: 1.0
// ============================================================================

#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QSettings>
#include <QJsonObject>
#include <QJsonArray>

// 前向声明 Server 类
class Server;

/**
 * @brief 配置管理器类
 * @details 管理应用程序通用设置和 Xray 配置生成，采用单例模式，全局唯一
 * 
 * 主要功能:
 * - 加载/保存应用程序配置
 * - 管理网络设置(端口、路由模式、DNS等)
 * - 管理 TUN 虚拟网卡参数
 * - 生成 Xray 核心配置
 * - 提供配置验证和解析功能
 */
class ConfigManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(VPNMode vpnMode READ vpnMode WRITE setVPNMode NOTIFY vpnModeChanged)
    Q_PROPERTY(RoutingMode routingMode READ routingMode WRITE setRoutingMode NOTIFY routingModeChanged)
    Q_PROPERTY(LatencyTestMethod latencyTestMethod READ latencyTestMethod WRITE setLatencyTestMethod NOTIFY latencyTestMethodChanged)
    Q_PROPERTY(SpeedTestFileSize speedTestFileSize READ speedTestFileSize WRITE setSpeedTestFileSize NOTIFY speedTestFileSizeChanged)

    // 基础设置属性
    Q_PROPERTY(bool autoConnect READ autoConnect WRITE setAutoConnect NOTIFY autoConnectChanged)
    Q_PROPERTY(bool autoStart READ autoStart WRITE setAutoStart NOTIFY autoStartChanged)
    Q_PROPERTY(bool minimizeToTray READ minimizeToTray WRITE setMinimizeToTray NOTIFY minimizeToTrayChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(int localSocksPort READ localSocksPort WRITE setLocalSocksPort NOTIFY localSocksPortChanged)
    Q_PROPERTY(int localHttpPort READ localHttpPort WRITE setLocalHttpPort NOTIFY localHttpPortChanged)
    Q_PROPERTY(bool allowLAN READ allowLAN WRITE setAllowLAN NOTIFY allowLANChanged)
    Q_PROPERTY(bool bypassLAN READ bypassLAN WRITE setBypassLAN NOTIFY bypassLANChanged)

    // 传输层设置属性
    Q_PROPERTY(bool enableMux READ enableMux WRITE setEnableMux NOTIFY enableMuxChanged)
    Q_PROPERTY(int muxConcurrency READ muxConcurrency WRITE setMuxConcurrency NOTIFY muxConcurrencyChanged)
    Q_PROPERTY(bool enableFragment READ enableFragment WRITE setEnableFragment NOTIFY enableFragmentChanged)
    Q_PROPERTY(QString fragmentLength READ fragmentLength WRITE setFragmentLength NOTIFY fragmentLengthChanged)
    Q_PROPERTY(QString fragmentInterval READ fragmentInterval WRITE setFragmentInterval NOTIFY fragmentIntervalChanged)
    Q_PROPERTY(bool tcpFastOpen READ tcpFastOpen WRITE setTcpFastOpen NOTIFY tcpFastOpenChanged)
    Q_PROPERTY(bool trafficSniffing READ trafficSniffing WRITE setTrafficSniffing NOTIFY trafficSniffingChanged)

    // 日志设置属性
    Q_PROPERTY(int logLevel READ logLevelInt WRITE setLogLevelInt NOTIFY logLevelChanged)
    Q_PROPERTY(bool enableAccessLog READ enableAccessLog WRITE setEnableAccessLog NOTIFY enableAccessLogChanged)
    Q_PROPERTY(int logRetentionDays READ logRetentionDays WRITE setLogRetentionDays NOTIFY logRetentionDaysChanged)

    // 超时设置属性
    // 注意: apiPort 已移除 - 流量统计现在通过 SuperRay 直接 API 查询
    Q_PROPERTY(int connectTimeout READ connectTimeout WRITE setConnectTimeout NOTIFY connectTimeoutChanged)
    Q_PROPERTY(int testTimeout READ testTimeout WRITE setTestTimeout NOTIFY testTimeoutChanged)
    Q_PROPERTY(int latencyTestInterval READ latencyTestInterval WRITE setLatencyTestInterval NOTIFY latencyTestIntervalChanged)

    // 订阅设置属性
    Q_PROPERTY(int subscriptionUpdateInterval READ subscriptionUpdateInterval WRITE setSubscriptionUpdateInterval NOTIFY subscriptionUpdateIntervalChanged)

    // 高级设置属性
    Q_PROPERTY(bool customGeoIP READ customGeoIP WRITE setCustomGeoIP NOTIFY customGeoIPChanged)
    Q_PROPERTY(bool customGeoSite READ customGeoSite WRITE setCustomGeoSite NOTIFY customGeoSiteChanged)
    Q_PROPERTY(QString domainStrategy READ domainStrategy WRITE setDomainStrategy NOTIFY domainStrategyChanged)
    Q_PROPERTY(QString dnsQueryStrategy READ dnsQueryStrategy WRITE setDnsQueryStrategy NOTIFY dnsQueryStrategyChanged)
    Q_PROPERTY(QString listenAddress READ listenAddress WRITE setListenAddress NOTIFY listenAddressChanged)
    Q_PROPERTY(QStringList dnsServers READ dnsServers WRITE setDNSServers NOTIFY dnsServersChanged)

    // Android 分应用代理设置
    Q_PROPERTY(int perAppProxyMode READ perAppProxyMode WRITE setPerAppProxyMode NOTIFY perAppProxyModeChanged)
    Q_PROPERTY(QStringList perAppProxyList READ perAppProxyList WRITE setPerAppProxyList NOTIFY perAppProxyListChanged)

    // 用户国家代码
    Q_PROPERTY(QString userCountryCode READ userCountryCode WRITE setUserCountryCode NOTIFY userCountryCodeChanged)

public:
    /**
     * @brief 日志等级枚举
     * @details 定义不同的日志详细程度
     */
    enum LogLevel {
        None,     ///< 无日志输出
        Error,    ///< 仅输出错误信息
        Warning,  ///< 输出警告及错误信息
        Info,     ///< 输出普通信息级别
        Debug     ///< 输出调试级别详细信息
    };
    Q_ENUM(LogLevel)

    /**
     * @brief VPN 模式枚举
     * @details 定义 VPN 的工作模式 (对应 NetworkExtensionManager 的 VPNMode)
     *
     * 重要：枚举值必须与 NetworkExtensionManager.h 中的 VPNMode 完全一致
     * - VPNModeTUN = 0 (Objective-C) <-> TUN = 0 (C++)
     * - VPNModeProxy = 1 (Objective-C) <-> Proxy = 1 (C++)
     */
    enum VPNMode {
        TUN = 0,           ///< TUN 模式 (NEPacketTunnelProvider - 全局VPN,捕获所有IP层流量)
        Proxy = 1          ///< Proxy 模式 (系统代理 - macOS/Windows/Linux)
    };
    Q_ENUM(VPNMode)

    /**
     * @brief 路由模式枚举
     * @details 定义代理的路由规则
     */
    enum RoutingMode {
        Global,        ///< 全局模式：所有流量都走代理
        Rule,          ///< 规则模式：根据 GeoIP 决定是否走代理
        Direct,        ///< 直连模式：所有流量直连不走代理
        Subscription   ///< 订阅模式：使用订阅配置中的路由规则
    };
    Q_ENUM(RoutingMode)

    /**
     * @brief 延时测试方法
     * @details 使用 SuperRay API 进行延迟测试
     */
    enum LatencyTestMethod {
        TCPTest,       ///< TCP测试：使用 SuperRay_TCPPing 直接连接服务器端口测试延迟
        HTTPTest       ///< HTTP测试：使用 SuperRay_HTTPPing 通过代理进行 HTTP 请求测试
    };
    Q_ENUM(LatencyTestMethod)

    /**
     * @brief 测速文件大小枚举
     * @details 使用 Cloudflare CDN 进行下载测速
     */
    enum SpeedTestFileSize {
        SpeedTest10MB = 10,    ///< 10MB 测速文件（快速测试）
        SpeedTest25MB = 25,    ///< 25MB 测速文件（标准测试）
        SpeedTest100MB = 100   ///< 100MB 测速文件（精确测试）
    };
    Q_ENUM(SpeedTestFileSize)

    /**
     * @brief 分应用代理模式枚举 (Android)
     * @details 定义哪些应用走代理
     */
    enum PerAppProxyMode {
        PerAppDisabled = 0,    ///< 禁用分应用代理，所有应用走代理
        PerAppAllowList = 1,   ///< 仅允许列表中的应用走代理
        PerAppBlockList = 2    ///< 排除列表中的应用，其他走代理
    };
    Q_ENUM(PerAppProxyMode)

    // ========================================================================
    // 公共方法 - 单例访问
    // ========================================================================
    
    /**
     * @brief 获取单例实例
     * @return ConfigManager& 配置管理器的全局唯一实例
     */
    static ConfigManager& instance();

    // ========================================================================
    // 公共方法 - 配置文件操作
    // ========================================================================
    
    /**
     * @brief 加载配置文件
     * @return bool 成功返回 true，失败返回 false
     * @details 从配置文件中读取所有设置项并加载到内存
     */
    bool load();
    
    /**
     * @brief 保存配置文件
     * @return bool 成功返回 true，失败返回 false
     * @details 将当前内存中的设置保存到配置文件
     */
    Q_INVOKABLE bool save();
    
    /**
     * @brief 重置为默认配置
     * @details 清除所有设置并恢复为初始默认值
     */
    Q_INVOKABLE void reset();

    // ========================================================================
    // 公共方法 - 通用键值对访问接口
    // ========================================================================
    
    /**
     * @brief 设置配置项
     * @param key 配置键名
     * @param value 配置值
     */
    void setSetting(const QString& key, const QVariant& value);
    
    /**
     * @brief 获取配置项
     * @param key 配置键名
     * @param defaultValue 默认值（如果键不存在则返回此值）
     * @return QVariant 配置值
     */
    QVariant getSetting(const QString& key, const QVariant& defaultValue = QVariant()) const;
    
    /**
     * @brief 移除配置项
     * @param key 要移除的配置键名
     */
    void removeSetting(const QString& key);
    
    /**
     * @brief 检查配置项是否存在
     * @param key 配置键名
     * @return bool 存在返回 true，否则返回 false
     */
    bool hasSetting(const QString& key) const;

    // ========================================================================
    // 公共方法 - 应用设置
    // ========================================================================
    
    /**
     * @brief 设置开机自启
     * @param enabled true 启用，false 禁用
     */
    void setAutoStart(bool enabled);

    /**
     * @brief 获取开机自启设置
     * @return bool 当前是否启用开机自启
     */
    Q_INVOKABLE bool autoStart() const;

    /**
     * @brief 设置最小化到系统托盘
     * @param enabled true 启用，false 禁用
     */
    void setMinimizeToTray(bool enabled);

    /**
     * @brief 获取最小化到系统托盘设置
     * @return bool 当前是否启用最小化到托盘
     */
    Q_INVOKABLE bool minimizeToTray() const;

    /**
     * @brief 设置自动连接
     * @param enabled true 启用，false 禁用
     * @details 启动时自动连接到上次使用的服务器
     */
    void setAutoConnect(bool enabled);
    
    /**
     * @brief 获取自动连接设置
     * @return bool 当前是否启用自动连接
     */
    bool autoConnect() const;

    /**
     * @brief 设置最近使用的服务器 ID
     * @param serverId 服务器唯一标识
     */
    void setLastServerId(const QString& serverId);
    
    /**
     * @brief 获取最近使用的服务器 ID
     * @return QString 服务器唯一标识
     */
    QString lastServerId() const;

    // ========================================================================
    // 公共方法 - 网络设置
    // ========================================================================
    
    /**
     * @brief 设置本地 SOCKS5 端口
     * @param port 端口号 (推荐 1080)
     */
    void setLocalSocksPort(int port);
    
    /**
     * @brief 获取本地 SOCKS5 端口
     * @return int 当前配置的 SOCKS5 端口号
     */
    Q_INVOKABLE int localSocksPort() const;

    /**
     * @brief 设置本地 HTTP 代理端口
     * @param port HTTP 代理端口号 (1024-65535)
     */
    Q_INVOKABLE void setLocalHttpPort(int port);

    /**
     * @brief 获取本地 HTTP 代理端口
     * @return int 当前配置的 HTTP 代理端口号
     */
    Q_INVOKABLE int localHttpPort() const;

    /**
     * @brief 设置 VPN 模式
     * @param mode VPN 模式枚举值
     * @see VPNMode
     */
    Q_INVOKABLE void setVPNMode(VPNMode mode);

    /**
     * @brief 获取 VPN 模式
     * @return VPNMode 当前 VPN 模式
     */
    VPNMode vpnMode() const;

    /**
     * @brief 设置路由模式
     * @param mode 路由模式枚举值
     * @see RoutingMode
     */
    Q_INVOKABLE void setRoutingMode(RoutingMode mode);

    /**
     * @brief 获取路由模式
     * @return RoutingMode 当前路由模式
     */
    RoutingMode routingMode() const;

    /**
     * @brief 设置延时测试方法
     * @param method 测试方法（ConnectTest 或 PingTest）
     */
    Q_INVOKABLE void setLatencyTestMethod(LatencyTestMethod method);

    /**
     * @brief 获取延时测试方法
     * @return LatencyTestMethod 当前测试方法
     */
    LatencyTestMethod latencyTestMethod() const;

    /**
     * @brief 设置用户所在国家代码
     * @param countryCode 国家代码（如 "cn", "us"）
     */
    void setUserCountryCode(const QString& countryCode);

    /**
     * @brief 获取用户所在国家代码
     * @return QString 国家代码
     */
    QString userCountryCode() const;

    /**
     * @brief 设置是否允许局域网访问
     * @param allow true 允许，false 仅本地访问
     */
    void setAllowLAN(bool allow);
    void setBypassLAN(bool bypass);
    
    /**
     * @brief 获取是否允许局域网访问
     * @return bool 当前设置
     */
    Q_INVOKABLE bool bypassLAN() const;
    Q_INVOKABLE bool allowLAN() const;

    /**
     * @brief 设置是否启用系统代理
     * @param enabled true 启用，false 禁用
     */
    void setSystemProxy(bool enabled);
    
    /**
     * @brief 获取是否启用系统代理
     * @return bool 当前设置
     */
    bool systemProxy() const;

    /**
     * @brief 设置是否启用 UDP 转发
     * @param enabled true 启用，false 禁用
     * @details UDP 用于 DNS 查询和部分应用
     */
    void setEnableUDP(bool enabled);
    
    /**
     * @brief 获取是否启用 UDP 转发
     * @return bool 当前设置
     */
    bool enableUDP() const;

    /**
     * @brief 设置是否启用多路复用 (Mux)
     * @param enabled true 启用，false 禁用
     * @details Mux 可以减少 TCP 连接数，提高性能
     */
    void setEnableMux(bool enabled);
    
    /**
     * @brief 获取是否启用多路复用
     * @return bool 当前设置
     */
    Q_INVOKABLE bool enableMux() const;

    /**
     * @brief 获取 Mux 并发连接数
     * @return int 并发连接数
     */
    Q_INVOKABLE int muxConcurrency() const;
    Q_INVOKABLE bool enableFragment() const;
    Q_INVOKABLE QString fragmentLength() const;
    Q_INVOKABLE QString fragmentInterval() const;

    /**
     * @brief 设置 Mux 并发连接数
     * @param concurrency 并发连接数 (推荐 8)
     */
    void setMuxConcurrency(int concurrency);
    void setEnableFragment(bool enable);
    void setFragmentLength(const QString& length);
    void setFragmentInterval(const QString& interval);

    /**
     * @brief 设置域名解析策略
     * @param strategy 解析策略 ("AsIs", "IPIfNonMatch", "IPOnDemand")
     */
    void setDomainStrategy(const QString& strategy);

    /**
     * @brief 获取域名解析策略
     * @return QString 解析策略
     */
    QString domainStrategy() const;

    /**
     * @brief 设置DNS查询策略
     * @param strategy 查询策略 ("UseIP", "UseIPv4", "UseIPv6")
     */
    void setDnsQueryStrategy(const QString& strategy);

    /**
     * @brief 获取DNS查询策略
     * @return QString 查询策略
     */
    QString dnsQueryStrategy() const;

    /**
     * @brief 设置监听地址
     * @param address 监听地址 (如 "127.0.0.1", "0.0.0.0")
     */
    void setListenAddress(const QString& address);

    /**
     * @brief 获取监听地址
     * @return QString 监听地址
     */
    QString listenAddress() const;

    /**
     * @brief 设置是否启用TCP Fast Open
     * @param enabled true启用，false禁用
     */
    void setTcpFastOpen(bool enabled);

    /**
     * @brief 获取是否启用TCP Fast Open
     * @return bool 当前设置
     */
    Q_INVOKABLE bool tcpFastOpen() const;

    /**
     * @brief 设置是否启用流量嗅探
     * @param enabled true启用，false禁用
     */
    void setTrafficSniffing(bool enabled);

    /**
     * @brief 获取是否启用流量嗅探
     * @return bool 当前设置
     */
    Q_INVOKABLE bool trafficSniffing() const;

    /**
     * @brief 设置是否启用访问日志
     * @param enabled true启用，false禁用
     */
    void setEnableAccessLog(bool enabled);

    /**
     * @brief 获取是否启用访问日志
     * @return bool 当前设置
     */
    Q_INVOKABLE bool enableAccessLog() const;

    /**
     * @brief 设置日志保留天数
     * @param days 保留天数
     */
    void setLogRetentionDays(int days);

    /**
     * @brief 获取日志保留天数
     * @return int 保留天数
     */
    Q_INVOKABLE int logRetentionDays() const;

    // 注意: setApiPort/apiPort 已移除 - 流量统计现在通过 SuperRay 直接 API 查询

    /**
     * @brief 设置连接超时时间
     * @param timeout 超时时间（秒）
     */
    void setConnectTimeout(int timeout);

    /**
     * @brief 获取连接超时时间
     * @return int 超时时间（秒）
     */
    Q_INVOKABLE int connectTimeout() const;

    /**
     * @brief 设置是否使用自定义GeoIP数据库
     * @param enabled true启用，false禁用
     */
    void setCustomGeoIP(bool enabled);

    /**
     * @brief 获取是否使用自定义GeoIP数据库
     * @return bool 当前设置
     */
    bool customGeoIP() const;

    /**
     * @brief 设置是否使用自定义GeoSite数据库
     * @param enabled true启用，false禁用
     */
    void setCustomGeoSite(bool enabled);

    /**
     * @brief 获取是否使用自定义GeoSite数据库
     * @return bool 当前设置
     */
    bool customGeoSite() const;

    // ========================================================================
    // 公共方法 - 日志设置
    // ========================================================================
    
    /**
     * @brief 设置日志等级
     * @param level 日志等级枚举值
     * @see LogLevel
     */
    void setLogLevel(LogLevel level);
    
    /**
     * @brief 获取日志等级
     * @return LogLevel 当前日志等级
     */
    LogLevel logLevel() const;
    
    /**
     * @brief 获取日志等级 (int 版本，供 QML 使用)
     * @return int 当前日志等级的整数值
     */
    int logLevelInt() const;
    
    /**
     * @brief 设置日志等级 (int 版本，供 QML 使用)
     * @param level 日志等级整数值 (0-4)
     */
    void setLogLevelInt(int level);

    // ========================================================================
    // 公共方法 - DNS 设置
    // ========================================================================
    
    /**
     * @brief 设置 DNS 服务器列表
     * @param servers DNS 服务器地址列表
     * @details 例如: {"8.8.8.8", "1.1.1.1"}
     */
    void setDNSServers(const QStringList& servers);
    
    /**
     * @brief 获取 DNS 服务器列表
     * @return QStringList DNS 服务器地址列表
     */
    Q_INVOKABLE QStringList dnsServers() const;

    // ========================================================================
    // 公共方法 - Android 分应用代理设置
    // ========================================================================

    /**
     * @brief 设置分应用代理模式
     * @param mode 模式 (0=禁用, 1=仅允许列表, 2=排除列表)
     */
    void setPerAppProxyMode(int mode);

    /**
     * @brief 获取分应用代理模式
     * @return int 当前模式
     */
    int perAppProxyMode() const;

    /**
     * @brief 设置分应用代理列表
     * @param apps 应用包名列表
     */
    void setPerAppProxyList(const QStringList& apps);

    /**
     * @brief 获取分应用代理列表
     * @return QStringList 应用包名列表
     */
    QStringList perAppProxyList() const;

    /**
     * @brief 添加应用到分应用代理列表
     * @param packageName 应用包名
     */
    Q_INVOKABLE void addPerAppProxyApp(const QString& packageName);

    /**
     * @brief 从分应用代理列表移除应用
     * @param packageName 应用包名
     */
    Q_INVOKABLE void removePerAppProxyApp(const QString& packageName);

    /**
     * @brief 检查应用是否在分应用代理列表中
     * @param packageName 应用包名
     * @return bool 是否在列表中
     */
    Q_INVOKABLE bool isAppInPerAppProxyList(const QString& packageName) const;

    // ========================================================================
    // 公共方法 - 测试设置
    // ========================================================================

    /**
     * @brief 设置网络测试 URL
     * @param url 测试 URL (推荐: https://www.google.com/generate_204)
     */
    void setTestURL(const QString& url);

    /**
     * @brief 获取网络测试 URL
     * @return QString 当前测试 URL
     */
    QString testURL() const;

    /**
     * @brief 设置网络测试超时
     * @param timeout 超时时间（秒）
     */
    void setTestTimeout(int timeout);

    /**
     * @brief 获取网络测试超时
     * @return int 超时时间（秒）
     */
    Q_INVOKABLE int testTimeout() const;

    /**
     * @brief 设置延时测试间隔
     * @param interval 间隔时间（秒），0表示禁用
     */
    void setLatencyTestInterval(int interval);

    /**
     * @brief 获取延时测试间隔
     * @return int 间隔时间（秒），0表示禁用
     */
    Q_INVOKABLE int latencyTestInterval() const;

    /**
     * @brief 设置订阅更新间隔
     * @param hours 更新间隔（小时）
     */
    void setSubscriptionUpdateInterval(int hours);

    /**
     * @brief 获取订阅更新间隔
     * @return int 更新间隔（小时）
     */
    Q_INVOKABLE int subscriptionUpdateInterval() const;

    // ========================================================================
    // 公共方法 - 测速设置
    // ========================================================================

    /**
     * @brief 设置测速文件大小
     * @param size 文件大小枚举值
     */
    Q_INVOKABLE void setSpeedTestFileSize(SpeedTestFileSize size);

    /**
     * @brief 获取测速文件大小
     * @return SpeedTestFileSize 当前测速文件大小
     */
    SpeedTestFileSize speedTestFileSize() const;

    /**
     * @brief 获取 Cloudflare 测速 URL
     * @return QString 根据当前配置的文件大小返回对应的 URL
     */
    Q_INVOKABLE QString speedTestUrl() const;

    /**
     * @brief 获取测速文件字节数
     * @return qint64 文件大小（字节）
     */
    Q_INVOKABLE qint64 speedTestFileBytes() const;

    // ========================================================================
    // 公共方法 - 国际化设置
    // ========================================================================
    
    /**
     * @brief 设置应用语言
     * @param language 语言代码 (如 "zh_CN", "en_US")
     */
    void setLanguage(const QString& language);
    
    /**
     * @brief 获取应用语言
     * @return QString 当前语言代码
     */
    QString language() const;

    /**
     * @brief 设置 UI 主题
     * @param theme 主题名称 ("light", "dark", "auto")
     */
    void setTheme(const QString& theme);
    
    /**
     * @brief 获取 UI 主题
     * @return QString 当前主题名称
     */
    QString theme() const;

    // ========================================================================
    // 公共方法 - TUN 虚拟网卡参数
    // ========================================================================
    
    /**
     * @brief 设置 TUN 设备名称
     * @param name 设备名称 (如 "jingo0")
     */
    void setTunDeviceName(const QString& name);
    
    /**
     * @brief 获取 TUN 设备名称
     * @return QString 设备名称
     */
    QString tunDeviceName() const;

    /**
     * @brief 设置 TUN 设备 IP 地址
     * @param ip IP 地址 (如 "172.19.0.1")
     */
    void setTunIPAddress(const QString& ip);
    
    /**
     * @brief 获取 TUN 设备 IP 地址
     * @return QString IP 地址
     */
    QString tunIPAddress() const;

    /**
     * @brief 设置 TUN 网关地址
     * @param gateway 网关地址 (如 "172.19.0.254")
     */
    void setTunGateway(const QString& gateway);
    
    /**
     * @brief 获取 TUN 网关地址
     * @return QString 网关地址
     */
    QString tunGateway() const;

    /**
     * @brief 设置 TUN 子网掩码
     * @param netmask 子网掩码 (如 "255.255.255.0")
     */
    void setTunNetmask(const QString& netmask);
    
    /**
     * @brief 获取 TUN 子网掩码
     * @return QString 子网掩码
     */
    QString tunNetmask() const;

    /**
     * @brief 设置 TUN MTU 值
     * @param mtu MTU 值 (推荐 1500)
     * @details MTU (Maximum Transmission Unit) 最大传输单元
     */
    void setTunMTU(int mtu);
    
    /**
     * @brief 获取 TUN MTU 值
     * @return int MTU 值
     */
    int tunMTU() const;

    /**
     * @brief 获取是否路由所有流量
     * @return bool true 表示路由所有流量
     */
    bool routeAllTraffic() const;

    /**
     * @brief 设置是否路由所有流量
     * @param enabled true 表示路由所有流量，false 仅路由代理流量
     * @note 在TUN模式下，此选项控制是否劫持所有流量
     */
    void setRouteAllTraffic(bool enabled);

    // ========================================================================
    // 公共方法 - Xray 配置生成
    // ========================================================================
    
    /**
     * @brief 生成 Xray 配置（JSON 字符串）
     * @param server 服务器配置对象
     * @param forTUN 是否为 TUN 模式生成配置
     * @param enableMetrics 是否启用 metrics（测速时应为 false，连接时应为 true）
     * @return QString 生成的 JSON 配置字符串
     * @details TUN 模式下只生成 SOCKS5 入站，系统代理模式下生成 SOCKS5 + HTTP 入站
     */
    QString generateXrayConfig(Server* server, bool forTUN = true, bool enableAPI = true) const;

    /**
     * @brief 验证配置是否有效
     * @param configJson JSON 配置字符串
     * @return bool 有效返回 true，无效返回 false
     * @details 检查 JSON 格式和必需字段是否存在
     */
    bool validateConfig(const QString& configJson) const;

    /**
     * @brief 解析 JSON 配置
     * @param configJson JSON 配置字符串
     * @return QJsonObject 解析后的 JSON 对象
     */
    QJsonObject parseConfig(const QString& configJson) const;

signals:
    /**
     * @brief 配置变更信号
     * @param key 变更的配置键名
     * @details 当任何配置项发生变化时发出此信号
     */
    void configChanged(const QString& key);

    /**
     * @brief 配置重置信号
     * @details 当配置被重置为默认值时发出此信号
     */
    void configReset();

    /**
     * @brief VPN 模式变更信号
     * @details 当 VPN 模式发生变化时发出此信号
     */
    void vpnModeChanged();

    /**
     * @brief 路由模式变更信号
     * @details 当路由模式发生变化时发出此信号
     */
    void routingModeChanged();

    /**
     * @brief 延时测试方法变更信号
     * @details 当延时测试方法发生变化时发出此信号
     */
    void latencyTestMethodChanged();

    /**
     * @brief 测速文件大小变更信号
     */
    void speedTestFileSizeChanged();

    // 基础设置变更信号
    void autoConnectChanged();
    void autoStartChanged();
    void minimizeToTrayChanged();
    void localSocksPortChanged();
    void localHttpPortChanged();
    void bypassLANChanged();
    void allowLANChanged();
    void languageChanged();
    void themeChanged();

    // 传输层设置变更信号
    void enableMuxChanged();
    void muxConcurrencyChanged();
    void enableFragmentChanged();
    void fragmentLengthChanged();
    void fragmentIntervalChanged();
    void tcpFastOpenChanged();
    void trafficSniffingChanged();

    // 日志设置变更信号
    void logLevelChanged();
    void enableAccessLogChanged();
    void logRetentionDaysChanged();

    // 超时设置变更信号
    // 注意: apiPortChanged 已移除
    void connectTimeoutChanged();
    void testTimeoutChanged();
    void latencyTestIntervalChanged();

    // 订阅设置变更信号
    void subscriptionUpdateIntervalChanged();

    // 高级设置变更信号
    void domainStrategyChanged();
    void dnsQueryStrategyChanged();
    void listenAddressChanged();
    void customGeoIPChanged();
    void customGeoSiteChanged();
    void dnsServersChanged();

    // Android 分应用代理变更信号
    void perAppProxyModeChanged();
    void perAppProxyListChanged();

    // 用户国家代码变更信号
    void userCountryCodeChanged();

private:
    // ========================================================================
    // 私有方法 - 构造/析构 (单例模式)
    // ========================================================================
    
    /**
     * @brief 私有构造函数
     * @param parent 父对象指针
     * @details 单例模式，禁止外部直接创建实例
     */
    ConfigManager(QObject* parent = nullptr);
    
    /**
     * @brief 析构函数
     * @details 自动保存配置并清理资源
     */
    ~ConfigManager();

    /**
     * @brief 禁止拷贝构造
     */
    ConfigManager(const ConfigManager&) = delete;
    
    /**
     * @brief 禁止赋值操作
     */
    ConfigManager& operator=(const ConfigManager&) = delete;

    // ========================================================================
    // 私有方法 - Xray 配置生成辅助函数
    // ========================================================================
    
    /**
     * @brief 生成入站配置数组
     * @param forTUN 是否为 TUN 模式
     * @return QJsonArray 入站配置 JSON 数组
     * @details TUN 模式仅 SOCKS5，系统代理模式包含 SOCKS5 + HTTP
     */
    QJsonArray generateInbounds(bool forTUN) const;

    /**
     * @brief 获取面板域名列表（从API基础URL动态提取）
     * @return QStringList 面板域名列表（包含主域名和完整域名）
     * @details 自动从ApiClient的baseUrl中提取域名，例如从"https://cp.jingo.cfd/api/v1"
     *          提取出"jingo.cfd"（主域名）和"cp.jingo.cfd"（完整域名）
     */
    QStringList getPanelDomains() const;

    /**
     * @brief 生成路由配置对象
     * @param server 当前服务器对象（用于提取服务器域名）
     * @return QJsonObject 路由配置 JSON 对象
     * @details 根据路由模式生成对应的规则
     */
    QJsonObject generateRouting(Server* server = nullptr) const;
    
    /**
     * @brief 生成 DNS 配置对象
     * @param server 当前服务器对象（用于提取服务器域名）
     * @return QJsonObject DNS 配置 JSON 对象
     */
    QJsonObject generateDNS(Server* server = nullptr) const;
    
    /**
     * @brief 生成日志配置对象
     * @return QJsonObject 日志配置 JSON 对象
     */
    QJsonObject generateLog() const;
    
    /**
     * @brief 生成策略配置对象
     * @return QJsonObject 策略配置 JSON 对象
     * @details 包含流量统计等策略设置
     */
    QJsonObject generatePolicy() const;

    /**
     * @brief 加载默认配置
     * @details 初始化或重置时调用，设置所有默认值
     */
    void loadDefaults();

    /**
     * @brief 从QSettings加载已保存的配置
     * @details 从配置文件读取已保存的值覆盖默认值
     */
    void loadFromSettings();

    /**
     * @brief 清理配置文件中的重复键
     * @details QSettings在某些平台会将键名转为小写,此函数清理这些重复的小写键
     */
    void cleanupDuplicateKeys();

    /**
     * @brief 获取配置文件路径
     * @return QString 配置文件的完整路径
     */
    QString configFilePath() const;

private:
    // ========================================================================
    // 私有成员变量 - 核心
    // ========================================================================

    QSettings* m_settings;    ///< 配置文件存储对象
    bool m_loaded;            ///< 是否已加载配置文件

    // ========================================================================
    // 私有成员变量 - 应用设置
    // ========================================================================

    bool m_autoStart;              ///< 是否开机自启动
    bool m_minimizeToTray;         ///< 是否最小化到系统托盘
    bool m_autoConnect;            ///< 是否自动连接
    QString m_language;            ///< 界面语言
    QString m_theme;               ///< UI主题

    // ========================================================================
    // 私有成员变量 - 网络设置
    // ========================================================================

    int m_localSocksPort;          ///< 本地 SOCKS5 端口
    int m_localHttpPort;           ///< 本地 HTTP 代理端口
    VPNMode m_vpnMode;                   ///< VPN 工作模式
    RoutingMode m_routingMode;           ///< 路由模式
    LatencyTestMethod m_latencyTestMethod; ///< 延时测试方法
    SpeedTestFileSize m_speedTestFileSize; ///< 测速文件大小
    QString m_userCountryCode;           ///< 用户所在国家代码
    bool m_allowLAN;               ///< 是否允许局域网访问
    bool m_bypassLAN;
    bool m_systemProxy;            ///< 是否启用系统代理
    bool m_enableUDP;              ///< 是否启用 UDP 转发
    bool m_enableMux;              ///< 是否启用多路复用
    int m_muxConcurrency;          ///< Mux 并发连接数
    bool m_enableFragment;         ///< 是否启用分片
    QString m_fragmentLength;      ///< 分片长度 (如 "100-200")
    QString m_fragmentInterval;    ///< 分片间隔 (如 "10-20")
    LogLevel m_logLevel;           ///< 日志等级
    QStringList m_dnsServers;      ///< DNS 服务器列表
    QString m_testURL;             ///< 网络测试 URL
    int m_testTimeout;             ///< 网络测试超时（秒）
    int m_latencyTestInterval;     ///< 延时测试间隔（秒），0表示禁用
    int m_subscriptionUpdateInterval; ///< 订阅更新间隔（小时）

    bool m_routeAllTraffic;        ///< 是否路由所有流量

    // ========================================================================
    // 私有成员变量 - 新增配置项
    // ========================================================================

    QString m_domainStrategy;      ///< 域名解析策略
    QString m_dnsQueryStrategy;    ///< DNS查询策略
    QString m_listenAddress;       ///< 监听地址
    bool m_tcpFastOpen;            ///< 是否启用TCP Fast Open
    bool m_trafficSniffing;        ///< 是否启用流量嗅探
    bool m_enableAccessLog;        ///< 是否启用访问日志
    int m_logRetentionDays;        ///< 日志保留天数
    int m_connectTimeout;          ///< 连接超时时间（秒）
    bool m_customGeoIP;            ///< 是否使用自定义GeoIP数据库
    bool m_customGeoSite;          ///< 是否使用自定义GeoSite数据库

    // ========================================================================
    // 私有成员变量 - TUN 参数
    // ========================================================================

    QString m_tunDeviceName;       ///< TUN 设备名称
    QString m_tunIPAddress;        ///< TUN IP 地址
    QString m_tunGateway;          ///< TUN 网关地址
    QString m_tunNetmask;          ///< TUN 子网掩码
    int m_tunMTU;                  ///< TUN MTU 值

    // ========================================================================
    // 私有成员变量 - Android 分应用代理
    // ========================================================================

    PerAppProxyMode m_perAppProxyMode;  ///< 分应用代理模式
    QStringList m_perAppProxyList;      ///< 分应用代理应用列表

    // 注意: s_expvarInitialized 已移除
    // 流量统计现在通过 SuperRay 直接 API (SuperRay_GetXrayStats) 查询
    // 不再需要 Xray expvar/metrics 端点
};

#endif // CONFIGMANAGER_H