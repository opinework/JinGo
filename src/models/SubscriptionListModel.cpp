/**
 * @file SubscriptionListModel.cpp
 * @brief 订阅列表模型实现文件
 * @details 实现订阅列表的数据模型，用于在 QML ListView 中展示订阅数据
 * @author JinGo VPN Team
 * @date 2025
 */

#include "SubscriptionListModel.h"
#include <models/Subscription.h>

// ============================================================================
// 构造和析构
// ============================================================================

/**
 * @brief 构造函数
 * @param parent 父对象指针，用于 Qt 对象树管理
 * @details 初始化空的订阅列表模型
 */
SubscriptionListModel::SubscriptionListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

/**
 * @brief 析构函数
 * @details
 * 清理流程：
 * 1. 删除所有订阅对象
 * 2. 清空列表
 *
 * @note 使用 qDeleteAll 确保所有对象被正确释放
 */
SubscriptionListModel::~SubscriptionListModel()
{
    qDeleteAll(m_subscriptions);
    m_subscriptions.clear();
}

// ============================================================================
// QAbstractListModel 接口实现
// ============================================================================

/**
 * @brief 获取模型中的行数
 * @param parent 父索引（列表模型中通常不使用）
 * @return int 订阅数量
 * @note QAbstractListModel 必须实现的虚函数
 */
int SubscriptionListModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return static_cast<int>(m_subscriptions.count());
}

/**
 * @brief 获取指定索引的数据
 * @param index 模型索引
 * @param role 数据角色（决定返回哪种数据）
 * @return QVariant 请求的数据
 *
 * @details
 * 支持的角色：
 * - SubscriptionRole: 订阅对象指针
 * - IdRole: 订阅 ID
 * - NameRole: 订阅名称
 * - UrlRole: 订阅 URL
 * - ServerCountRole: 服务器数量
 * - IsEnabledRole: 是否启用
 * - LastUpdatedRole: 最后更新时间
 * - FormattedTrafficRole: 格式化的流量字符串
 * - TrafficUsagePercentRole: 流量使用百分比
 * - UpdateStatusRole: 更新状态
 * - TypeRole: 订阅类型
 *
 * @note
 * - 索引无效时返回空 QVariant
 * - 订阅对象为空时返回空 QVariant
 */
QVariant SubscriptionListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_subscriptions.count()) {
        return QVariant();
    }

    Subscription* subscription = m_subscriptions.at(index.row());
    if (!subscription) {
        return QVariant();
    }

    switch (role) {
    case SubscriptionRole:
        return QVariant::fromValue(subscription);
    case IdRole:
        return subscription->id();
    case NameRole:
        return subscription->name();
    case UrlRole:
        return subscription->url();
    case ServerCountRole:
        return subscription->serverCount();
    case IsEnabledRole:
        return subscription->isEnabled();
    case LastUpdatedRole:
        return subscription->lastUpdated();
    case FormattedTrafficRole:
        return subscription->formatTraffic();
    case TrafficUsagePercentRole:
        return subscription->trafficUsagePercent();
    case UpdateStatusRole:
        return subscription->updateStatus();
    case TypeRole:
        return subscription->type();
    default:
        return QVariant();
    }
}

/**
 * @brief 获取角色名称映射
 * @return QHash<int, QByteArray> 角色 ID 到角色名称的映射
 *
 * @details
 * 定义 QML 中可访问的属性名称：
 * - subscription: 订阅对象
 * - id: 订阅 ID
 * - name: 订阅名称
 * - url: 订阅 URL
 * - serverCount: 服务器数量
 * - isEnabled: 是否启用
 * - lastUpdated: 最后更新时间
 * - formattedTraffic: 格式化流量
 * - trafficUsagePercent: 流量使用百分比
 * - updateStatus: 更新状态
 * - type: 订阅类型
 *
 * @note
 * - QML 通过这些名称访问模型数据
 * - QAbstractListModel 必须实现的虚函数
 *
 * @example QML 中使用
 * @code
 * ListView {
 *     model: subscriptionListModel
 *     delegate: Item {
 *         Text { text: name }           // 访问 NameRole
 *         Text { text: serverCount }    // 访问 ServerCountRole
 *         Text { text: formattedTraffic } // 访问 FormattedTrafficRole
 *     }
 * }
 * @endcode
 */
QHash<int, QByteArray> SubscriptionListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[SubscriptionRole] = "subscription";
    roles[IdRole] = "id";
    roles[NameRole] = "name";
    roles[UrlRole] = "url";
    roles[ServerCountRole] = "serverCount";
    roles[IsEnabledRole] = "isEnabled";
    roles[LastUpdatedRole] = "lastUpdated";
    roles[FormattedTrafficRole] = "formattedTraffic";
    roles[TrafficUsagePercentRole] = "trafficUsagePercent";
    roles[UpdateStatusRole] = "updateStatus";
    roles[TypeRole] = "type";
    return roles;
}

// ============================================================================
// 订阅管理
// ============================================================================

/**
 * @brief 添加订阅到模型
 * @param subscription 订阅对象指针
 *
 * @details
 * 添加流程：
 * 1. 验证订阅对象有效性
 * 2. 检查是否已存在（避免重复）
 * 3. 通知视图即将插入行（beginInsertRows）
 * 4. 设置对象的父对象为模型（内存管理）
 * 5. 添加到列表末尾
 * 6. 通知视图插入完成（endInsertRows）
 * 7. 连接订阅对象的信号以自动更新视图
 * 8. 发出 countChanged 信号
 *
 * @note
 * - 订阅对象的所有权转移给模型
 * - 重复添加会被忽略
 * - 自动连接信号实现响应式更新
 *
 * @see removeSubscription(), connectSubscriptionSignals()
 */
void SubscriptionListModel::addSubscription(Subscription* subscription)
{
    if (!subscription) {
        return;
    }

    // 检查是否已存在
    if (m_subscriptions.contains(subscription)) {
        return;
    }

    beginInsertRows(QModelIndex(), static_cast<int>(m_subscriptions.count()), static_cast<int>(m_subscriptions.count()));
    subscription->setParent(this);
    m_subscriptions.append(subscription);
    endInsertRows();

    // 连接信号以便更新视图
    connectSubscriptionSignals(subscription);

    emit countChanged();
}

/**
 * @brief 根据索引删除订阅
 * @param index 订阅在列表中的索引（0-based）
 *
 * @details
 * 删除流程：
 * 1. 验证索引有效性
 * 2. 通知视图即将删除行（beginRemoveRows）
 * 3. 从列表中移除订阅
 * 4. 断开信号连接
 * 5. 延迟删除对象（deleteLater）
 * 6. 通知视图删除完成（endRemoveRows）
 * 7. 发出 countChanged 信号
 *
 * @note
 * - 索引无效时操作被忽略
 * - 使用 deleteLater 确保安全删除
 * - 自动更新视图
 *
 * @see removeSubscriptionById(), disconnectSubscriptionSignals()
 */
void SubscriptionListModel::removeSubscription(int index)
{
    if (index < 0 || index >= m_subscriptions.count()) {
        return;
    }

    beginRemoveRows(QModelIndex(), index, index);
    Subscription* subscription = m_subscriptions.takeAt(index);
    disconnectSubscriptionSignals(subscription);
    subscription->deleteLater();
    endRemoveRows();

    emit countChanged();
}

/**
 * @brief 根据 ID 删除订阅
 * @param id 订阅的唯一标识符
 *
 * @details
 * 查找流程：
 * 1. 遍历所有订阅
 * 2. 匹配 ID
 * 3. 调用 removeSubscription(index) 删除
 *
 * @note
 * - ID 不存在时操作被忽略
 * - 内部调用 removeSubscription(int)
 *
 * @see removeSubscription(int)
 */
void SubscriptionListModel::removeSubscriptionById(const QString& id)
{
    for (int i = 0; i < m_subscriptions.count(); ++i) {
        if (m_subscriptions.at(i)->id() == id) {
            removeSubscription(i);
            return;
        }
    }
}

/**
 * @brief 根据索引获取订阅对象
 * @param index 订阅在列表中的索引
 * @return Subscription* 订阅对象指针，索引无效返回 nullptr
 *
 * @note
 * - 返回的指针由模型管理，不要手动删除
 * - 用于 C++ 代码访问订阅对象
 */
Subscription* SubscriptionListModel::getSubscription(int index) const
{
    if (index >= 0 && index < m_subscriptions.count()) {
        return m_subscriptions.at(index);
    }
    return nullptr;
}

/**
 * @brief 根据 ID 查找订阅对象
 * @param id 订阅的唯一标识符
 * @return Subscription* 订阅对象指针，未找到返回 nullptr
 *
 * @details 遍历列表查找匹配的订阅
 *
 * @note
 * - 返回的指针由模型管理，不要手动删除
 * - 时间复杂度 O(n)
 */
Subscription* SubscriptionListModel::findSubscription(const QString& id) const
{
    for (Subscription* subscription : m_subscriptions) {
        if (subscription && subscription->id() == id) {
            return subscription;
        }
    }
    return nullptr;
}

/**
 * @brief 清空所有订阅
 *
 * @details
 * 清空流程：
 * 1. 检查列表是否为空
 * 2. 通知视图即将重置模型（beginResetModel）
 * 3. 断开所有订阅的信号连接
 * 4. 删除所有订阅对象
 * 5. 清空列表
 * 6. 通知视图重置完成（endResetModel）
 * 7. 发出 countChanged 信号
 *
 * @note
 * - 列表为空时操作被忽略
 * - 使用 beginResetModel/endResetModel 提高性能
 * - 所有订阅对象被删除
 *
 * @see setSubscriptions()
 */
void SubscriptionListModel::clear()
{
    if (m_subscriptions.isEmpty()) {
        return;
    }

    beginResetModel();

    // 断开所有信号连接
    for (Subscription* subscription : m_subscriptions) {
        disconnectSubscriptionSignals(subscription);
    }

    qDeleteAll(m_subscriptions);
    m_subscriptions.clear();
    endResetModel();

    emit countChanged();
}

// ============================================================================
// 属性访问
// ============================================================================

/**
 * @brief 获取订阅数量
 * @return int 模型中的订阅总数
 * @note 用于 QML 属性绑定
 */
int SubscriptionListModel::count() const
{
    return static_cast<int>(m_subscriptions.count());
}

/**
 * @brief 获取所有订阅列表
 * @return QList<Subscription*> 订阅对象指针列表
 * @note
 * - 返回的是列表的副本
 * - 指针仍由模型管理
 */
QList<Subscription*> SubscriptionListModel::subscriptions() const
{
    return m_subscriptions;
}

// ============================================================================
// 数据更新
// ============================================================================

/**
 * @brief 通知视图指定订阅的数据已更改
 * @param index 订阅在列表中的索引
 *
 * @details
 * 发出 dataChanged 信号通知视图刷新该行的显示
 *
 * @note
 * - 通常在订阅属性变更后调用
 * - 视图会自动重新获取数据并刷新显示
 * - 索引无效时操作被忽略
 *
 * @see updateSubscriptionById()
 */
void SubscriptionListModel::updateSubscription(int index)
{
    if (index >= 0 && index < m_subscriptions.count()) {
        QModelIndex modelIndex = this->index(index);
        emit dataChanged(modelIndex, modelIndex);
    }
}

/**
 * @brief 根据 ID 通知视图订阅数据已更改
 * @param id 订阅的唯一标识符
 *
 * @details
 * 查找流程：
 * 1. 遍历列表查找匹配的 ID
 * 2. 调用 updateSubscription(index)
 *
 * @note
 * - ID 不存在时操作被忽略
 * - 内部调用 updateSubscription(int)
 *
 * @see updateSubscription(int)
 */
void SubscriptionListModel::updateSubscriptionById(const QString& id)
{
    for (int i = 0; i < m_subscriptions.count(); ++i) {
        if (m_subscriptions.at(i)->id() == id) {
            updateSubscription(i);
            return;
        }
    }
}

// ============================================================================
// 查询方法
// ============================================================================

/**
 * @brief 获取所有启用的订阅
 * @return QList<Subscription*> 启用的订阅列表
 *
 * @details
 * 筛选条件：subscription->isEnabled() == true
 *
 * @note
 * - 返回新的列表，不影响原列表
 * - 指针仍由模型管理
 * - 可能返回空列表
 */
QList<Subscription*> SubscriptionListModel::enabledSubscriptions() const
{
    QList<Subscription*> enabled;
    for (Subscription* subscription : m_subscriptions) {
        if (subscription && subscription->isEnabled()) {
            enabled.append(subscription);
        }
    }
    return enabled;
}

/**
 * @brief 计算所有订阅的服务器总数
 * @return int 服务器总数
 *
 * @details
 * 遍历所有订阅并累加每个订阅的服务器数量
 *
 * @note
 * - 包括启用和禁用的订阅
 * - 空列表返回 0
 */
int SubscriptionListModel::totalServerCount() const
{
    int total = 0;
    for (Subscription* subscription : m_subscriptions) {
        if (subscription) {
            total += subscription->serverCount();
        }
    }
    return total;
}

/**
 * @brief 检查是否已存在指定 URL 的订阅
 * @param url 订阅 URL
 * @return bool true 表示存在，false 表示不存在
 *
 * @details
 * 用于添加订阅前检查 URL 是否重复
 *
 * @note 时间复杂度 O(n)
 */
bool SubscriptionListModel::hasSubscription(const QString& url) const
{
    for (Subscription* subscription : m_subscriptions) {
        if (subscription && subscription->url() == url) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// 私有辅助方法
// ============================================================================

/**
 * @brief 连接订阅对象的信号
 * @param subscription 订阅对象指针
 *
 * @details
 * 连接的信号：
 * - nameChanged: 名称变更
 * - isEnabledChanged: 启用状态变更
 * - serverCountChanged: 服务器数量变更
 * - lastUpdatedChanged: 最后更新时间变更
 * - trafficChanged: 流量信息变更
 * - updateStatusChanged: 更新状态变更
 *
 * 所有信号都连接到 onSubscriptionDataChanged 槽
 *
 * @note
 * - 实现响应式更新：订阅数据变更自动刷新视图
 * - 在添加订阅时自动调用
 *
 * @see disconnectSubscriptionSignals(), onSubscriptionDataChanged()
 */
void SubscriptionListModel::connectSubscriptionSignals(Subscription* subscription)
{
    if (!subscription) {
        return;
    }

    connect(subscription, &Subscription::nameChanged,
            this, &SubscriptionListModel::onSubscriptionDataChanged);
    connect(subscription, &Subscription::isEnabledChanged,
            this, &SubscriptionListModel::onSubscriptionDataChanged);
    connect(subscription, &Subscription::serverCountChanged,
            this, &SubscriptionListModel::onSubscriptionDataChanged);
    connect(subscription, &Subscription::lastUpdatedChanged,
            this, &SubscriptionListModel::onSubscriptionDataChanged);
    connect(subscription, &Subscription::trafficChanged,
            this, &SubscriptionListModel::onSubscriptionDataChanged);
    connect(subscription, &Subscription::updateStatusChanged,
            this, &SubscriptionListModel::onSubscriptionDataChanged);
}

/**
 * @brief 断开订阅对象的所有信号连接
 * @param subscription 订阅对象指针
 *
 * @details
 * 断开该订阅对象与模型的所有信号连接
 *
 * @note
 * - 在删除订阅前调用，避免访问已删除对象
 * - 使用通配符断开所有连接
 *
 * @see connectSubscriptionSignals()
 */
void SubscriptionListModel::disconnectSubscriptionSignals(Subscription* subscription)
{
    if (!subscription) {
        return;
    }

    disconnect(subscription, nullptr, this, nullptr);
}

/**
 * @brief 订阅数据变更槽函数
 *
 * @details
 * 处理流程：
 * 1. 获取信号发送者（订阅对象）
 * 2. 查找该订阅在列表中的索引
 * 3. 发出 dataChanged 信号通知视图刷新
 *
 * @note
 * - 由订阅对象的各种信号触发
 * - 实现自动更新视图
 * - 发送者不是订阅对象时操作被忽略
 *
 * @see connectSubscriptionSignals()
 */
void SubscriptionListModel::onSubscriptionDataChanged()
{
    Subscription* subscription = qobject_cast<Subscription*>(sender());
    if (!subscription) {
        return;
    }

    int index = static_cast<int>(m_subscriptions.indexOf(subscription));
    if (index >= 0) {
        QModelIndex modelIndex = this->index(index);
        emit dataChanged(modelIndex, modelIndex);
    }
}

// ============================================================================
// 批量操作
// ============================================================================

/**
 * @brief 批量设置订阅列表
 * @param subscriptions 新的订阅列表
 *
 * @details
 * 替换流程：
 * 1. 通知视图即将重置模型（beginResetModel）
 * 2. 断开旧订阅的信号连接
 * 3. 删除旧订阅对象
 * 4. 清空列表
 * 5. 添加新订阅并设置父对象
 * 6. 连接新订阅的信号
 * 7. 通知视图重置完成（endResetModel）
 * 8. 发出 countChanged 信号
 *
 * @note
 * - 旧订阅会被删除
 * - 新订阅的所有权转移给模型
 * - 使用 beginResetModel/endResetModel 提高性能
 * - 空订阅对象会被跳过
 *
 * @see clear(), addSubscription()
 */
void SubscriptionListModel::setSubscriptions(const QList<Subscription*>& subscriptions)
{
    beginResetModel();

    // 清理旧数据
    for (Subscription* subscription : m_subscriptions) {
        disconnectSubscriptionSignals(subscription);
        subscription->deleteLater();
    }
    m_subscriptions.clear();

    // 添加新数据
    for (Subscription* subscription : subscriptions) {
        if (subscription) {
            subscription->setParent(this);
            m_subscriptions.append(subscription);
            connectSubscriptionSignals(subscription);
        }
    }

    endResetModel();
    emit countChanged();
}

/**
 * @brief 移动订阅位置
 * @param fromIndex 源索引
 * @param toIndex 目标索引
 *
 * @details
 * 移动流程：
 * 1. 验证索引有效性
 * 2. 检查源索引和目标索引是否相同
 * 3. 计算调整后的目标索引（Qt 要求）
 * 4. 通知视图即将移动行（beginMoveRows）
 * 5. 在列表中移动订阅
 * 6. 通知视图移动完成（endMoveRows）
 *
 * @note
 * - 索引无效时操作被忽略
 * - 源索引和目标索引相同时操作被忽略
 * - 从上往下移动时，目标索引需要 +1
 * - 视图自动更新显示顺序
 *
 * @warning
 * Qt 的 beginMoveRows 对目标索引有特殊要求：
 * - 向下移动时，toIndex 需要 +1
 * - 向上移动时，toIndex 保持不变
 */
void SubscriptionListModel::moveSubscription(int fromIndex, int toIndex)
{
    if (fromIndex < 0 || fromIndex >= m_subscriptions.count() ||
        toIndex < 0 || toIndex >= m_subscriptions.count() ||
        fromIndex == toIndex) {
        return;
    }

    // Qt的beginMoveRows要求toIndex是目标位置
    // 如果从上往下移动，toIndex需要+1
    int adjustedToIndex = toIndex;
    if (fromIndex < toIndex) {
        adjustedToIndex++;
    }

    beginMoveRows(QModelIndex(), fromIndex, fromIndex, QModelIndex(), adjustedToIndex);
    m_subscriptions.move(fromIndex, toIndex);
    endMoveRows();
}