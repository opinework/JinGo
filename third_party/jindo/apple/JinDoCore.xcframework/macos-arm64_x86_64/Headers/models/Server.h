/**
 * @file Server.h
 * @brief 服务器数据模型头文件
 * @details 定义服务器的详细信息，包括连接参数、统计数据和元数据
 * @author JinGo VPN Team
 * @date 2025
 */

#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QStringList>
#include <QVariantMap>

/**
 * @class Server
 * @brief 服务器数据模型
 */
class Server : public QObject
{
    Q_OBJECT

    // 基本属性
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString address READ address WRITE setAddress NOTIFY addressChanged)
    Q_PROPERTY(QString resolvedIP READ resolvedIP WRITE setResolvedIP NOTIFY resolvedIPChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(QString protocol READ protocol WRITE setProtocol NOTIFY protocolChanged)
    Q_PROPERTY(QString location READ location WRITE setLocation NOTIFY locationChanged)
    Q_PROPERTY(QString displayName READ displayName NOTIFY nameChanged NOTIFY locationChanged NOTIFY addressChanged)
    Q_PROPERTY(QString fullAddress READ fullAddress NOTIFY addressChanged NOTIFY portChanged)

    // 状态属性
    Q_PROPERTY(int latency READ latency WRITE setLatency NOTIFY latencyChanged)
    Q_PROPERTY(QString latencyText READ latencyText NOTIFY latencyChanged)
    Q_PROPERTY(QString latencyLevel READ latencyLevel NOTIFY latencyChanged)
    Q_PROPERTY(bool isFavorite READ isFavorite WRITE setIsFavorite NOTIFY isFavoriteChanged)
    Q_PROPERTY(bool isAvailable READ isAvailable WRITE setIsAvailable NOTIFY isAvailableChanged)
    Q_PROPERTY(bool isTestingSpeed READ isTestingSpeed WRITE setIsTestingSpeed NOTIFY isTestingSpeedChanged)
    Q_PROPERTY(QDateTime lastTested READ lastTested WRITE setLastTested NOTIFY lastTestedChanged)
    Q_PROPERTY(QString lastTestedText READ lastTestedText NOTIFY lastTestedChanged)

    // 统计属性
    Q_PROPERTY(qint64 totalUpload READ totalUpload NOTIFY statsChanged)
    Q_PROPERTY(qint64 totalDownload READ totalDownload NOTIFY statsChanged)
    Q_PROPERTY(qint64 totalTraffic READ totalTraffic NOTIFY statsChanged)
    Q_PROPERTY(QString totalUploadText READ totalUploadText NOTIFY statsChanged)
    Q_PROPERTY(QString totalDownloadText READ totalDownloadText NOTIFY statsChanged)
    Q_PROPERTY(QString totalTrafficText READ totalTrafficText NOTIFY statsChanged)
    Q_PROPERTY(int connectCount READ connectCount NOTIFY statsChanged)
    Q_PROPERTY(QDateTime lastConnected READ lastConnected NOTIFY statsChanged)
    Q_PROPERTY(QString lastConnectedText READ lastConnectedText NOTIFY statsChanged)

    // 速度统计属性
    Q_PROPERTY(quint64 peakUploadSpeed READ peakUploadSpeed NOTIFY speedStatsChanged)
    Q_PROPERTY(quint64 peakDownloadSpeed READ peakDownloadSpeed NOTIFY speedStatsChanged)
    Q_PROPERTY(quint64 averageUploadSpeed READ averageUploadSpeed NOTIFY speedStatsChanged)
    Q_PROPERTY(quint64 averageDownloadSpeed READ averageDownloadSpeed NOTIFY speedStatsChanged)
    Q_PROPERTY(QString peakUploadSpeedText READ peakUploadSpeedText NOTIFY speedStatsChanged)
    Q_PROPERTY(QString peakDownloadSpeedText READ peakDownloadSpeedText NOTIFY speedStatsChanged)
    Q_PROPERTY(QString averageUploadSpeedText READ averageUploadSpeedText NOTIFY speedStatsChanged)
    Q_PROPERTY(QString averageDownloadSpeedText READ averageDownloadSpeedText NOTIFY speedStatsChanged)

    // UI 相关属性
    Q_PROPERTY(QString iconName READ iconName NOTIFY protocolChanged)
    Q_PROPERTY(QString locationFlag READ locationFlag NOTIFY locationChanged)
    Q_PROPERTY(int score READ score NOTIFY latencyChanged NOTIFY isAvailableChanged NOTIFY isFavoriteChanged)

    // 服务器元数据属性
    Q_PROPERTY(QString countryCode READ countryCode NOTIFY nameChanged)
    Q_PROPERTY(QString continent READ continent NOTIFY nameChanged)
    Q_PROPERTY(QString countryFlag READ countryFlag NOTIFY nameChanged)
    Q_PROPERTY(double serverLoad READ serverLoad CONSTANT)
    Q_PROPERTY(QString bandwidth READ bandwidth CONSTANT)
    Q_PROPERTY(bool isPro READ isPro CONSTANT)

public:
    // 协议类型枚举
    enum Protocol { Unknown, VMess, VLESS, Trojan, Shadowsocks, Socks, HTTP, Hysteria, Hysteria2, TUIC, WireGuard };
    Q_ENUM(Protocol)

    // 网络类型枚举
    enum NetworkType { TCP, KCP, WebSocket, HTTP2, QUIC, GRPC };
    Q_ENUM(NetworkType)

    // 安全层类型枚举
    enum SecurityType { NoSecurity, TLS, XTLS, Reality };
    Q_ENUM(SecurityType)

    explicit Server(QObject* parent = nullptr);
    explicit Server(const QString& id, QObject* parent = nullptr);
    ~Server() override;

    // Getters
    QString id() const;
    QString name() const;
    QString address() const;
    QString resolvedIP() const;
    int port() const;
    QString protocol() const;
    Protocol protocolType() const;
    QString location() const;
    QString displayName() const;
    QString fullAddress() const;
    QString protocolDisplayName() const;

    int latency() const;
    QString latencyText() const;
    QString latencyLevel() const;
    bool isFavorite() const;
    bool isAvailable() const;
    bool isTestingSpeed() const;
    QDateTime lastTested() const;
    QString lastTestedText() const;

    QString uuid() const;
    int alterId() const;
    QString security() const;
    QString password() const;
    QString method() const;
    QJsonObject settings() const;
    QVariant setting(const QString& key, const QVariant& defaultValue = QVariant()) const;

    QString network() const;
    NetworkType networkType() const;
    QString path() const;
    QString host() const;
    QVariantMap headers() const;
    QString headerType() const;
    QJsonObject streamSettings() const;

    bool isTLSEnabled() const;
    SecurityType securityType() const;
    QString tlsServerName() const;
    bool allowInsecure() const;
    QString alpn() const;
    QString fingerprint() const;
    QJsonObject tlsSettings() const;

    // Reality getters
    QString realityPublicKey() const;
    QString realityShortId() const;
    QString realitySpiderX() const;

    // XTLS getters
    QString flow() const;

    // Hysteria/Hysteria2 getters
    QString hysteriaProtocol() const;      // 协议版本 (udp, wechat-video等)
    QString hysteriaObfs() const;          // 混淆密码
    QString hysteriaObfsType() const;      // 混淆类型 (salamander等)
    QString hysteriaAuth() const;          // 认证字符串
    int hysteriaUpMbps() const;            // 上行带宽限制 (Mbps)
    int hysteriaDownMbps() const;          // 下行带宽限制 (Mbps)
    QString hysteriaRecvWindow() const;    // 接收窗口
    QString hysteriaRecvWindowConn() const;// 连接接收窗口
    bool hysteriaDisableMtuDiscovery() const; // 禁用MTU发现

    // TUIC getters
    QString tuicUuid() const;              // TUIC UUID
    QString tuicToken() const;             // TUIC Token (v4)
    QString tuicCongestionControl() const; // 拥塞控制算法 (bbr, cubic等)
    QString tuicUdpRelayMode() const;      // UDP中继模式 (native, quic)
    bool tuicReduceRtt() const;            // 减少RTT (0-RTT)
    int tuicHeartbeatInterval() const;     // 心跳间隔 (ms)

    // WireGuard getters
    QString wgPrivateKey() const;          // 私钥
    QString wgPublicKey() const;           // 服务器公钥
    QString wgPresharedKey() const;        // 预共享密钥
    QString wgLocalAddress() const;        // 本地地址
    int wgMtu() const;                     // MTU
    QString wgReserved() const;            // 保留字节

    // 通用字段
    bool udpEnabled() const;               // UDP支持
    QString plugin() const;                // 插件名称 (obfs等)
    QString pluginOpts() const;            // 插件选项

    QString subscriptionId() const;
    QString subscriptionUrl() const;
    QString remarks() const;
    QStringList tags() const;

    QString countryCode() const;
    QString continent() const;
    QString countryFlag() const;
    double serverLoad() const;
    QString bandwidth() const;
    bool isPro() const;

    qint64 totalUpload() const;
    qint64 totalDownload() const;
    qint64 totalTraffic() const;
    QString totalUploadText() const;
    QString totalDownloadText() const;
    QString totalTrafficText() const;
    int connectCount() const;
    QDateTime lastConnected() const;
    QString lastConnectedText() const;

    quint64 peakUploadSpeed() const;
    quint64 peakDownloadSpeed() const;
    quint64 averageUploadSpeed() const;
    quint64 averageDownloadSpeed() const;
    QString peakUploadSpeedText() const;
    QString peakDownloadSpeedText() const;
    QString averageUploadSpeedText() const;
    QString averageDownloadSpeedText() const;

    QDateTime createdAt() const;
    QDateTime updatedAt() const;

    // Setters
    void setName(const QString& name);
    void setAddress(const QString& address);
    void setResolvedIP(const QString& ip);
    void setPort(int port);
    void setProtocol(const QString& protocol);
    void setLocation(const QString& location);

    void setLatency(int latency);
    void setIsFavorite(bool favorite);
    void setIsAvailable(bool available);
    void setIsTestingSpeed(bool testing);
    void setLastTested(const QDateTime& dateTime);

    void setUuid(const QString& uuid);
    void setAlterId(int alterId);
    void setSecurity(const QString& security);
    void setPassword(const QString& password);
    void setMethod(const QString& method);
    void setSettings(const QJsonObject& settings);
    void setSetting(const QString& key, const QVariant& value);

    void setNetwork(const QString& network);
    void setPath(const QString& path);
    void setHost(const QString& host);
    void setHeaders(const QVariantMap& headers);
    void setHeaderType(const QString& type);
    void setStreamSettings(const QJsonObject& streamSettings);

    void setTLSEnabled(bool enabled);
    void setSecurityType(SecurityType type);
    void setTlsServerName(const QString& serverName);
    void setAllowInsecure(bool allow);
    void setAlpn(const QString& alpn);
    void setFingerprint(const QString& fingerprint);
    void setTlsSettings(const QJsonObject& tlsSettings);

    // Reality setters
    void setRealityPublicKey(const QString& publicKey);
    void setRealityShortId(const QString& shortId);
    void setRealitySpiderX(const QString& spiderX);

    // XTLS setters
    void setFlow(const QString& flow);

    // Hysteria/Hysteria2 setters
    void setHysteriaProtocol(const QString& protocol);
    void setHysteriaObfs(const QString& obfs);
    void setHysteriaObfsType(const QString& obfsType);
    void setHysteriaAuth(const QString& auth);
    void setHysteriaUpMbps(int upMbps);
    void setHysteriaDownMbps(int downMbps);
    void setHysteriaRecvWindow(const QString& recvWindow);
    void setHysteriaRecvWindowConn(const QString& recvWindowConn);
    void setHysteriaDisableMtuDiscovery(bool disable);

    // TUIC setters
    void setTuicUuid(const QString& uuid);
    void setTuicToken(const QString& token);
    void setTuicCongestionControl(const QString& cc);
    void setTuicUdpRelayMode(const QString& mode);
    void setTuicReduceRtt(bool reduce);
    void setTuicHeartbeatInterval(int interval);

    // WireGuard setters
    void setWgPrivateKey(const QString& key);
    void setWgPublicKey(const QString& key);
    void setWgPresharedKey(const QString& key);
    void setWgLocalAddress(const QString& address);
    void setWgMtu(int mtu);
    void setWgReserved(const QString& reserved);

    // 通用字段 setters
    void setUdpEnabled(bool enabled);
    void setPlugin(const QString& plugin);
    void setPluginOpts(const QString& opts);

    void setSubscriptionId(const QString& subscriptionId);
    void setSubscriptionUrl(const QString& url);
    void setRemarks(const QString& remarks);
    void setTags(const QStringList& tags);
    void addTag(const QString& tag);
    void removeTag(const QString& tag);

    void setCountryCode(const QString& code);
    void setServerLoad(double load);
    void setBandwidth(const QString& bandwidth);
    void setIsPro(bool isPro);

    void setTotalUpload(qint64 bytes);
    void setTotalDownload(qint64 bytes);
    void setLastConnected(const QDateTime& dateTime);
    void setCreatedAt(const QDateTime& dateTime);
    void setUpdatedAt(const QDateTime& dateTime);

    void setPeakUploadSpeed(quint64 speed);
    void setPeakDownloadSpeed(quint64 speed);
    void setAverageUploadSpeed(quint64 speed);
    void setAverageDownloadSpeed(quint64 speed);

    // 统计方法
    Q_INVOKABLE void addUpload(qint64 bytes);
    Q_INVOKABLE void addDownload(qint64 bytes);
    Q_INVOKABLE void incrementConnectCount();
    Q_INVOKABLE void resetStats();
    Q_INVOKABLE void updateLastTested();
    Q_INVOKABLE void updateLastConnected();

    // 序列化和解析
    QJsonObject toJson() const;
    static Server* fromJson(const QJsonObject& json, QObject* parent = nullptr);
    QJsonObject toXrayOutbound() const;
    static Server* fromShareLink(const QString& link, QObject* parent = nullptr);
    Q_INVOKABLE QString toShareLink() const;
    static Server* parse(const QString& content, QObject* parent = nullptr);
    Server* clone(QObject* parent = nullptr) const;

    // 验证和比较
    Q_INVOKABLE bool isValid() const;
    Q_INVOKABLE QString validationError() const;
    bool isEqual(const Server* other) const;
    QString configHash() const;

    // DNS解析
    Q_INVOKABLE void resolveAddress();

    // UI 相关
    QString iconName() const;
    QString locationFlag() const;
    int score() const;

    // 静态辅助方法
    static Protocol protocolFromString(const QString& protocol);
    static QString protocolToString(Protocol protocol);
    static NetworkType networkTypeFromString(const QString& network);
    static QString networkTypeToString(NetworkType type);
    static QString formatBytes(qint64 bytes);
    static QString formatTimeAgo(const QDateTime& dateTime);

signals:
    void nameChanged();
    void addressChanged();
    void resolvedIPChanged();
    void portChanged();
    void protocolChanged();
    void locationChanged();
    void latencyChanged();
    void isFavoriteChanged();
    void isAvailableChanged();
    void isTestingSpeedChanged();
    void lastTestedChanged();
    void statsChanged();
    void speedStatsChanged();
    void settingsChanged();
    void streamSettingsChanged();
    void tlsSettingsChanged();
    void tagsChanged();

private:
    friend class DatabaseManager;
    friend class SubscriptionManager;  // 允许SubscriptionManager访问私有方法
    void setConnectCount(int count);
    void setId(const QString& id) { m_id = id; }  // 私有方法：设置ID

    static QString generateId();
    static QString generateIdFromProperties(const QString& protocol, const QString& address, int port);
    static Server* parseVMessLink(const QString& link, QObject* parent);
    static Server* parseVLESSLink(const QString& link, QObject* parent);
    static Server* parseTrojanLink(const QString& link, QObject* parent);
    static Server* parseShadowsocksLink(const QString& link, QObject* parent);
    static Server* parseHysteriaLink(const QString& link, QObject* parent);
    static Server* parseHysteria2Link(const QString& link, QObject* parent);
    static Server* parseTuicLink(const QString& link, QObject* parent);
    static Server* parseWireGuardLink(const QString& link, QObject* parent);
    static QString urlDecode(const QString& encoded);
    static QString urlEncode(const QString& data);

    QString m_id;
    QString m_name;
    QString m_address;
    QString m_resolvedIP;
    int m_port;
    QString m_protocol;
    QString m_location;

    int m_latency;
    bool m_isFavorite;
    bool m_isAvailable;
    bool m_isTestingSpeed;
    QDateTime m_lastTested;

    // 协议特定参数
    QString m_uuid;           // VMess/VLESS UUID
    int m_alterId;            // VMess alterId
    QString m_security;       // 加密方式
    QString m_password;       // 密码（Shadowsocks/Trojan）
    QString m_method;         // 加密方法（Shadowsocks）

    // 传输层参数
    QString m_network;        // 传输协议
    QString m_path;           // WebSocket/HTTP2路径
    QString m_host;           // Host header
    QString m_headerType;     // 伪装类型
    QVariantMap m_headers;    // 自定义headers

    // TLS参数
    bool m_tlsEnabled;        // 是否启用TLS
    QString m_tlsServerName;  // TLS SNI
    bool m_allowInsecure;     // 允许不安全证书
    QString m_alpn;           // ALPN
    QString m_fingerprint;    // 指纹

    // Reality参数
    QString m_realityPublicKey;  // Reality公钥
    QString m_realityShortId;    // Reality短ID
    QString m_realitySpiderX;    // Reality Spider-X (spx)

    // XTLS参数
    QString m_flow;              // XTLS流控模式 (xtls-rprx-vision等)

    // Hysteria/Hysteria2 参数
    QString m_hysteriaProtocol;      // 协议版本
    QString m_hysteriaObfs;          // 混淆密码
    QString m_hysteriaObfsType;      // 混淆类型
    QString m_hysteriaAuth;          // 认证字符串
    int m_hysteriaUpMbps;            // 上行带宽限制
    int m_hysteriaDownMbps;          // 下行带宽限制
    QString m_hysteriaRecvWindow;    // 接收窗口
    QString m_hysteriaRecvWindowConn;// 连接接收窗口
    bool m_hysteriaDisableMtuDiscovery; // 禁用MTU发现

    // TUIC 参数
    QString m_tuicUuid;              // TUIC UUID
    QString m_tuicToken;             // TUIC Token
    QString m_tuicCongestionControl; // 拥塞控制
    QString m_tuicUdpRelayMode;      // UDP中继模式
    bool m_tuicReduceRtt;            // 减少RTT
    int m_tuicHeartbeatInterval;     // 心跳间隔

    // WireGuard 参数
    QString m_wgPrivateKey;          // 私钥
    QString m_wgPublicKey;           // 服务器公钥
    QString m_wgPresharedKey;        // 预共享密钥
    QString m_wgLocalAddress;        // 本地地址
    int m_wgMtu;                     // MTU
    QString m_wgReserved;            // 保留字节

    // 通用参数
    bool m_udpEnabled;               // UDP支持
    QString m_plugin;                // 插件名称
    QString m_pluginOpts;            // 插件选项

    QJsonObject m_settings;
    QJsonObject m_streamSettings;
    QJsonObject m_tlsSettings;

    QString m_subscriptionId;
    QString m_subscriptionUrl;
    QString m_remarks;
    QStringList m_tags;

    // 服务器元数据
    QString m_countryCode;    // 国家代码
    double m_serverLoad;      // 服务器负载
    QString m_bandwidth;      // 带宽
    bool m_isPro;             // 是否为Pro服务器

    qint64 m_totalUpload;
    qint64 m_totalDownload;
    int m_connectCount;
    QDateTime m_lastConnected;

    // 速度统计
    quint64 m_peakUploadSpeed;
    quint64 m_peakDownloadSpeed;
    quint64 m_averageUploadSpeed;
    quint64 m_averageDownloadSpeed;

    QDateTime m_createdAt;
    QDateTime m_updatedAt;
};

#endif // SERVER_H
