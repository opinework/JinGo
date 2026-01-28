/**
 * @file ServerListViewModel.h
 * @brief 服务器列表视图模型头文件
 * @details 管理服务器列表的显示、筛选、排序和选择，为QML界面提供数据绑定
 * @author JinGo VPN Team
 * @date 2025
 */

#ifndef SERVERLISTVIEWMODEL_H
#define SERVERLISTVIEWMODEL_H

#include <QObject>
#include <QList>
#include <QString>
#include <QPointer>
#include "models/Server.h"

class SubscriptionManager;
class VPNManager;

/**
 * @class ServerListViewModel
 * @brief 服务器列表视图模型
 * @details 提供服务器列表的管理功能，包括：
 * - 从订阅管理器加载服务器列表
 * - 筛选和搜索服务器
 * - 按延迟或名称排序
 * - 选择和连接服务器
 * - 测试服务器延迟
 */
class ServerListViewModel : public QObject
{
    Q_OBJECT

    /// 过滤后的服务器列表（只读，返回副本）
    Q_PROPERTY(QList<Server*> servers READ servers NOTIFY serversChanged)

    /// 当前选中的服务器（可读写，使用QPointer安全持有）
    Q_PROPERTY(Server* selectedServer READ selectedServer WRITE selectServer NOTIFY selectedServerChanged)

    /// 筛选文本（可读写）
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)

    /// 是否正在加载（只读）
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

    /// 是否已连接（只读）
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY isConnectedChanged)

    /// 连接状态文本（只读）
    Q_PROPERTY(QString connectionStateText READ connectionStateText NOTIFY connectionStateTextChanged)

    /// 是否正在批量测试延迟（只读）
    Q_PROPERTY(bool isBatchTesting READ isBatchTesting NOTIFY isBatchTestingChanged)

    /// 测试进度文本（只读，例如 "Testing 3 of 10 servers..."）
    Q_PROPERTY(QString testingProgressText READ testingProgressText NOTIFY testingProgressTextChanged)

    /// 速度测试结果（serverId -> {ip, speed, asn, isp, country}）
    Q_PROPERTY(QVariantMap speedTestResults READ speedTestResults NOTIFY speedTestResultsChanged)

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit ServerListViewModel(QObject* parent = nullptr);

    /**
     * @brief 获取过滤后的服务器列表（返回副本）
     * @return 服务器指针列表的副本，所有空指针已被过滤
     */
    QList<Server*> servers() const {
        // 返回副本，并过滤掉空指针和已删除的对象
        // QPointer 会在对象被删除时自动变为 null，所以这里很安全
        QList<Server*> validServers;
        for (const QPointer<Server>& serverPtr : m_filteredServers) {
            // QPointer 在对象被删除后会自动变为 null
            if (!serverPtr.isNull()) {
                validServers.append(serverPtr.data());
            }
        }
        return validServers;
    }

    /**
     * @brief 获取当前选中的服务器
     * @return 服务器指针，如果没有选中或对象已被删除则返回nullptr
     */
    Server* selectedServer() const { return m_selectedServer.data(); }

    /**
     * @brief 获取当前的筛选文本
     * @return 筛选文本字符串
     */
    QString filterText() const { return m_filterText; }

    /**
     * @brief 获取是否正在加载状态
     * @return true表示正在加载，false表示加载完成
     */
    bool isLoading() const { return m_isLoading; }

    /**
     * @brief 获取是否已连接到VPN
     * @return true表示已连接，false表示未连接
     */
    bool isConnected() const;

    /**
     * @brief 获取连接状态文本描述
     * @return 状态文本（如"已连接"、"正在连接"、"未连接"等）
     */
    QString connectionStateText() const;

    /**
     * @brief 获取是否正在批量测试
     * @return true表示正在批量测试，false表示未测试
     */
    bool isBatchTesting() const { return m_isBatchTesting; }

    /**
     * @brief 获取测试进度文本
     * @return 进度文本（例如 "Testing 3 of 10 servers..."）
     */
    QString testingProgressText() const { return m_testingProgressText; }

    /**
     * @brief 获取速度测试结果
     * @return 测速结果映射 {serverId -> {ip, speed, asn, isp, country}}
     */
    QVariantMap speedTestResults() const { return m_speedTestResults; }

    /**
     * @brief 设置筛选文本
     * @param text 筛选文本，将用于匹配服务器名称、地址和位置
     */
    void setFilterText(const QString& text);

    /**
     * @brief 选择服务器
     * @param server 要选择的服务器指针
     */
    void selectServer(Server* server);

public slots:
    /**
     * @brief 刷新服务器列表
     * @details 触发订阅更新，延迟2秒后重新从订阅管理器加载服务器
     */
    void refreshServers();

    /**
     * @brief 连接到指定服务器
     * @param server 要连接的服务器指针
     */
    void connectToServer(Server* server);

    /**
     * @brief 断开当前VPN连接
     */
    void disconnect();

    /**
     * @brief 快速连接到当前选中的服务器
     * @details 如果没有选中服务器，将不执行任何操作
     */
    void connectToSelected();

    /**
     * @brief 切换连接状态
     * @details 如果当前已连接，则断开；如果未连接，则连接到选中的服务器
     */
    void toggleConnection();

    /**
     * @brief 测试服务器延迟
     * @param serverId 要测试的服务器ID
     */
    Q_INVOKABLE void testServerLatency(const QString& serverId);

    /**
     * @brief 测试服务器吞吐量（下载速度）
     * @param serverId 要测试的服务器ID
     * @details 通过临时启动代理并下载测试文件来测量服务器的实际下载速度
     */
    Q_INVOKABLE void testServerThroughput(const QString& serverId);

    /**
     * @brief 批量测试所有服务器延迟
     */
    Q_INVOKABLE void testAllServersLatency();

    /**
     * @brief 按延迟排序服务器列表
     */
    Q_INVOKABLE void sortByLatency();

    /**
     * @brief 按名称排序服务器列表
     */
    Q_INVOKABLE void sortByName();

    /**
     * @brief 获取按距离排序的大洲列表
     * @return 按距离从近到远排序的大洲列表
     */
    Q_INVOKABLE QStringList getSortedContinents() const;

    /**
     * @brief 从数据库重新加载服务器列表
     * @details 不触发网络更新，只从本地SubscriptionManager的缓存中加载
     */
    void loadServersFromDatabase();

    /**
     * @brief 设置服务器的速度测试结果
     * @param serverId 服务器ID
     * @param result 测试结果 {ip, speed, asn, isp, country}
     */
    Q_INVOKABLE void setSpeedTestResult(const QString& serverId, const QVariantMap& result);

    /**
     * @brief 获取服务器的速度测试结果
     * @param serverId 服务器ID
     * @return 测试结果，如果没有则返回空 QVariantMap
     */
    Q_INVOKABLE QVariantMap getSpeedTestResult(const QString& serverId) const;

    /**
     * @brief 清除所有速度测试结果
     */
    Q_INVOKABLE void clearSpeedTestResults();

    /**
     * @brief 保存测速结果到本地文件
     */
    Q_INVOKABLE void saveSpeedTestResults();

    /**
     * @brief 从本地文件加载测速结果
     */
    Q_INVOKABLE void loadSpeedTestResults();

    /**
     * @brief 手动添加服务器
     * @param jsonOrLink JSON配置或分享链接
     * @return 添加成功返回服务器指针，失败返回nullptr
     */
    Server* addManualServer(const QString& jsonOrLink);

private slots:
    /**
     * @brief 从订阅管理器加载服务器
     * @details 获取所有订阅的服务器并应用当前筛选条件
     */
    void loadServersFromManager();

    /**
     * @brief 应用筛选条件
     * @details 根据filterText筛选服务器并更新filteredServers列表
     */
    void applyFilter();

signals:
    /**
     * @brief 服务器列表变化信号
     */
    void serversChanged();

    /**
     * @brief 选中服务器变化信号
     */
    void selectedServerChanged();

    /**
     * @brief 筛选文本变化信号
     */
    void filterTextChanged();

    /**
     * @brief 加载状态变化信号
     */
    void isLoadingChanged();

    /**
     * @brief 服务器测试完成信号
     * @param server 已测试的服务器指针
     */
    void serverTestCompleted(Server* server);

    /**
     * @brief 连接状态变化信号
     */
    void isConnectedChanged();

    /**
     * @brief 连接状态文本变化信号
     */
    void connectionStateTextChanged();

    /**
     * @brief 批量测试状态变化信号
     */
    void isBatchTestingChanged();

    /**
     * @brief 测试进度文本变化信号
     */
    void testingProgressTextChanged();

    /**
     * @brief 所有服务器测试完成信号
     */
    void allTestsCompleted();

    /**
     * @brief 速度测试结果变化信号
     */
    void speedTestResultsChanged();

    /**
     * @brief 服务器吞吐量测试完成信号
     * @param server 已测试的服务器指针
     * @param speedMbps 下载速度（Mbps），失败时为-1
     */
    void serverThroughputTestCompleted(Server* server, double speedMbps);

private:
    /**
     * @brief 设置加载状态
     * @param loading 是否正在加载
     */
    void setIsLoading(bool loading);

    /**
     * @brief 处理批量测试进度更新
     * @details 检查并更新批量测试进度，当所有测试完成时执行排序
     */
    void handleBatchTestProgress();

    QList<QPointer<Server>> m_allServers;          ///< 所有服务器列表（使用QPointer防止悬挂指针）
    QList<QPointer<Server>> m_filteredServers;     ///< 过滤后的服务器列表（使用QPointer防止悬挂指针）
    QPointer<Server> m_selectedServer;     ///< 当前选中的服务器（使用QPointer安全持有）
    QString m_filterText;                  ///< 筛选文本
    bool m_isLoading;                      ///< 是否正在加载
    bool m_isUpdating;                     ///< 是否正在更新（防重入）
    bool m_isBatchTesting;                 ///< 是否正在批量测试
    int m_totalTestCount;                  ///< 总测试数量
    int m_completedTestCount;              ///< 已完成测试数量
    QString m_testingProgressText;         ///< 测试进度文本
    QVariantMap m_speedTestResults;        ///< 速度测试结果 {serverId -> {ip, speed, asn, isp, country}}

    SubscriptionManager* m_subscriptionManager; ///< 订阅管理器指针
    VPNManager* m_vpnManager;                   ///< VPN管理器指针
};

#endif // SERVERLISTVIEWMODEL_H
