/**
 * @file ClashConfigParser.h
 * @brief Clash 配置文件解析器
 * @details 轻量级 YAML 解析器，专门用于解析 Clash 订阅配置
 * @author JinDo Core Team
 * @date 2025
 */

#ifndef CLASHCONFIGPARSER_H
#define CLASHCONFIGPARSER_H

#include <QString>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>

class Server;
class ProxyGroup;
class Rule;
class DnsConfig;
class Subscription;

/**
 * @class ClashConfigParser
 * @brief Clash 配置解析器
 *
 * 支持解析 Clash 配置中的：
 * - proxies: 代理服务器列表
 * - proxy-groups: 代理组配置
 * - rules: 路由规则
 * - dns: DNS 配置
 *
 * 使用简单的行解析方法处理 YAML，避免引入外部依赖
 */
class ClashConfigParser
{
public:
    /**
     * @brief 解析结果结构
     */
    struct ParseResult {
        QList<Server*> servers;          ///< 解析出的服务器列表
        QList<ProxyGroup*> proxyGroups;  ///< 解析出的代理组列表
        QList<Rule*> rules;              ///< 解析出的路由规则
        DnsConfig* dnsConfig;            ///< DNS 配置
        QString errorMessage;            ///< 错误信息（如果有）
        bool success;                    ///< 是否解析成功

        ParseResult() : dnsConfig(nullptr), success(false) {}
    };

    ClashConfigParser();
    ~ClashConfigParser();

    /**
     * @brief 解析 Clash 配置
     * @param data YAML 格式的配置数据
     * @param subscription 所属订阅（可选，用于设置 subscriptionId）
     * @return 解析结果
     */
    ParseResult parse(const QByteArray& data, Subscription* subscription = nullptr);

    /**
     * @brief 检测是否为 Clash 格式
     * @param data 配置数据
     * @return 是否为 Clash 格式
     */
    static bool isClashFormat(const QByteArray& data);

private:
    // YAML 解析辅助方法
    QJsonObject parseYamlToJson(const QString& yaml);
    QJsonArray parseYamlArray(const QStringList& lines, int& index, int baseIndent);
    QJsonObject parseYamlObject(const QStringList& lines, int& index, int baseIndent);
    QJsonValue parseYamlValue(const QString& value);
    int getIndentLevel(const QString& line);
    QString trimComment(const QString& line);

    // 配置段解析
    QList<Server*> parseProxies(const QJsonArray& proxiesArray, Subscription* subscription);
    QList<ProxyGroup*> parseProxyGroups(const QJsonArray& groupsArray, Subscription* subscription);
    QList<Rule*> parseRules(const QJsonArray& rulesArray, Subscription* subscription);
    DnsConfig* parseDns(const QJsonObject& dnsObject, Subscription* subscription);

    // 服务器解析
    Server* parseVmessProxy(const QJsonObject& config, Subscription* subscription);
    Server* parseVlessProxy(const QJsonObject& config, Subscription* subscription);
    Server* parseShadowsocksProxy(const QJsonObject& config, Subscription* subscription);
    Server* parseTrojanProxy(const QJsonObject& config, Subscription* subscription);
    Server* parseSocksProxy(const QJsonObject& config, Subscription* subscription);
    Server* parseHttpProxy(const QJsonObject& config, Subscription* subscription);
    Server* parseHysteriaProxy(const QJsonObject& config, Subscription* subscription);
    Server* parseHysteria2Proxy(const QJsonObject& config, Subscription* subscription);
    Server* parseTuicProxy(const QJsonObject& config, Subscription* subscription);
    Server* parseWireGuardProxy(const QJsonObject& config, Subscription* subscription);
};

#endif // CLASHCONFIGPARSER_H
