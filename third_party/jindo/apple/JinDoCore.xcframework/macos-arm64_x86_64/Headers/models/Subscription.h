/**
 * @file Subscription.h
 * @brief 订阅数据模型头文件
 * @details 定义订阅信息模型，包括服务器管理、更新控制、流量统计、认证信息、过滤分组等功能
 * @author JinGo VPN Team
 * @date 2025
 */

#ifndef SUBSCRIPTION_H
#define SUBSCRIPTION_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QUrl>

// FIX: 包含 Server 的完整定义
#include "Server.h"

// 前向声明 Clash 配置相关类
class ProxyGroup;
class Rule;
class DnsConfig;

/**
 * @class Subscription
 * @brief 订阅数据模型
 *
 * @details
 * 该类封装了订阅的完整信息，包括：
 * - 基本属性：ID、名称、URL、启用状态、类型、更新状态
 * - 服务器管理：服务器列表的增删改查
 * - 更新控制：自动更新、更新间隔、最后更新时间
 * - 流量统计：总流量、已用流量、剩余流量
 * - 认证信息：用户名、密码、令牌、自定义请求头
 * - 过滤分组：分组名、标签、包含/排除关键词
 * - 其他设置：User-Agent、超时、SSL 验证
 *
 * 该类继承自 QObject，支持信号/槽机制，并暴露 Q_PROPERTY 供 QML 使用
 *
 * @note
 * - 线程安全性：该类不是线程安全的，应在单一线程中使用
 * - 服务器对象的所有权由订阅管理
 *
 * 使用示例：
 * @code
 * Subscription* sub = new Subscription(this);
 * sub->setName("我的订阅");
 * sub->setUrl("https://example.com/subscribe");
 * sub->setUpdateInterval(60); // 60分钟自动更新
 * sub->setAutoUpdate(true);
 *
 * // 添加服务器
 * Server* server = new Server(this);
 * sub->addServer(server);
 *
 * // 序列化
 * QJsonObject json = sub->toJson();
 *
 * // 反序列化
 * Subscription* loadedSub = Subscription::fromJson(json);
 * @endcode
 */
class Subscription : public QObject
{
    Q_OBJECT

    /// 订阅唯一标识符（只读）
    Q_PROPERTY(QString id READ id CONSTANT)

    /// 订阅名称（可读写，变化时发出 nameChanged 信号）
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

    /// 订阅 URL（可读写，变化时发出 urlChanged 信号）
    Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY urlChanged)

    /// 是否启用（可读写，变化时发出 isEnabledChanged 信号）
    Q_PROPERTY(bool isEnabled READ isEnabled WRITE setIsEnabled NOTIFY isEnabledChanged)

    /// 服务器数量（只读，变化时发出 serverCountChanged 信号）
    Q_PROPERTY(int serverCount READ serverCount NOTIFY serverCountChanged)

    /// 最后更新时间（只读，变化时发出 lastUpdatedChanged 信号）
    Q_PROPERTY(QDateTime lastUpdated READ lastUpdated NOTIFY lastUpdatedChanged)

    // FIX: 将 READ formattedTraffic 改为 READ formatTraffic
    /// 格式化的流量信息（只读，流量变化时发出 trafficChanged 信号）
    Q_PROPERTY(QString formattedTraffic READ formatTraffic NOTIFY trafficChanged)

    /// 流量使用百分比（只读，流量变化时发出 trafficChanged 信号）
    Q_PROPERTY(int trafficUsagePercent READ trafficUsagePercent NOTIFY trafficChanged)

    /// Clash 代理组数量（只读）
    Q_PROPERTY(int proxyGroupCount READ proxyGroupCount NOTIFY proxyGroupsChanged)

    /// Clash 路由规则数量（只读）
    Q_PROPERTY(int ruleCount READ ruleCount NOTIFY rulesChanged)

    /// 是否有 DNS 配置（只读）
    Q_PROPERTY(bool hasDnsConfig READ hasDnsConfig NOTIFY dnsConfigChanged)

public:
    /**
     * @enum SubscriptionType
     * @brief 订阅类型枚举
     *
     * @details
     * 定义支持的订阅格式类型：
     * - Standard: 标准 Base64 编码的节点列表（最常见）
     * - SIP008: Shadowsocks SIP008 JSON 格式
     * - V2rayN: V2rayN 客户端格式
     * - Clash: Clash 代理工具的 YAML 格式
     * - SingBox: sing-box JSON 格式（outbounds 数组）
     * - Surfboard: Surfboard 客户端格式
     * - Custom: 自定义格式
     */
    enum SubscriptionType {
        Standard,       ///< 标准订阅 (Base64 编码的节点列表)
        SIP008,         ///< Shadowsocks SIP008 JSON 格式
        V2rayN,         ///< V2rayN 格式
        Clash,          ///< Clash YAML 格式
        SingBox,        ///< sing-box JSON 格式
        Surfboard,      ///< Surfboard 格式
        Custom          ///< 自定义格式
    };
    Q_ENUM(SubscriptionType)

    /**
     * @enum UpdateStatus
     * @brief 更新状态枚举
     *
     * @details
     * 表示订阅当前的更新状态：
     * - Idle: 空闲状态，未在更新
     * - Updating: 正在更新中
     * - Success: 最后一次更新成功
     * - Failed: 最后一次更新失败
     */
    enum UpdateStatus {
        Idle,           ///< 空闲
        Updating,       ///< 更新中
        Success,        ///< 更新成功
        Failed          ///< 更新失败
    };
    Q_ENUM(UpdateStatus)

    /**
     * @brief 默认构造函数
     * @param parent 父对象指针
     *
     * @details
     * 创建新订阅时会自动：
     * - 生成唯一 ID
     * - 设置启用状态为 true
     * - 设置类型为 Standard
     * - 设置更新状态为 Idle
     * - 设置更新间隔为 60 分钟
     * - 启用自动更新
     * - 设置创建时间为当前时间
     * - 设置超时为 30 秒
     * - 启用 SSL 验证
     */
    explicit Subscription(QObject* parent = nullptr);

    /**
     * @brief 带 ID 的构造函数
     * @param id 订阅 ID（可选，为空则自动生成）
     * @param parent 父对象指针
     *
     * @details
     * 与默认构造函数类似，但允许指定自定义 ID
     * 通常用于从数据库加载订阅时
     *
     * @note 如果 id 参数为空，会自动生成新的 UUID
     */
    explicit Subscription(const QString& id, QObject* parent = nullptr);

    /**
     * @brief 析构函数
     * @details 会自动删除所有关联的服务器对象
     */
    ~Subscription() override;

    // ========== 基本属性 ==========

    /**
     * @brief 获取订阅 ID
     * @return QString 订阅唯一标识符（UUID 格式，无大括号）
     */
    QString id() const;

    /**
     * @brief 获取订阅名称
     * @return QString 订阅名称
     */
    QString name() const;

    /**
     * @brief 设置订阅名称
     * @param name 新的订阅名称
     * @details 如果名称发生变化，会发出 nameChanged() 信号
     */
    void setName(const QString& name);

    /**
     * @brief 获取订阅 URL
     * @return QString 订阅地址
     */
    QString url() const;

    /**
     * @brief 设置订阅 URL
     * @param url 新的订阅地址
     * @details 如果 URL 发生变化，会发出 urlChanged() 信号
     */
    void setUrl(const QString& url);

    /**
     * @brief 检查订阅是否启用
     * @return bool true 表示启用，false 表示禁用
     */
    bool isEnabled() const;

    /**
     * @brief 设置订阅启用状态
     * @param enabled true 启用，false 禁用
     * @details
     * 如果状态发生变化，会发出 isEnabledChanged() 信号
     * 禁用的订阅不会自动更新，其服务器也不会显示在可用列表中
     */
    void setIsEnabled(bool enabled);

    /**
     * @brief 获取订阅类型
     * @return SubscriptionType 订阅类型枚举值
     */
    SubscriptionType type() const;

    /**
     * @brief 设置订阅类型
     * @param type 订阅类型
     * @note
     * - 不发出信号
     * - 类型通常在解析订阅内容时自动设置
     */
    void setType(SubscriptionType type);

    /**
     * @brief 获取更新状态
     * @return UpdateStatus 更新状态枚举值
     */
    UpdateStatus updateStatus() const;

    /**
     * @brief 设置更新状态
     * @param status 新的更新状态
     * @details 如果状态发生变化，会发出 updateStatusChanged() 信号
     */
    void setUpdateStatus(UpdateStatus status);

    // ========== 服务器管理 ==========

    /**
     * @brief 获取服务器数量
     * @return int 订阅中包含的服务器总数
     */
    int serverCount() const;

    /**
     * @brief 获取所有服务器
     * @return QList<Server*> 服务器对象指针列表
     * @note 返回的指针由订阅管理，调用者不应删除
     */
    QList<Server*> servers() const;

    /**
     * @brief 设置服务器列表
     * @param servers 新的服务器列表
     * @details
     * - 会删除旧的服务器对象
     * - 发出 serverCountChanged() 和 serversChanged() 信号
     * @warning 旧的服务器对象会被删除，确保没有其他引用
     */
    void setServers(const QList<Server*>& servers);

    /**
     * @brief 添加服务器
     * @param server 要添加的服务器对象指针
     * @details
     * - 会设置服务器的父对象为当前订阅
     * - 重复添加会被忽略
     * - 发出 serverCountChanged() 和 serversChanged() 信号
     * @note 服务器对象的所有权转移给订阅
     */
    void addServer(Server* server);

    /**
     * @brief 移除服务器
     * @param server 要移除的服务器对象指针
     * @details
     * - 服务器会被延迟删除
     * - 发出 serverCountChanged() 和 serversChanged() 信号
     */
    void removeServer(Server* server);

    /**
     * @brief 清空所有服务器
     * @details
     * - 删除所有服务器对象
     * - 发出 serverCountChanged() 和 serversChanged() 信号
     */
    void clearServers();

    /**
     * @brief 根据索引获取服务器
     * @param index 服务器索引（从 0 开始）
     * @return Server* 服务器对象指针，索引无效返回 nullptr
     */
    Server* getServer(int index) const;

    /**
     * @brief 根据 ID 查找服务器
     * @param serverId 服务器唯一标识符
     * @return Server* 服务器对象指针，未找到返回 nullptr
     */
    Server* findServer(const QString& serverId) const;

    // ========== 更新信息 ==========

    /**
     * @brief 获取最后更新时间
     * @return QDateTime 最后一次成功更新的时间
     * @note 如果返回无效的 QDateTime，表示从未更新过
     */
    QDateTime lastUpdated() const;

    /**
     * @brief 设置最后更新时间
     * @param dateTime 更新时间
     * @details 如果时间发生变化，会发出 lastUpdatedChanged() 信号
     */
    void setLastUpdated(const QDateTime& dateTime);

    /**
     * @brief 获取创建时间
     * @return QDateTime 订阅创建的时间
     */
    QDateTime createdAt() const;

    /**
     * @brief 设置创建时间
     * @param dateTime 创建时间
     * @note 不发出信号，通常用于从数据库加载时
     */
    void setCreatedAt(const QDateTime& dateTime);

    /**
     * @brief 获取下次更新时间
     * @return QDateTime 计算出的下次更新时间
     * @details
     * 计算公式：最后更新时间 + 更新间隔
     * 如果最后更新时间无效或更新间隔 <= 0，返回无效的 QDateTime
     */
    QDateTime nextUpdateTime() const;

    /**
     * @brief 获取更新间隔
     * @return int 更新间隔（分钟）
     */
    int updateInterval() const;  // 分钟

    /**
     * @brief 设置更新间隔
     * @param minutes 更新间隔（分钟）
     * @note 不发出信号
     */
    void setUpdateInterval(int minutes);

    /**
     * @brief 检查是否启用自动更新
     * @return bool true 表示启用，false 表示禁用
     */
    bool autoUpdate() const;

    /**
     * @brief 设置自动更新状态
     * @param enabled true 启用，false 禁用
     * @note 不发出信号
     */
    void setAutoUpdate(bool enabled);

    /**
     * @brief 获取最后一次更新的错误信息
     * @return QString 错误信息，成功则为空字符串
     */
    QString lastUpdateError() const;

    /**
     * @brief 设置最后一次更新的错误信息
     * @param error 错误信息
     * @note 不发出信号
     */
    void setLastUpdateError(const QString& error);

    // ========== 流量统计 ==========

    /**
     * @brief 获取总流量配额
     * @return qint64 总流量（字节）
     * @note 0 或负数表示无限流量
     */
    qint64 totalTraffic() const;

    /**
     * @brief 设置总流量配额
     * @param bytes 总流量（字节）
     * @details 如果流量发生变化，会发出 trafficChanged() 信号
     */
    void setTotalTraffic(qint64 bytes);

    /**
     * @brief 获取已使用流量
     * @return qint64 已用流量（字节）
     */
    qint64 usedTraffic() const;

    /**
     * @brief 设置已使用流量
     * @param bytes 已用流量（字节）
     * @details 如果流量发生变化，会发出 trafficChanged() 信号
     */
    void setUsedTraffic(qint64 bytes);

    /**
     * @brief 获取剩余流量
     * @return qint64 剩余流量（字节）
     * @details
     * 计算公式：总流量 - 已用流量
     * 如果结果为负数，返回 0
     * @note 这是一个计算属性，不直接存储
     */
    qint64 remainingTraffic() const;

    /**
     * @brief 获取流量重置日期
     * @return QDateTime 流量将被重置的时间
     */
    QDateTime trafficResetDate() const;

    /**
     * @brief 设置流量重置日期
     * @param date 重置时间
     * @note 不发出信号
     */
    void setTrafficResetDate(const QDateTime& date);

    // ========== 认证信息 ==========

    /**
     * @brief 获取用户名
     * @return QString 用户名（用于 HTTP 基本认证）
     */
    QString username() const;

    /**
     * @brief 设置用户名
     * @param username 新的用户名
     * @note 不发出信号
     */
    void setUsername(const QString& username);

    /**
     * @brief 获取密码
     * @return QString 密码（用于 HTTP 基本认证）
     */
    QString password() const;

    /**
     * @brief 设置密码
     * @param password 新的密码
     * @note 不发出信号，密码应加密存储
     */
    void setPassword(const QString& password);

    /**
     * @brief 获取访问令牌
     * @return QString 访问令牌（用于 Bearer 认证）
     */
    QString token() const;

    /**
     * @brief 设置访问令牌
     * @param token 新的访问令牌
     * @note 不发出信号
     */
    void setToken(const QString& token);

    /**
     * @brief 获取自定义 HTTP 请求头
     * @return QMap<QString, QString> 请求头键值对映射
     */
    QMap<QString, QString> customHeaders() const;

    /**
     * @brief 设置自定义 HTTP 请求头
     * @param headers 请求头键值对映射
     * @note 不发出信号
     */
    void setCustomHeaders(const QMap<QString, QString>& headers);

    /**
     * @brief 添加单个自定义请求头
     * @param key 请求头名称
     * @param value 请求头值
     * @note 如果已存在同名请求头，会被覆盖
     */
    void addCustomHeader(const QString& key, const QString& value);

    /**
     * @brief 移除自定义请求头
     * @param key 要移除的请求头名称
     */
    void removeCustomHeader(const QString& key);

    // ========== 过滤和分组 ==========

    /**
     * @brief 获取分组名称
     * @return QString 订阅所属的分组名称
     */
    QString groupName() const;

    /**
     * @brief 设置分组名称
     * @param group 新的分组名称
     * @note 不发出信号，用于将订阅组织到不同的分组中
     */
    void setGroupName(const QString& group);

    /**
     * @brief 获取标签列表
     * @return QStringList 订阅的标签列表
     */
    QStringList tags() const;

    /**
     * @brief 设置标签列表
     * @param tags 新的标签列表
     * @note 不发出信号，标签用于多维度分类
     */
    void setTags(const QStringList& tags);

    /**
     * @brief 添加标签
     * @param tag 要添加的标签
     * @note 如果标签已存在，不会重复添加
     */
    void addTag(const QString& tag);

    /**
     * @brief 移除标签
     * @param tag 要移除的标签
     * @note 会移除所有匹配的标签
     */
    void removeTag(const QString& tag);

    /**
     * @brief 获取包含关键词列表
     * @return QStringList 服务器名称必须包含的关键词
     */
    QStringList includeKeywords() const;

    /**
     * @brief 设置包含关键词列表
     * @param keywords 新的包含关键词列表
     * @note 用于过滤订阅中的服务器，只保留名称包含这些关键词的服务器
     */
    void setIncludeKeywords(const QStringList& keywords);

    /**
     * @brief 获取排除关键词列表
     * @return QStringList 服务器名称不能包含的关键词
     */
    QStringList excludeKeywords() const;

    /**
     * @brief 设置排除关键词列表
     * @param keywords 新的排除关键词列表
     * @note 用于过滤订阅中的服务器，排除名称包含这些关键词的服务器
     */
    void setExcludeKeywords(const QStringList& keywords);

    // ========== 用户代理和其他设置 ==========

    /**
     * @brief 获取用户代理字符串
     * @return QString HTTP User-Agent 请求头的值
     */
    QString userAgent() const;

    /**
     * @brief 设置用户代理字符串
     * @param ua 新的 User-Agent 值
     * @note 不发出信号
     */
    void setUserAgent(const QString& ua);

    /**
     * @brief 获取网络请求超时时间
     * @return int 超时时间（秒）
     */
    int timeout() const;  // 秒

    /**
     * @brief 设置网络请求超时时间
     * @param seconds 超时时间（秒）
     * @note 不发出信号
     */
    void setTimeout(int seconds);

    /**
     * @brief 检查是否验证 SSL 证书
     * @return bool true 表示验证，false 表示不验证
     */
    bool verifySSL() const;

    /**
     * @brief 设置是否验证 SSL 证书
     * @param verify true 验证，false 不验证
     * @note 不发出信号，不验证 SSL 可能存在安全风险
     */
    void setVerifySSL(bool verify);

    // ========== 序列化 ==========

    /**
     * @brief 将订阅信息序列化为 JSON 对象
     * @return QJsonObject 包含订阅信息的 JSON 对象
     * @details
     * 导出的 JSON 字段：
     * - 基本信息：id, name, url, enabled, type
     * - 更新设置：updateInterval, autoUpdate
     * - 流量信息：totalTraffic, usedTraffic
     * - 认证信息：username, password, token
     * - 其他设置：userAgent, timeout, verifySSL
     * - 时间信息：createdAt, lastUpdated
     * @note 不导出服务器列表
     */
    QJsonObject toJson() const;

    /**
     * @brief 从 JSON 对象创建 Subscription 实例
     * @param json JSON 对象，包含订阅信息
     * @param parent 父对象指针
     * @return Subscription* 创建的订阅对象指针
     * @details
     * 支持的 JSON 字段（均为可选）：
     * - 基本信息：id, name, url, enabled, type
     * - 更新设置：updateInterval, autoUpdate
     * - 流量信息：totalTraffic, usedTraffic
     * - 认证信息：username, password, token
     * - 其他设置：userAgent, timeout, verifySSL
     * - 时间信息：createdAt, lastUpdated
     * @note 返回的对象由调用者管理
     */
    static Subscription* fromJson(const QJsonObject& json, QObject* parent = nullptr);

    // ========== 验证 ==========

    /**
     * @brief 检查订阅是否有效
     * @return bool true 表示有效，false 表示无效
     * @details
     * 验证规则：
     * - URL 不能为空
     * - URL 格式必须有效
     */
    bool isValid() const;

    /**
     * @brief 获取验证错误信息
     * @return QString 错误信息，有效则返回空字符串
     */
    QString validationError() const;

    /**
     * @brief 检查 URL 格式是否有效
     * @return bool true 表示有效，false 表示无效
     * @details URL 必须是 http 或 https 协议
     */
    bool isUrlValid() const;

    // ========== Clash 配置 ==========

    /**
     * @brief 获取代理组数量
     * @return int 代理组数量
     */
    int proxyGroupCount() const;

    /**
     * @brief 获取所有代理组
     * @return QList<ProxyGroup*> 代理组列表
     */
    QList<ProxyGroup*> proxyGroups() const;

    /**
     * @brief 设置代理组列表
     * @param groups 新的代理组列表
     */
    void setProxyGroups(const QList<ProxyGroup*>& groups);

    /**
     * @brief 添加代理组
     * @param group 要添加的代理组
     */
    void addProxyGroup(ProxyGroup* group);

    /**
     * @brief 清空代理组
     */
    void clearProxyGroups();

    /**
     * @brief 根据名称查找代理组
     * @param name 代理组名称
     * @return ProxyGroup* 代理组指针，未找到返回 nullptr
     */
    ProxyGroup* findProxyGroup(const QString& name) const;

    /**
     * @brief 获取路由规则数量
     * @return int 路由规则数量
     */
    int ruleCount() const;

    /**
     * @brief 获取所有路由规则
     * @return QList<Rule*> 路由规则列表
     */
    QList<Rule*> rules() const;

    /**
     * @brief 设置路由规则列表
     * @param rules 新的路由规则列表
     */
    void setRules(const QList<Rule*>& rules);

    /**
     * @brief 添加路由规则
     * @param rule 要添加的路由规则
     */
    void addRule(Rule* rule);

    /**
     * @brief 清空路由规则
     */
    void clearRules();

    /**
     * @brief 检查是否有 DNS 配置
     * @return bool 是否有 DNS 配置
     */
    bool hasDnsConfig() const;

    /**
     * @brief 获取 DNS 配置
     * @return DnsConfig* DNS 配置指针，可能为 nullptr
     */
    DnsConfig* dnsConfig() const;

    /**
     * @brief 设置 DNS 配置
     * @param config DNS 配置
     */
    void setDnsConfig(DnsConfig* config);

    /**
     * @brief 清空 DNS 配置
     */
    void clearDnsConfig();

    /**
     * @brief 清空所有 Clash 配置（代理组、规则、DNS）
     */
    void clearClashConfig();

    // ========== 工具方法 ==========

    /**
     * @brief 格式化流量信息显示
     * @return QString 格式化后的流量字符串
     * @details
     * 显示格式：
     * - 无限流量：返回 "无限制"
     * - 有限流量：返回 "已用 / 总量"
     * @note 自动选择合适的单位（B, KB, MB, GB, TB）
     */
    QString formatTraffic() const;

    /**
     * @brief 获取流量使用百分比
     * @return int 使用百分比，范围 0-100
     * @details 计算公式：(已用流量 / 总流量) * 100
     */
    int trafficUsagePercent() const;

    /**
     * @brief 检查是否需要更新
     * @return bool true 表示需要更新，false 表示不需要
     * @details
     * 判断逻辑：
     * - 未启用自动更新或更新间隔 <= 0：返回 false
     * - 从未更新过：返回 true
     * - 下次更新时间已到：返回 true
     */
    bool needsUpdate() const;

    /**
     * @brief 格式化下次更新时间显示
     * @return QString 格式化后的时间字符串
     * @details
     * 显示规则：
     * - 不自动更新：返回 "不自动更新"
     * - 需要更新：返回 "需要更新"
     * - 其他：返回 "X 秒/分钟/小时/天后"
     */
    QString formatNextUpdateTime() const;

    /**
     * @brief 获取距离上次更新的分钟数
     * @return int 分钟数，-1 表示从未更新
     */
    int minutesSinceLastUpdate() const;

    /**
     * @brief 克隆订阅对象
     * @param parent 新对象的父对象指针
     * @return Subscription* 克隆的订阅对象指针
     * @details 克隆所有属性但不包括服务器列表和时间信息
     * @note 返回的对象由调用者管理
     */
    Subscription* clone(QObject* parent = nullptr) const;

signals:
    /**
     * @brief 名称变化信号
     * @details 当调用 setName() 且值发生变化时发出
     */
    void nameChanged();

    /**
     * @brief URL 变化信号
     * @details 当调用 setUrl() 且值发生变化时发出
     */
    void urlChanged();

    /**
     * @brief 启用状态变化信号
     * @details 当调用 setIsEnabled() 且值发生变化时发出
     */
    void isEnabledChanged();

    /**
     * @brief 服务器数量变化信号
     * @details
     * 在以下情况发出：
     * - 添加服务器
     * - 移除服务器
     * - 清空服务器
     * - 设置服务器列表
     */
    void serverCountChanged();

    /**
     * @brief 最后更新时间变化信号
     * @details 当调用 setLastUpdated() 且值发生变化时发出
     */
    void lastUpdatedChanged();

    /**
     * @brief 服务器列表变化信号
     * @details
     * 在以下情况发出：
     * - 添加服务器
     * - 移除服务器
     * - 清空服务器
     * - 设置服务器列表
     */
    void serversChanged();

    /**
     * @brief 流量信息变化信号
     * @details
     * 在以下情况发出：
     * - 设置总流量
     * - 设置已用流量
     */
    void trafficChanged();

    /**
     * @brief 更新状态变化信号
     * @details 当调用 setUpdateStatus() 且值发生变化时发出
     */
    void updateStatusChanged();

    /**
     * @brief 更新开始信号
     * @details 由 SubscriptionManager 在开始更新时发出
     */
    void updateStarted();

    /**
     * @brief 更新完成信号
     * @param success 是否成功
     * @param serverCount 解析到的服务器数量
     * @details 由 SubscriptionManager 在更新完成时发出
     */
    void updateCompleted(bool success, int serverCount);

    /**
     * @brief 更新失败信号
     * @param error 错误信息
     * @details 由 SubscriptionManager 在更新失败时发出
     */
    void updateFailed(const QString& error);

    /**
     * @brief 更新进度信号
     * @param percentage 下载进度百分比（0-100）
     * @details 由 SubscriptionManager 在下载过程中发出
     */
    void updateProgress(int percentage);

    /**
     * @brief 代理组变化信号
     */
    void proxyGroupsChanged();

    /**
     * @brief 路由规则变化信号
     */
    void rulesChanged();

    /**
     * @brief DNS 配置变化信号
     */
    void dnsConfigChanged();

private:
    // ========== 基本属性 ==========
    QString m_id;                    ///< 订阅唯一标识符
    QString m_name;                  ///< 订阅名称
    QString m_url;                   ///< 订阅地址
    bool m_isEnabled;                ///< 是否启用
    SubscriptionType m_type;         ///< 订阅类型
    UpdateStatus m_updateStatus;     ///< 更新状态
    QList<Server*> m_servers;        ///< 服务器列表

    // ========== 时间信息 ==========
    QDateTime m_lastUpdated;         ///< 最后更新时间
    QDateTime m_createdAt;           ///< 创建时间
    int m_updateInterval;            ///< 更新间隔（分钟）
    bool m_autoUpdate;               ///< 是否自动更新
    QString m_lastUpdateError;       ///< 最后更新错误信息

    // ========== 流量信息 ==========
    qint64 m_totalTraffic;           ///< 总流量（字节）
    qint64 m_usedTraffic;            ///< 已用流量（字节）
    QDateTime m_trafficResetDate;    ///< 流量重置日期

    // ========== 认证信息 ==========
    QString m_username;              ///< 用户名
    QString m_password;              ///< 密码
    QString m_token;                 ///< 访问令牌
    QMap<QString, QString> m_customHeaders;  ///< 自定义请求头

    // ========== 分组和过滤 ==========
    QString m_groupName;             ///< 分组名称
    QStringList m_tags;              ///< 标签列表
    QStringList m_includeKeywords;   ///< 包含关键词
    QStringList m_excludeKeywords;   ///< 排除关键词

    // ========== 其他设置 ==========
    QString m_userAgent;             ///< User-Agent 字符串
    int m_timeout;                   ///< 超时时间（秒）
    bool m_verifySSL;                ///< 是否验证 SSL 证书

    // ========== Clash 配置 ==========
    QList<ProxyGroup*> m_proxyGroups;  ///< Clash 代理组列表
    QList<Rule*> m_rules;              ///< Clash 路由规则列表
    DnsConfig* m_dnsConfig;            ///< Clash DNS 配置

    /**
     * @brief 生成唯一 ID
     * @return QString UUID 字符串（不含大括号）
     * @note 使用 Qt 的 UUID 生成器
     */
    static QString generateId();

    /**
     * @brief 从字符串解析订阅类型
     * @param typeStr 类型字符串
     * @return SubscriptionType 订阅类型枚举值
     * @details
     * 支持的类型字符串：
     * - "SIP008" → SIP008
     * - "V2rayN" → V2rayN
     * - "Clash" → Clash
     * - "Surfboard" → Surfboard
     * - "Custom" → Custom
     * - 其他 → Standard（默认）
     */
    static SubscriptionType parseType(const QString& typeStr);

    /**
     * @brief 订阅类型转字符串
     * @param type 订阅类型枚举值
     * @return QString 类型字符串
     * @details 用于序列化到 JSON
     */
    static QString typeToString(SubscriptionType type);
};

#endif // SUBSCRIPTION_H