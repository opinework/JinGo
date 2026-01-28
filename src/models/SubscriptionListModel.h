/**
 * @file SubscriptionListModel.h
 * @brief 订阅列表模型头文件
 * @details 提供订阅数据的列表模型，用于在 QML ListView/Repeater 中展示和管理订阅
 * @author JinGo VPN Team
 * @date 2025
 */

#ifndef SUBSCRIPTIONLISTMODEL_H
#define SUBSCRIPTIONLISTMODEL_H

#include <QAbstractListModel>
#include <QList>

// 前向声明
class Subscription;

// ============================================================================
// SubscriptionListModel 类定义
// ============================================================================

/**
 * @class SubscriptionListModel
 * @brief 订阅列表模型（继承自 QAbstractListModel）
 *
 * @details
 * 核心功能：
 * - 数据展示：为 QML ListView 提供订阅数据
 * - 订阅管理：添加、删除、查找订阅
 * - 响应式更新：订阅数据变更自动刷新视图
 * - 批量操作：批量设置、移动订阅
 * - 查询过滤：获取启用的订阅、统计服务器数量
 *
 * 设计特点：
 * - Qt Model/View 架构：标准的列表模型实现
 * - 自动信号连接：订阅属性变更自动触发视图更新
 * - 内存管理：模型负责订阅对象的生命周期
 * - QML 友好：使用 Q_INVOKABLE 和角色名称映射
 *
 * 支持的数据角色：
 * - SubscriptionRole: 订阅对象指针
 * - IdRole: 订阅 ID
 * - NameRole: 订阅名称
 * - UrlRole: 订阅 URL
 * - IsEnabledRole: 是否启用
 * - ServerCountRole: 服务器数量
 * - LastUpdatedRole: 最后更新时间
 * - FormattedTrafficRole: 格式化的流量字符串
 * - TrafficUsagePercentRole: 流量使用百分比
 * - UpdateStatusRole: 更新状态
 * - TypeRole: 订阅类型
 *
 * @note
 * - 线程安全：仅在主线程中使用
 * - 内存管理：订阅对象由模型管理，自动释放
 * - 响应式：订阅属性变更自动更新视图
 *
 * @example C++ 使用示例
 * @code
 * // 创建模型
 * SubscriptionListModel* model = new SubscriptionListModel(this);
 *
 * // 添加订阅
 * Subscription* sub = new Subscription();
 * sub->setName("我的订阅");
 * sub->setUrl("https://example.com/subscribe");
 * model->addSubscription(sub);
 *
 * // 获取订阅数量
 * int count = model->count();
 *
 * // 查找订阅
 * Subscription* found = model->findSubscription("subscription-id");
 * @endcode
 *
 * @example QML 使用示例
 * @code
 * import QtQuick 2.15
 * import QtQuick.Controls 2.15
 *
 * ListView {
 *     model: subscriptionListModel  // 从 C++ 注册的模型
 *
 *     delegate: Item {
 *         width: ListView.view.width
 *         height: 80
 *
 *         Column {
 *             Text { text: name }  // NameRole
 *             Text { text: "服务器: " + serverCount }  // ServerCountRole
 *             Text { text: formattedTraffic }  // FormattedTrafficRole
 *             ProgressBar {
 *                 value: trafficUsagePercent / 100  // TrafficUsagePercentRole
 *             }
 *         }
 *
 *         Switch {
 *             checked: isEnabled  // IsEnabledRole
 *             onToggled: subscription.setEnabled(checked)  // SubscriptionRole
 *         }
 *     }
 *
 *     Text {
 *         text: "总订阅数: " + model.count
 *     }
 * }
 *
 * // 调用模型方法
 * Button {
 *     text: "清空所有"
 *     onClicked: subscriptionListModel.clear()
 * }
 *
 * Button {
 *     text: "获取启用的订阅"
 *     onClicked: {
 *         var enabled = subscriptionListModel.enabledSubscriptions()
 *         console.log("启用的订阅数:", enabled.length)
 *     }
 * }
 * @endcode
 */
class SubscriptionListModel : public QAbstractListModel
{
    Q_OBJECT

    // ========================================================================
    // QML 属性
    // ========================================================================

    /**
     * @property count
     * @brief 订阅数量（只读属性）
     * @details 返回模型中的订阅总数
     * @notify countChanged()
     */
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    // ========================================================================
    // 枚举类型 - 数据角色
    // ========================================================================

    /**
     * @enum SubscriptionRoles
     * @brief 订阅数据角色枚举
     * @details 定义模型中可访问的数据类型，用于 data() 方法和 QML 绑定
     */
    enum SubscriptionRoles {
        SubscriptionRole = Qt::UserRole + 1,  ///< 订阅对象指针（Subscription*）
        IdRole,                                ///< 订阅 ID（QString）
        NameRole,                              ///< 订阅名称（QString）
        UrlRole,                               ///< 订阅 URL（QString）
        IsEnabledRole,                         ///< 是否启用（bool）
        ServerCountRole,                       ///< 服务器数量（int）
        LastUpdatedRole,                       ///< 最后更新时间（QDateTime）
        FormattedTrafficRole,                  ///< 格式化的流量字符串（QString）
        TrafficUsagePercentRole,               ///< 流量使用百分比（double）
        UpdateStatusRole,                      ///< 更新状态（QString）
        TypeRole                               ///< 订阅类型（QString 或 enum）
    };
    Q_ENUM(SubscriptionRoles)

    // ========================================================================
    // 构造和析构
    // ========================================================================

    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    explicit SubscriptionListModel(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     * @details 自动清理所有订阅对象
     */
    ~SubscriptionListModel() override;

    // ========================================================================
    // QAbstractListModel 接口实现
    // ========================================================================

    /**
     * @brief 获取模型行数
     * @param parent 父索引（列表模型中通常不使用）
     * @return int 订阅数量
     * @note QAbstractListModel 必须实现的虚函数
     */
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief 获取指定索引和角色的数据
     * @param index 模型索引
     * @param role 数据角色（默认 Qt::DisplayRole）
     * @return QVariant 请求的数据
     * @note QAbstractListModel 必须实现的虚函数
     * @see SubscriptionRoles
     */
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    /**
     * @brief 获取角色名称映射
     * @return QHash<int, QByteArray> 角色 ID 到名称的映射
     * @details
     * 定义 QML 中可访问的属性名称：
     * - subscription: 订阅对象
     * - id: 订阅 ID
     * - name: 订阅名称
     * - url: 订阅 URL
     * - isEnabled: 是否启用
     * - serverCount: 服务器数量
     * - lastUpdated: 最后更新时间
     * - formattedTraffic: 格式化流量
     * - trafficUsagePercent: 流量使用百分比
     * - updateStatus: 更新状态
     * - type: 订阅类型
     * @note QAbstractListModel 必须实现的虚函数
     */
    QHash<int, QByteArray> roleNames() const override;

    // ========================================================================
    // 订阅管理方法
    // ========================================================================

    /**
     * @brief 添加订阅到模型
     * @param subscription 订阅对象指针
     * @details
     * - 订阅对象的所有权转移给模型
     * - 自动连接信号实现响应式更新
     * - 重复添加会被忽略
     * @note
     * - 空指针会被忽略
     * - 添加后发出 countChanged() 信号
     * @see removeSubscription(), clear()
     */
    Q_INVOKABLE void addSubscription(Subscription* subscription);

    /**
     * @brief 根据索引删除订阅
     * @param index 订阅在列表中的索引（0-based）
     * @details
     * - 订阅对象会被删除
     * - 自动断开信号连接
     * - 视图自动更新
     * @note
     * - 索引无效时操作被忽略
     * - 删除后发出 countChanged() 信号
     * @see removeSubscriptionById(), addSubscription()
     */
    Q_INVOKABLE void removeSubscription(int index);

    /**
     * @brief 根据 ID 删除订阅
     * @param id 订阅的唯一标识符
     * @details 内部调用 removeSubscription(int)
     * @note ID 不存在时操作被忽略
     * @see removeSubscription(int)
     */
    Q_INVOKABLE void removeSubscriptionById(const QString& id);

    /**
     * @brief 清空所有订阅
     * @details
     * - 删除所有订阅对象
     * - 断开所有信号连接
     * - 视图自动更新
     * @note
     * - 列表为空时操作被忽略
     * - 清空后发出 countChanged() 信号
     * @see setSubscriptions()
     */
    Q_INVOKABLE void clear();

    /**
     * @brief 根据索引获取订阅对象
     * @param index 订阅在列表中的索引
     * @return Subscription* 订阅对象指针，索引无效返回 nullptr
     * @note
     * - 返回的指针由模型管理，不要手动删除
     * - 用于 C++ 和 QML 代码访问订阅对象
     * @see findSubscription()
     */
    Q_INVOKABLE Subscription* getSubscription(int index) const;

    /**
     * @brief 根据 ID 查找订阅对象
     * @param id 订阅的唯一标识符
     * @return Subscription* 订阅对象指针，未找到返回 nullptr
     * @note
     * - 返回的指针由模型管理，不要手动删除
     * - 时间复杂度 O(n)
     * @see getSubscription()
     */
    Q_INVOKABLE Subscription* findSubscription(const QString& id) const;

    // ========================================================================
    // 批量操作方法
    // ========================================================================

    /**
     * @brief 批量设置订阅列表
     * @param subscriptions 新的订阅列表
     * @details
     * - 旧订阅会被删除
     * - 新订阅的所有权转移给模型
     * - 使用 beginResetModel/endResetModel 提高性能
     * - 空订阅对象会被跳过
     * @note
     * - 列表完全替换
     * - 设置后发出 countChanged() 信号
     * @see clear(), addSubscription()
     */
    void setSubscriptions(const QList<Subscription*>& subscriptions);

    /**
     * @brief 移动订阅位置
     * @param fromIndex 源索引
     * @param toIndex 目标索引
     * @details
     * - 改变订阅在列表中的顺序
     * - 视图自动更新显示顺序
     * @note
     * - 索引无效时操作被忽略
     * - 源索引和目标索引相同时操作被忽略
     * @warning
     * - 从上往下移动时，内部会自动调整目标索引
     */
    Q_INVOKABLE void moveSubscription(int fromIndex, int toIndex);

    // ========================================================================
    // 数据更新方法
    // ========================================================================

    /**
     * @brief 通知视图指定订阅的数据已更改
     * @param index 订阅在列表中的索引
     * @details 发出 dataChanged 信号，视图会重新获取数据并刷新显示
     * @note
     * - 通常在订阅属性变更后调用
     * - 索引无效时操作被忽略
     * @see updateSubscriptionById()
     */
    void updateSubscription(int index);

    /**
     * @brief 根据 ID 通知视图订阅数据已更改
     * @param id 订阅的唯一标识符
     * @details 内部调用 updateSubscription(int)
     * @note ID 不存在时操作被忽略
     * @see updateSubscription(int)
     */
    void updateSubscriptionById(const QString& id);

    // ========================================================================
    // 属性访问方法
    // ========================================================================

    /**
     * @brief 获取订阅数量
     * @return int 模型中的订阅总数
     * @note 用于 QML 属性绑定 (count 属性)
     */
    int count() const;

    /**
     * @brief 获取所有订阅列表
     * @return QList<Subscription*> 订阅对象指针列表
     * @note
     * - 返回的是列表的副本
     * - 指针仍由模型管理，不要手动删除
     */
    QList<Subscription*> subscriptions() const;

    // ========================================================================
    // 查询方法
    // ========================================================================

    /**
     * @brief 获取所有启用的订阅
     * @return QList<Subscription*> 启用的订阅列表
     * @details 筛选条件：subscription->isEnabled() == true
     * @note
     * - 返回新的列表，不影响原列表
     * - 指针仍由模型管理
     * - 可能返回空列表
     */
    Q_INVOKABLE QList<Subscription*> enabledSubscriptions() const;

    /**
     * @brief 计算所有订阅的服务器总数
     * @return int 服务器总数
     * @details 遍历所有订阅并累加每个订阅的服务器数量
     * @note
     * - 包括启用和禁用的订阅
     * - 空列表返回 0
     */
    Q_INVOKABLE int totalServerCount() const;

    /**
     * @brief 检查是否已存在指定 URL 的订阅
     * @param url 订阅 URL
     * @return bool true 表示存在，false 表示不存在
     * @details 用于添加订阅前检查 URL 是否重复
     * @note 时间复杂度 O(n)
     */
    Q_INVOKABLE bool hasSubscription(const QString& url) const;

    // ========================================================================
    // 信号定义
    // ========================================================================

signals:
    /**
     * @brief 订阅数量变化信号
     * @details
     * 触发时机：
     * - 添加订阅
     * - 删除订阅
     * - 清空列表
     * - 批量设置订阅
     */
    void countChanged();

    // ========================================================================
    // 私有槽函数
    // ========================================================================

private slots:
    /**
     * @brief 订阅数据变更槽函数
     * @details
     * - 由订阅对象的各种信号触发
     * - 自动查找订阅在列表中的位置
     * - 发出 dataChanged 信号更新视图
     * @note 实现响应式更新机制
     * @see connectSubscriptionSignals()
     */
    void onSubscriptionDataChanged();

    // ========================================================================
    // 私有辅助方法
    // ========================================================================

private:
    /**
     * @brief 连接订阅对象的信号
     * @param subscription 订阅对象指针
     * @details
     * 连接的信号：
     * - nameChanged
     * - isEnabledChanged
     * - serverCountChanged
     * - lastUpdatedChanged
     * - trafficChanged
     * - updateStatusChanged
     * @note
     * - 实现响应式更新
     * - 在添加订阅时自动调用
     * @see disconnectSubscriptionSignals(), onSubscriptionDataChanged()
     */
    void connectSubscriptionSignals(Subscription* subscription);

    /**
     * @brief 断开订阅对象的所有信号连接
     * @param subscription 订阅对象指针
     * @details 在删除订阅前调用，避免访问已删除对象
     * @see connectSubscriptionSignals()
     */
    void disconnectSubscriptionSignals(Subscription* subscription);

    // ========================================================================
    // 私有成员变量
    // ========================================================================

private:
    QList<Subscription*> m_subscriptions;  ///< 订阅列表
};

#endif // SUBSCRIPTIONLISTMODEL_H