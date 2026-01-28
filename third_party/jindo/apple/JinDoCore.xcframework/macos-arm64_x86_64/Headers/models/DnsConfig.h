/**
 * @file DnsConfig.h
 * @brief Clash DNS 配置数据模型
 * @details 定义 Clash 配置中的 dns 结构，支持 DoH、DoT、DNS 分流等
 * @author JinDo Core Team
 * @date 2025
 */

#ifndef DNSCONFIG_H
#define DNSCONFIG_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>

/**
 * @class DnsConfig
 * @brief Clash DNS 配置数据模型
 *
 * 支持的 DNS 功能：
 * - 基础 DNS 服务器 (nameserver)
 * - 回退 DNS 服务器 (fallback)
 * - DNS-over-HTTPS (DoH)
 * - DNS-over-TLS (DoT)
 * - 回退过滤器 (fallback-filter)
 * - 域名到 DNS 服务器映射 (nameserver-policy)
 * - Fake-IP 模式
 */
class DnsConfig : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool ipv6 READ ipv6 WRITE setIpv6 NOTIFY ipv6Changed)
    Q_PROPERTY(QString listen READ listen WRITE setListen NOTIFY listenChanged)
    Q_PROPERTY(bool enhancedMode READ enhancedMode WRITE setEnhancedMode NOTIFY enhancedModeChanged)
    Q_PROPERTY(QString fakeIpRange READ fakeIpRange WRITE setFakeIpRange NOTIFY fakeIpRangeChanged)
    Q_PROPERTY(QStringList fakeIpFilter READ fakeIpFilter WRITE setFakeIpFilter NOTIFY fakeIpFilterChanged)
    Q_PROPERTY(QStringList nameservers READ nameservers WRITE setNameservers NOTIFY nameserversChanged)
    Q_PROPERTY(QStringList fallback READ fallback WRITE setFallback NOTIFY fallbackChanged)
    Q_PROPERTY(QString subscriptionId READ subscriptionId WRITE setSubscriptionId NOTIFY subscriptionIdChanged)

public:
    explicit DnsConfig(QObject* parent = nullptr);
    explicit DnsConfig(const QString& id, QObject* parent = nullptr);
    ~DnsConfig() override;

    // Getters
    QString id() const;
    bool enabled() const;
    bool ipv6() const;
    QString listen() const;
    bool enhancedMode() const;
    QString fakeIpRange() const;
    QStringList fakeIpFilter() const;
    QStringList nameservers() const;
    QStringList fallback() const;
    QString subscriptionId() const;

    // Fallback filter getters
    bool fallbackFilterGeoIp() const;
    QString fallbackFilterGeoIpCode() const;
    QStringList fallbackFilterIpCidr() const;
    QStringList fallbackFilterDomain() const;

    // Nameserver policy getter
    QMap<QString, QString> nameserverPolicy() const;

    // Setters
    void setEnabled(bool enabled);
    void setIpv6(bool ipv6);
    void setListen(const QString& listen);
    void setEnhancedMode(bool enhanced);
    void setFakeIpRange(const QString& range);
    void setFakeIpFilter(const QStringList& filter);
    void setNameservers(const QStringList& servers);
    void setFallback(const QStringList& servers);
    void setSubscriptionId(const QString& id);

    // Fallback filter setters
    void setFallbackFilterGeoIp(bool enabled);
    void setFallbackFilterGeoIpCode(const QString& code);
    void setFallbackFilterIpCidr(const QStringList& cidrs);
    void setFallbackFilterDomain(const QStringList& domains);

    // Nameserver policy setter
    void setNameserverPolicy(const QMap<QString, QString>& policy);

    // 序列化
    QJsonObject toJson() const;
    static DnsConfig* fromJson(const QJsonObject& json, QObject* parent = nullptr);

    // 从 Clash DNS 配置解析
    static DnsConfig* fromClashConfig(const QJsonObject& config, QObject* parent = nullptr);

signals:
    void enabledChanged();
    void ipv6Changed();
    void listenChanged();
    void enhancedModeChanged();
    void fakeIpRangeChanged();
    void fakeIpFilterChanged();
    void nameserversChanged();
    void fallbackChanged();
    void subscriptionIdChanged();
    void fallbackFilterChanged();
    void nameserverPolicyChanged();

private:
    QString m_id;
    bool m_enabled;                      // 是否启用 DNS
    bool m_ipv6;                         // 是否启用 IPv6
    QString m_listen;                    // 监听地址 (如: 0.0.0.0:53)
    bool m_enhancedMode;                 // 增强模式 (fake-ip / redir-host)
    QString m_fakeIpRange;               // Fake-IP 范围 (如: 198.18.0.1/16)
    QStringList m_fakeIpFilter;          // Fake-IP 过滤域名列表
    QStringList m_nameservers;           // 主 DNS 服务器列表
    QStringList m_fallback;              // 回退 DNS 服务器列表
    QString m_subscriptionId;            // 所属订阅 ID

    // Fallback filter
    bool m_fallbackFilterGeoIp;          // 是否根据 GeoIP 判断
    QString m_fallbackFilterGeoIpCode;   // GeoIP 国家代码 (如: CN)
    QStringList m_fallbackFilterIpCidr;  // IP CIDR 过滤列表
    QStringList m_fallbackFilterDomain;  // 域名过滤列表

    // Nameserver policy (域名 -> DNS 服务器映射)
    QMap<QString, QString> m_nameserverPolicy;
};

#endif // DNSCONFIG_H
