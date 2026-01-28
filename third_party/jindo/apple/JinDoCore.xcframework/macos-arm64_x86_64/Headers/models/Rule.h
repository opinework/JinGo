/**
 * @file Rule.h
 * @brief Clash 路由规则数据模型
 * @details 定义 Clash 配置中的 rules 结构，支持域名、IP、地理位置等多种匹配规则
 * @author JinDo Core Team
 * @date 2025
 */

#ifndef RULE_H
#define RULE_H

#include <QObject>
#include <QString>
#include <QJsonObject>

/**
 * @class Rule
 * @brief Clash 路由规则数据模型
 *
 * 支持的规则类型：
 * - DOMAIN: 完整域名匹配
 * - DOMAIN-SUFFIX: 域名后缀匹配
 * - DOMAIN-KEYWORD: 域名关键词匹配
 * - DOMAIN-REGEX: 域名正则匹配
 * - IP-CIDR: IPv4 CIDR 匹配
 * - IP-CIDR6: IPv6 CIDR 匹配
 * - SRC-IP-CIDR: 源 IP CIDR 匹配
 * - SRC-PORT: 源端口匹配
 * - DST-PORT: 目标端口匹配
 * - GEOIP: 地理位置 IP 匹配
 * - GEOSITE: 地理位置域名匹配
 * - PROCESS-NAME: 进程名匹配
 * - RULE-SET: 规则集引用
 * - MATCH: 默认匹配（通常放最后）
 */
class Rule : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString type READ type WRITE setType NOTIFY typeChanged)
    Q_PROPERTY(QString payload READ payload WRITE setPayload NOTIFY payloadChanged)
    Q_PROPERTY(QString target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(bool noResolve READ noResolve WRITE setNoResolve NOTIFY noResolveChanged)
    Q_PROPERTY(int order READ order WRITE setOrder NOTIFY orderChanged)
    Q_PROPERTY(QString subscriptionId READ subscriptionId WRITE setSubscriptionId NOTIFY subscriptionIdChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

public:
    /**
     * @brief 规则类型枚举
     */
    enum RuleType {
        Domain,             ///< DOMAIN
        DomainSuffix,       ///< DOMAIN-SUFFIX
        DomainKeyword,      ///< DOMAIN-KEYWORD
        DomainRegex,        ///< DOMAIN-REGEX
        IpCidr,             ///< IP-CIDR
        IpCidr6,            ///< IP-CIDR6
        SrcIpCidr,          ///< SRC-IP-CIDR
        SrcPort,            ///< SRC-PORT
        DstPort,            ///< DST-PORT
        GeoIp,              ///< GEOIP
        GeoSite,            ///< GEOSITE
        ProcessName,        ///< PROCESS-NAME
        RuleSet,            ///< RULE-SET
        Match               ///< MATCH (default)
    };
    Q_ENUM(RuleType)

    explicit Rule(QObject* parent = nullptr);
    explicit Rule(const QString& id, QObject* parent = nullptr);
    ~Rule() override;

    // Getters
    QString id() const;
    QString type() const;
    RuleType ruleType() const;
    QString payload() const;
    QString target() const;
    bool noResolve() const;
    int order() const;
    QString subscriptionId() const;
    bool enabled() const;

    // Setters
    void setType(const QString& type);
    void setPayload(const QString& payload);
    void setTarget(const QString& target);
    void setNoResolve(bool noResolve);
    void setOrder(int order);
    void setSubscriptionId(const QString& id);
    void setEnabled(bool enabled);

    // 序列化
    QJsonObject toJson() const;
    static Rule* fromJson(const QJsonObject& json, QObject* parent = nullptr);

    // 从 Clash 规则字符串解析 (如: "DOMAIN-SUFFIX,google.com,Proxy")
    static Rule* fromClashRule(const QString& ruleStr, int order, QObject* parent = nullptr);

    // 从 sing-box JSON 规则对象解析
    static QList<Rule*> fromSingboxRule(const QJsonObject& ruleObj, int& order, QObject* parent = nullptr);

    // 转换为 Clash 规则字符串
    QString toClashRule() const;

    // 转换为 Xray 路由规则 JSON 对象
    // @param proxyTag 代理出站标签 (默认 "proxy")
    // @param directTag 直连出站标签 (默认 "direct")
    // @param blockTag 阻止出站标签 (默认 "block")
    QJsonObject toXrayRule(const QString& proxyTag = "proxy",
                           const QString& directTag = "direct",
                           const QString& blockTag = "block") const;

    // 批量转换为 Xray 路由规则数组
    static QJsonArray toXrayRules(const QList<Rule*>& rules,
                                  const QString& proxyTag = "proxy",
                                  const QString& directTag = "direct",
                                  const QString& blockTag = "block");

    // 规则类型字符串转换
    static QString ruleTypeToString(RuleType type);
    static RuleType stringToRuleType(const QString& str);

signals:
    void typeChanged();
    void payloadChanged();
    void targetChanged();
    void noResolveChanged();
    void orderChanged();
    void subscriptionIdChanged();
    void enabledChanged();

private:
    QString m_id;
    QString m_type;              // 规则类型字符串
    QString m_payload;           // 匹配内容 (域名/IP/端口等)
    QString m_target;            // 目标代理组或 DIRECT/REJECT
    bool m_noResolve;            // 是否不解析域名 (用于 IP 规则)
    int m_order;                 // 规则顺序 (越小优先级越高)
    QString m_subscriptionId;    // 所属订阅 ID
    bool m_enabled;              // 是否启用
};

#endif // RULE_H
