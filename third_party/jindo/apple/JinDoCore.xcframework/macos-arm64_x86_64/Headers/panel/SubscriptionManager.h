/**
 * @file SubscriptionManager.h
 * @brief 订阅管理器头文件
 * @details 提供订阅的增删改查、自动更新、协议解析等完整功能，支持多种订阅格式
 * @author JinGo VPN Team
 * @date 2025
 */

#ifndef SUBSCRIPTIONMANAGER_H
#define SUBSCRIPTIONMANAGER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QMutex>
#include <QPointer>
#include "../models/Subscription.h"
#include "../core/IServerProvider.h"

// 前向声明
class Server;
class DatabaseManager;

// ============================================================================
// SubscriptionManager 类定义
// ============================================================================

/**
 * @class SubscriptionManager
 * @brief 订阅管理器（单例模式，线程安全）
 *
 * @details
 * 核心功能：
 * - 订阅管理：添加、删除、更新、查询订阅
 * - 服务器解析：支持多种协议和格式
 *   - 协议：VMess、VLESS、Shadowsocks、Trojan
 *   - 格式：Base64、JSON (SIP008)、YAML (Clash)
 * - 自动更新：定时检查并更新订阅
 * - 批量操作：批量更新多个订阅
 * - 持久化：订阅数据保存到数据库
 * - 手动配置：支持手动添加服务器
 *
 * 设计特点：
 * - 单例模式：全局唯一实例
 * - 线程安全：使用互斥锁保护共享数据
 * - 异步网络：所有网络请求都是异步的
 * - 进度反馈：更新进度通过信号实时通知
 * - 错误处理：详细的错误信息和恢复机制
 *
 * 使用流程：
 * 1. 获取单例：SubscriptionManager::instance()
 * 2. 添加订阅：addSubscription(url, name)
 * 3. 更新订阅：updateSubscription(id) 或 updateAllSubscriptions()
 * 4. 获取服务器：getServers(id) 或 getAllServers()
 * 5. 连接信号：监听 updateCompleted、subscriptionAdded 等
 *
 * @note
 * - 线程安全：可在多线程环境中使用
 * - 内存管理：Server 和 Subscription 对象由管理器管理
 * - 数据持久化：订阅自动保存到 SQLite 数据库
 *
 * @example C++ 使用示例
 * @code
 * // 获取单例
 * SubscriptionManager& manager = SubscriptionManager::instance();
 *
 * // 连接信号
 * connect(&manager, &SubscriptionManager::updateCompleted,
 *         this, [](const QString& id, bool success, int count) {
 *     qDebug() << "订阅更新完成:" << id << "成功:" << success << "服务器数:" << count;
 * });
 *
 * // 添加订阅
 * Subscription* sub = manager.addSubscription(
 *     "https://example.com/subscribe?token=xxx",
 *     "我的订阅"
 * );
 *
 * // 更新订阅
 * if (sub) {
 *     manager.updateSubscription(sub->id());
 * }
 *
 * // 获取所有服务器
 * QList<Server*> servers = manager.getAllServers();
 * @endcode
 *
 * @example QML 使用示例
 * @code
 * import JinGo 1.0
 *
 * Button {
 *     text: "添加订阅"
 *     onClicked: {
 *         SubscriptionManager.addSubscription(urlField.text, nameField.text)
 *     }
 * }
 *
 * Button {
 *     text: "更新所有订阅"
 *     enabled: !SubscriptionManager.isUpdating
 *     onClicked: {
 *         SubscriptionManager.updateAllSubscriptions()
 *     }
 * }
 *
 * Text {
 *     text: "订阅数量: " + SubscriptionManager.subscriptionCount
 * }
 *
 * Text {
 *     text: "服务器总数: " + SubscriptionManager.totalServerCount
 * }
 *
 * Connections {
 *     target: SubscriptionManager
 *     function onUpdateCompleted(subscriptionId, success, serverCount) {
 *         if (success) {
 *             console.log("更新成功，获得", serverCount, "个服务器")
 *         } else {
 *             console.error("更新失败")
 *         }
 *     }
 * }
 * @endcode
 */
class SubscriptionManager : public QObject, public IServerProvider
{
    Q_OBJECT

    // ========================================================================
    // QML 属性
    // ========================================================================

    /**
     * @property subscriptionCount
     * @brief 订阅数量（只读属性）
     * @details 返回当前管理的订阅总数
     * @notify subscriptionCountChanged()
     */
    Q_PROPERTY(int subscriptionCount READ subscriptionCount NOTIFY subscriptionCountChanged)

    /**
     * @property totalServerCount
     * @brief 服务器总数（只读属性）
     * @details 返回所有订阅中的服务器总数
     * @notify totalServerCountChanged()
     */
    Q_PROPERTY(int totalServerCount READ totalServerCount NOTIFY totalServerCountChanged)

    /**
     * @property isUpdating
     * @brief 更新状态（只读属性）
     * @details
     * - true：正在更新订阅
     * - false：空闲状态
     * @note 可用于禁用 UI 按钮，避免重复操作
     * @notify isUpdatingChanged()
     */
    Q_PROPERTY(bool isUpdating READ isUpdating NOTIFY isUpdatingChanged)

public:
    // ========================================================================
    // 单例访问
    // ========================================================================

    /**
     * @brief 获取单例实例
     * @return SubscriptionManager& 全局唯一实例的引用
     * @note 线程安全，使用双重检查锁定模式
     */
    static SubscriptionManager& instance();

    /**
     * @brief 销毁单例实例
     * @details
     * 清理流程：
     * 1. 保存所有订阅到数据库
     * 2. 取消所有进行中的网络请求
     * 3. 清理订阅和服务器对象
     * 4. 删除单例实例
     *
     * @note
     * - 通常在应用退出时调用
     * - 调用后需要重新调用 instance() 才能使用
     * - 线程安全
     */
    static void destroy();

    // ========================================================================
    // 订阅管理
    // ========================================================================

    /**
     * @brief 添加订阅
     * @param url 订阅 URL（必需）
     * @param name 订阅名称（可选，不提供则自动生成）
     * @return Subscription* 创建的订阅对象，失败返回 nullptr
     *
     * @details
     * 添加流程：
     * 1. 验证 URL 格式
     * 2. 检查 URL 是否已存在（避免重复添加）
     * 3. 创建 Subscription 对象
     * 4. 保存到数据库
     * 5. 添加到内存列表
     * 6. 发出 subscriptionAdded 信号
     *
     * 支持的 URL 格式：
     * - 标准 HTTP/HTTPS 订阅地址
     * - Base64 编码的节点列表
     * - JSON 格式订阅 (SIP008)
     * - YAML 格式订阅 (Clash)
     *
     * @note
     * - 添加成功后不会自动更新，需手动调用 updateSubscription()
     * - URL 必须以 http:// 或 https:// 开头
     * - 名称为空时自动从 URL 或响应头中提取
     * - 返回的对象由 SubscriptionManager 管理，不要手动删除
     *
     * @warning
     * - URL 重复会导致添加失败
     * - 网络不可达不影响添加，但更新会失败
     *
     * @see updateSubscription(), removeSubscription(), subscriptionAdded()
     */
    Q_INVOKABLE Subscription* addSubscription(const QString& url, const QString& name = QString());

    /**
     * @brief 删除订阅
     * @param subscriptionId 订阅唯一标识符
     * @return bool true 表示删除成功，false 表示订阅不存在
     *
     * @details
     * 删除流程：
     * 1. 查找订阅对象
     * 2. 取消该订阅的进行中的更新请求
     * 3. 删除关联的所有服务器
     * 4. 从数据库中删除
     * 5. 从内存列表中移除
     * 6. 发出 subscriptionRemoved 信号
     * 7. 释放对象内存
     *
     * @note
     * - 删除操作不可逆，请谨慎操作
     * - 关联的服务器也会被删除
     * - 如果该订阅的服务器正在连接，会先断开
     *
     * @see addSubscription(), subscriptionRemoved()
     */
    Q_INVOKABLE bool removeSubscription(const QString& subscriptionId);

    /**
     * @brief 更新订阅
     * @param subscriptionId 订阅唯一标识符
     *
     * @details
     * 更新流程：
     * 1. 检查订阅是否存在
     * 2. 发出 updateStarted 信号
     * 3. 发起异步网络请求
     * 4. 下载订阅内容
     * 5. 解析服务器列表
     * 6. 更新数据库
     * 7. 更新内存数据
     * 8. 发出 updateCompleted 或 updateFailed 信号
     *
     * 支持的订阅格式：
     * - Base64：标准的 Base64 编码节点列表
     * - SIP008：Shadowsocks JSON 格式
     * - Clash：YAML 配置格式
     *
     * 响应信号：
     * - 开始：updateStarted(subscriptionId)
     * - 进度：updateProgress(subscriptionId, percentage)
     * - 成功：updateCompleted(subscriptionId, true, serverCount)
     * - 失败：updateFailed(subscriptionId, error)
     *
     * @note
     * - 异步操作，不会阻塞 UI
     * - 更新会覆盖旧的服务器列表
     * - 网络超时时间为 30 秒
     * - 支持 HTTP 重定向
     *
     * @see updateAllSubscriptions(), cancelUpdate(), updateCompleted()
     */
    Q_INVOKABLE void updateSubscription(const QString& subscriptionId);

    /**
     * @brief 更新所有启用的订阅
     *
     * @details
     * 批量更新流程：
     * 1. 筛选所有启用的订阅
     * 2. 发出 batchUpdateStarted 信号
     * 3. 逐个更新订阅
     * 4. 发出 batchUpdateProgress 信号（每完成一个）
     * 5. 全部完成后发出 batchUpdateCompleted 信号
     *
     * 响应信号：
     * - 开始：batchUpdateStarted(totalCount)
     * - 进度：batchUpdateProgress(current, total)
     * - 完成：batchUpdateCompleted(successCount, failedCount)
     *
     * @note
     * - 只更新 enabled 为 true 的订阅
     * - 按添加顺序依次更新
     * - 单个订阅失败不影响其他订阅
     * - 可以通过 cancelAllUpdates() 取消
     *
     * @see updateSubscriptions(), cancelAllUpdates(), batchUpdateCompleted()
     */
    Q_INVOKABLE void updateAllSubscriptions();

    /**
     * @brief 批量更新指定订阅
     * @param subscriptionIds 订阅 ID 列表
     *
     * @details
     * 与 updateAllSubscriptions() 类似，但只更新指定的订阅
     *
     * @note
     * - 不检查订阅是否启用
     * - 无效的 ID 会被跳过
     * - 支持部分更新（一些成功，一些失败）
     *
     * @see updateAllSubscriptions(), updateSubscription()
     */
    Q_INVOKABLE void updateSubscriptions(const QStringList& subscriptionIds);

    /**
     * @brief 取消正在进行的更新
     * @param subscriptionId 订阅唯一标识符
     *
     * @details
     * 取消流程：
     * 1. 查找对应的网络请求
     * 2. 中止网络请求
     * 3. 清理临时数据
     * 4. 发出 updateFailed 信号（错误信息为 "已取消"）
     *
     * @note
     * - 已下载的数据会被丢弃
     * - 数据库不会被修改
     * - 取消后可以重新更新
     *
     * @see updateSubscription(), cancelAllUpdates()
     */
    Q_INVOKABLE void cancelUpdate(const QString& subscriptionId);

    /**
     * @brief 取消所有更新
     *
     * @details
     * 中止所有进行中的网络请求，并清理状态
     *
     * @note
     * - 会中止批量更新操作
     * - 发出 batchUpdateCompleted 信号（带取消标记）
     *
     * @see cancelUpdate(), updateAllSubscriptions()
     */
    Q_INVOKABLE void cancelAllUpdates();

    // ========================================================================
    // 查询方法
    // ========================================================================

    /**
     * @brief 获取所有订阅
     * @return QList<Subscription*> 订阅对象指针列表
     * @note
     * - 包括启用和禁用的订阅
     * - 返回的指针由 SubscriptionManager 管理
     * - 列表按添加时间排序
     */
    QList<Subscription*> subscriptions() const;

    /**
     * @brief 获取启用的订阅
     * @return QList<Subscription*> 启用的订阅列表
     * @note 只返回 enabled 为 true 的订阅
     */
    QList<Subscription*> enabledSubscriptions() const;

    /**
     * @brief 获取指定订阅
     * @param subscriptionId 订阅唯一标识符
     * @return Subscription* 订阅对象指针，未找到返回 nullptr
     */
    Subscription* getSubscription(const QString& subscriptionId) const;

    /**
     * @brief 根据 URL 查找订阅
     * @param url 订阅 URL
     * @return Subscription* 订阅对象指针，未找到返回 nullptr
     * @note 用于检查 URL 是否已存在
     */
    Subscription* findSubscriptionByUrl(const QString& url) const;

    /**
     * @brief 获取所有服务器
     * @return QList<Server*> 所有订阅的服务器列表
     * @note
     * - 包括所有订阅的服务器（启用和禁用）
     * - 返回的指针由 SubscriptionManager 管理
     * - 列表不包含重复服务器
     * @implements IServerProvider::getAllServers
     */
    QList<Server*> getAllServers() const override;

    /**
     * @brief 获取订阅的服务器
     * @param subscriptionId 订阅唯一标识符
     * @return QList<Server*> 该订阅的服务器列表
     * @note 只返回指定订阅的服务器
     */
    QList<Server*> getServers(const QString& subscriptionId) const;

    /**
     * @brief 获取启用订阅的所有服务器
     * @return QList<Server*> 启用订阅的服务器列表
     * @note
     * - 只返回启用订阅中的服务器
     * - 用于显示可连接的服务器列表
     */
    QList<Server*> getEnabledServers() const;

    /**
     * @brief 根据服务器ID获取服务器
     * @param serverId 服务器唯一标识符
     * @return Server* 服务器对象指针，未找到返回 nullptr
     * @note
     * - 在所有订阅的服务器中查找
     * - 包括启用和禁用的订阅
     * - 用于通过ID恢复上次选中的服务器
     * @implements IServerProvider::getServerById
     */
    Server* getServerById(const QString& serverId) const override;

    /**
     * @brief 获取订阅数量
     * @return int 当前管理的订阅总数
     */
    int subscriptionCount() const;

    /**
     * @brief 获取服务器总数
     * @return int 所有订阅中的服务器总数
     */
    int totalServerCount() const;

    /**
     * @brief 获取更新状态
     * @return bool true 表示正在更新，false 表示空闲
     */
    bool isUpdating() const;

    // ========================================================================
    // 持久化方法
    // ========================================================================

    /**
     * @brief 从数据库加载订阅
     * @return bool true 表示加载成功，false 表示失败
     *
     * @details
     * 加载流程：
     * 1. 连接数据库
     * 2. 查询所有订阅记录
     * 3. 创建 Subscription 对象
     * 4. 加载关联的服务器
     * 5. 添加到内存列表
     *
     * @note
     * - 应用启动时自动调用
     * - 加载失败不影响应用运行，但订阅列表为空
     * - 数据库文件路径：~/.config/JinGoVPN/subscriptions.db
     *
     * @see saveAllSubscriptions()
     */
    bool loadSubscriptions();

    /**
     * @brief 保存所有订阅到数据库
     * @return bool true 表示保存成功，false 表示失败
     *
     * @details
     * 保存流程：
     * 1. 开启数据库事务
     * 2. 逐个保存订阅和服务器
     * 3. 提交事务
     *
     * @note
     * - 应用退出时自动调用
     * - 保存失败会回滚事务
     * - 使用事务确保数据一致性
     *
     * @see saveSubscription(), loadSubscriptions()
     */
    bool saveAllSubscriptions();

    /**
     * @brief 保存单个订阅
     * @param subscription 订阅对象指针
     * @return bool true 表示保存成功，false 表示失败
     *
     * @details
     * - 如果订阅已存在，则更新
     * - 如果订阅不存在，则插入
     * - 同时保存关联的服务器
     *
     * @note
     * - 订阅信息变更后自动调用
     * - 服务器列表变更后也会调用
     *
     * @see saveAllSubscriptions()
     */
    bool saveSubscription(Subscription* subscription);

    /**
     * @brief 导出订阅配置（JSON 格式）
     * @return QString JSON 格式的订阅配置字符串
     *
     * @details
     * 导出格式：
     * ```json
     * {
     *   "version": "1.0",
     *   "subscriptions": [
     *     {
     *       "id": "uuid",
     *       "name": "订阅名称",
     *       "url": "订阅地址",
     *       "enabled": true,
     *       "updateInterval": 86400
     *     }
     *   ]
     * }
     * ```
     *
     * @note
     * - 不导出服务器列表（只导出订阅配置）
     * - 可用于备份或分享订阅配置
     * - 不包含敏感信息（如 Token）
     *
     * @see importSubscriptions()
     */
    QString exportSubscriptions() const;

    /**
     * @brief 导入订阅配置
     * @param json JSON 格式的订阅配置字符串
     * @return bool true 表示导入成功，false 表示失败
     *
     * @details
     * 导入流程：
     * 1. 解析 JSON 字符串
     * 2. 验证格式和版本
     * 3. 逐个添加订阅（跳过重复的）
     * 4. 保存到数据库
     *
     * @note
     * - URL 重复的订阅会被跳过
     * - 导入成功后会发出 subscriptionAdded 信号
     * - 不会自动更新订阅，需手动调用
     *
     * @see exportSubscriptions()
     */
    bool importSubscriptions(const QString& json);

    // ========================================================================
    // 手动配置
    // ========================================================================

    /**
     * @brief 手动添加服务器配置
     * @param jsonOrLink 服务器配置 JSON 字符串或分享链接
     * @param subscriptionId 关联的订阅 ID（可选）
     * @return Server* 创建的服务器对象，失败返回 nullptr
     *
     * @details
     * 支持的格式：
     * 1. 分享链接：
     *    - vmess://...
     *    - vless://...
     *    - ss://...
     *    - trojan://...
     *
     * 2. JSON 配置：
     * ```json
     * {
     *   "type": "vmess",
     *   "name": "服务器名称",
     *   "address": "example.com",
     *   "port": 443,
     *   "uuid": "uuid-string",
     *   ...
     * }
     * ```
     *
     * @param subscriptionId
     * - 如果提供，服务器会关联到该订阅
     * - 如果为空，创建一个 "手动配置" 订阅
     *
     * @note
     * - 手动添加的服务器也会保存到数据库
     * - 支持与订阅服务器混合使用
     * - 手动服务器不会被订阅更新覆盖
     *
     * @see parseVmessUri(), parseShadowsocksUri()
     */
    Q_INVOKABLE Server* addServerManually(const QString& jsonOrLink, const QString& subscriptionId = QString());

    // ========================================================================
    // 优惠券和通知相关方法
    // ========================================================================

    /**
     * @brief 获取优惠券列表
     *
     * @details
     * 查询流程：
     * 1. 检查是否已登录
     * 2. 发送请求到 /user/coupon/fetch
     * 3. 返回可用优惠券列表
     *
     * 响应信号：
     * - 成功：couponsLoaded(QJsonArray)
     * - 失败：couponsFailed(QString error)
     */
    Q_INVOKABLE void fetchCoupons();

    /**
     * @brief 检查优惠券有效性
     * @param code 优惠券代码
     * @param planId 套餐ID
     *
     * @details
     * 检查流程：
     * 1. 检查是否已登录
     * 2. 发送请求到 /user/coupon/check
     * 3. 验证优惠券是否可用
     *
     * 响应信号：
     * - 成功：couponChecked(QJsonObject)
     * - 失败：couponCheckFailed(QString error)
     */
    Q_INVOKABLE void checkCoupon(const QString& code, int planId);

    /**
     * @brief 获取通知列表
     *
     * @details
     * 查询流程：
     * 1. 检查是否已登录
     * 2. 发送请求到 /user/notice/fetch
     * 3. 返回通知列表
     *
     * 响应信号：
     * - 成功：noticesLoaded(QJsonArray)
     * - 失败：noticesFailed(QString error)
     */
    Q_INVOKABLE void fetchNotices();

    /**
     * @brief 标记通知为已读
     * @param noticeId 通知ID
     *
     * @details
     * 标记流程：
     * 1. 检查是否已登录
     * 2. 发送请求到 /user/notice/read
     * 3. 更新通知状态
     *
     * 响应信号：
     * - 成功：noticeRead(int noticeId)
     * - 失败：noticeReadFailed(QString error)
     */
    Q_INVOKABLE void markNoticeRead(int noticeId);

    // ========================================================================
    // 信号定义
    // ========================================================================

signals:
    // ====== 订阅相关信号 ======

    /**
     * @brief 订阅添加信号
     * @param subscription 新添加的订阅对象
     */
    void subscriptionAdded(Subscription* subscription);

    /**
     * @brief 订阅删除信号
     * @param subscriptionId 被删除的订阅 ID
     */
    void subscriptionRemoved(const QString& subscriptionId);

    /**
     * @brief 订阅更新信号
     * @param subscription 更新的订阅对象
     * @details 订阅信息（如名称、状态）变更时发出
     */
    void subscriptionUpdated(Subscription* subscription);

    /**
     * @brief 订阅数量变化信号
     * @details 添加或删除订阅时发出
     */
    void subscriptionCountChanged();

    /**
     * @brief 服务器总数变化信号
     * @details 服务器列表变更时发出
     */
    void totalServerCountChanged();

    /**
     * @brief 当前选中服务器变化信号
     * @param server 新选中的服务器（nullptr 表示清空选择）
     * @details 当选中的服务器被删除并自动切换到其他服务器时发出
     */
    void currentServerChanged(Server* server);

    // ====== 更新相关信号 ======

    /**
     * @brief 更新开始信号
     * @param subscriptionId 开始更新的订阅 ID
     */
    void updateStarted(const QString& subscriptionId);

    /**
     * @brief 更新进度信号
     * @param subscriptionId 正在更新的订阅 ID
     * @param percentage 进度百分比（0-100）
     */
    void updateProgress(const QString& subscriptionId, int percentage);

    /**
     * @brief 更新完成信号
     * @param subscriptionId 完成更新的订阅 ID
     * @param success 是否成功
     * @param serverCount 获得的服务器数量
     */
    void updateCompleted(const QString& subscriptionId, bool success, int serverCount);

    /**
     * @brief 更新失败信号
     * @param subscriptionId 更新失败的订阅 ID
     * @param error 错误描述信息
     */
    void updateFailed(const QString& subscriptionId, const QString& error);

    // ====== 批量更新信号 ======

    /**
     * @brief 批量更新开始信号
     * @param totalCount 总订阅数
     */
    void batchUpdateStarted(int totalCount);

    /**
     * @brief 批量更新进度信号
     * @param current 当前完成数量
     * @param total 总数量
     */
    void batchUpdateProgress(int current, int total);

    /**
     * @brief 批量更新完成信号
     * @param successCount 成功数量
     * @param failedCount 失败数量
     */
    void batchUpdateCompleted(int successCount, int failedCount);

    // ====== 状态信号 ======

    /**
     * @brief 更新状态变化信号
     * @details isUpdating 属性变化时发出
     */
    void isUpdatingChanged();

    // ====== 错误信号 ======

    /**
     * @brief 错误发生信号
     * @param error 错误描述信息
     * @details 用于通用错误通知（不特定于某个操作）
     */
    void errorOccurred(const QString& error);

    // ====== 优惠券和通知信号 ======

    /**
     * @brief 优惠券列表加载成功信号
     * @param coupons 优惠券列表 JSON 数组
     */
    void couponsLoaded(const QJsonArray& coupons);

    /**
     * @brief 优惠券列表加载失败信号
     * @param error 错误描述信息
     */
    void couponsFailed(const QString& error);

    /**
     * @brief 优惠券检查成功信号
     * @param result 优惠券信息 JSON 对象
     */
    void couponChecked(const QJsonObject& result);

    /**
     * @brief 优惠券检查失败信号
     * @param error 错误描述信息
     */
    void couponCheckFailed(const QString& error);

    /**
     * @brief 通知列表加载成功信号
     * @param notices 通知列表 JSON 数组
     */
    void noticesLoaded(const QJsonArray& notices);

    /**
     * @brief 通知列表加载失败信号
     * @param error 错误描述信息
     */
    void noticesFailed(const QString& error);

    /**
     * @brief 通知已读成功信号
     * @param noticeId 通知ID
     */
    void noticeRead(int noticeId);

    /**
     * @brief 通知已读失败信号
     * @param error 错误描述信息
     */
    void noticeReadFailed(const QString& error);

    // ========================================================================
    // 私有部分
    // ========================================================================

private:
    /**
     * @brief 私有构造函数（单例模式）
     * @param parent 父对象指针
     */
    explicit SubscriptionManager(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~SubscriptionManager();

    // 禁用拷贝和赋值（单例模式）
    SubscriptionManager(const SubscriptionManager&) = delete;
    SubscriptionManager& operator=(const SubscriptionManager&) = delete;

    // ========================================================================
    // 私有辅助方法 - 网络和解析
    // ========================================================================

    /**
     * @brief 发起网络请求获取订阅
     * @param subscription 订阅对象指针
     * @details 创建 QNetworkRequest 并发起异步 GET 请求
     */
    void fetchSubscription(Subscription* subscription);

    /**
     * @brief 解析订阅数据（自动检测格式）
     * @param data 原始订阅数据
     * @param subscription 订阅对象指针
     * @return QList<Server*> 解析出的服务器列表
     * @details 自动检测 Base64、JSON、YAML 格式并调用相应解析器
     */
    QList<Server*> parseSubscriptionData(const QByteArray& data, Subscription* subscription);

    /**
     * @brief 解析订阅数据为 JSON 配置列表（不创建 Server 对象）
     * @param data 原始订阅数据
     * @param subscription 订阅对象指针
     * @return QList<QJsonObject> 服务器配置的 JSON 对象列表
     * @details 解析数据但不创建 Server 对象，用于直接写入数据库
     */
    QList<QJsonObject> parseSubscriptionToJson(const QByteArray& data, Subscription* subscription);

    /**
     * @brief 解析 Base64 编码的订阅
     * @param data Base64 编码的数据
     * @param subscription 订阅对象指针
     * @return QList<Server*> 解析出的服务器列表
     * @details 解码后按行分割，逐行解析节点 URI
     */
    QList<Server*> parseBase64Subscription(const QByteArray& data, Subscription* subscription);

    /**
     * @brief 解析 JSON 格式订阅 (SIP008)
     * @param data JSON 格式数据
     * @param subscription 订阅对象指针
     * @return QList<Server*> 解析出的服务器列表
     * @details 支持 Shadowsocks SIP008 标准格式
     */
    QList<Server*> parseJsonSubscription(const QByteArray& data, Subscription* subscription);

    /**
     * @brief 解析 YAML 格式订阅 (Clash)
     * @param data YAML 格式数据
     * @param subscription 订阅对象指针
     * @return QList<Server*> 解析出的服务器列表
     * @details 支持 Clash 配置格式
     */
    QList<Server*> parseYamlSubscription(const QByteArray& data, Subscription* subscription);

    /**
     * @brief 解析 sing-box JSON 格式订阅
     * @param data sing-box JSON 格式数据
     * @param subscription 订阅对象指针
     * @return QList<Server*> 解析出的服务器列表
     * @details 支持 sing-box 配置格式（outbounds 数组）
     */
    QList<Server*> parseSingBoxSubscription(const QByteArray& data, Subscription* subscription);

    /**
     * @brief 从响应头解析订阅信息
     * @param subscription 订阅对象指针
     * @param reply 网络响应对象
     * @details
     * 解析响应头中的订阅信息：
     * - subscription-userinfo: 流量信息
     * - profile-update-interval: 更新间隔
     * - content-disposition: 文件名（作为订阅名称）
     */
    void parseSubscriptionInfo(Subscription* subscription, QNetworkReply* reply);

    /**
     * @brief 检测订阅类型
     * @param data 原始订阅数据
     * @return Subscription::SubscriptionType 订阅类型枚举
     * @details
     * 检测逻辑：
     * - 以 "{" 开头 → JSON
     * - 包含 "proxies:" → YAML
     * - 其他 → Base64
     */
    Subscription::SubscriptionType detectSubscriptionType(const QByteArray& data);

    /**
     * @brief 删除订阅的所有服务器
     * @param subscriptionId 订阅 ID
     * @details 从数据库和内存中删除该订阅的所有服务器
     */
    void removeSubscriptionServers(const QString& subscriptionId);

    /**
     * @brief 检查批量更新是否完成
     * @details
     * 当所有订阅更新完成时：
     * - 发出 batchUpdateCompleted 信号
     * - 重置批量更新状态
     */
    void checkBatchUpdateComplete();

    /**
     * @brief 为指定订阅加载服务器列表（从数据库）
     * @param subscription 订阅对象指针
     * @note 这是一个内部辅助函数，不加锁
     */
    void loadServersForSubscription(Subscription* subscription);

    // ========================================================================
    // 私有辅助方法 - 协议解析
    // ========================================================================

    /**
     * @brief 解析 VMess URI
     * @param uri VMess 分享链接（vmess://...）
     * @return Server* 服务器对象，失败返回 nullptr
     */
    Server* parseVmessUri(const QString& uri);

    /**
     * @brief 解析 VLESS URI
     * @param uri VLESS 分享链接（vless://...）
     * @return Server* 服务器对象，失败返回 nullptr
     */
    Server* parseVlessUri(const QString& uri);

    /**
     * @brief 解析 Shadowsocks URI
     * @param uri Shadowsocks 分享链接（ss://...）
     * @return Server* 服务器对象，失败返回 nullptr
     */
    Server* parseShadowsocksUri(const QString& uri);

    /**
     * @brief 解析 Trojan URI
     * @param uri Trojan 分享链接（trojan://...）
     * @return Server* 服务器对象，失败返回 nullptr
     */
    Server* parseTrojanUri(const QString& uri);

    // ========================================================================
    // 私有槽函数 - 网络回调
    // ========================================================================

private slots:
    /**
     * @brief 网络请求完成回调
     * @details 处理订阅下载完成，解析数据并更新服务器列表
     */
    void onNetworkReplyFinished();

    /**
     * @brief 下载进度回调
     * @param bytesReceived 已接收字节数
     * @param bytesTotal 总字节数
     * @details 计算并发出 updateProgress 信号
     */
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

    /**
     * @brief 网络错误回调
     * @param error 网络错误类型
     * @details 记录错误日志并发出 updateFailed 信号
     */
    void onNetworkError(QNetworkReply::NetworkError error);

    // ========================================================================
    // 私有成员变量
    // ========================================================================

private:
    static SubscriptionManager* s_instance;  ///< 单例实例指针
    static QMutex s_instanceMutex;           ///< 单例创建互斥锁

    DatabaseManager* m_dbManager;            ///< 数据库管理器
    QNetworkAccessManager* m_networkManager; ///< 网络访问管理器

    QList<Subscription*> m_subscriptions;    ///< 订阅列表
    QMap<QPointer<QNetworkReply>, Subscription*> m_activeRequests; ///< 活动的网络请求映射（使用 QPointer 防止野指针）

    mutable QMutex m_mutex;                  ///< 数据访问互斥锁

    // 批量更新状态
    int m_batchUpdateTotal;    ///< 批量更新总数
    int m_batchUpdateCurrent;  ///< 当前完成数
    int m_batchUpdateSuccess;  ///< 成功数量
    int m_batchUpdateFailed;   ///< 失败数量
};

#endif // SUBSCRIPTIONMANAGER_H