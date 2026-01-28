/**
 * @file ProxyGroup.h
 * @brief Clash 代理组数据模型
 * @details 定义 Clash 配置中的 proxy-groups 结构，支持选择、自动测速、负载均衡等策略
 * @author JinDo Core Team
 * @date 2025
 */

#ifndef PROXYGROUP_H
#define PROXYGROUP_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>

/**
 * @class ProxyGroup
 * @brief Clash 代理组数据模型
 *
 * 支持的代理组类型：
 * - select: 手动选择
 * - url-test: 自动测速选择最快节点
 * - fallback: 故障转移，按顺序选择可用节点
 * - load-balance: 负载均衡
 * - relay: 链式代理
 */
class ProxyGroup : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString type READ type WRITE setType NOTIFY typeChanged)
    Q_PROPERTY(QStringList proxies READ proxies WRITE setProxies NOTIFY proxiesChanged)
    Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(int interval READ interval WRITE setInterval NOTIFY intervalChanged)
    Q_PROPERTY(int tolerance READ tolerance WRITE setTolerance NOTIFY toleranceChanged)
    Q_PROPERTY(bool lazy READ lazy WRITE setLazy NOTIFY lazyChanged)
    Q_PROPERTY(QString strategy READ strategy WRITE setStrategy NOTIFY strategyChanged)
    Q_PROPERTY(QString subscriptionId READ subscriptionId WRITE setSubscriptionId NOTIFY subscriptionIdChanged)
    Q_PROPERTY(QString currentProxy READ currentProxy WRITE setCurrentProxy NOTIFY currentProxyChanged)

public:
    /**
     * @brief 代理组类型枚举
     */
    enum GroupType {
        Select,         ///< 手动选择
        UrlTest,        ///< 自动测速
        Fallback,       ///< 故障转移
        LoadBalance,    ///< 负载均衡
        Relay           ///< 链式代理
    };
    Q_ENUM(GroupType)

    explicit ProxyGroup(QObject* parent = nullptr);
    explicit ProxyGroup(const QString& id, QObject* parent = nullptr);
    ~ProxyGroup() override;

    // Getters
    QString id() const;
    QString name() const;
    QString type() const;
    GroupType groupType() const;
    QStringList proxies() const;
    QString url() const;
    int interval() const;
    int tolerance() const;
    bool lazy() const;
    QString strategy() const;
    QString subscriptionId() const;
    QString currentProxy() const;

    // Setters
    void setName(const QString& name);
    void setType(const QString& type);
    void setProxies(const QStringList& proxies);
    void setUrl(const QString& url);
    void setInterval(int interval);
    void setTolerance(int tolerance);
    void setLazy(bool lazy);
    void setStrategy(const QString& strategy);
    void setSubscriptionId(const QString& id);
    void setCurrentProxy(const QString& proxy);

    // 序列化
    QJsonObject toJson() const;
    static ProxyGroup* fromJson(const QJsonObject& json, QObject* parent = nullptr);

    // 从 Clash YAML 解析
    static ProxyGroup* fromClashConfig(const QJsonObject& config, QObject* parent = nullptr);

signals:
    void nameChanged();
    void typeChanged();
    void proxiesChanged();
    void urlChanged();
    void intervalChanged();
    void toleranceChanged();
    void lazyChanged();
    void strategyChanged();
    void subscriptionIdChanged();
    void currentProxyChanged();

private:
    QString m_id;
    QString m_name;
    QString m_type;              // select, url-test, fallback, load-balance, relay
    QStringList m_proxies;       // 包含的代理名称列表
    QString m_url;               // 测速 URL (用于 url-test/fallback)
    int m_interval;              // 测速间隔(秒)
    int m_tolerance;             // 延迟容差(ms)
    bool m_lazy;                 // 是否延迟测速
    QString m_strategy;          // 负载均衡策略: consistent-hashing, round-robin
    QString m_subscriptionId;    // 所属订阅 ID
    QString m_currentProxy;      // 当前选中的代理
};

#endif // PROXYGROUP_H
