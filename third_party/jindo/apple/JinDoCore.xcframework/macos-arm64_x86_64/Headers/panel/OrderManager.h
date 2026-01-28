/**
 * @file OrderManager.h
 * @brief 订单管理器头文件
 * @details 提供订单创建、查询、取消等完整功能
 * @author JinGo VPN Team
 * @date 2025
 */

#ifndef ORDERMANAGER_H
#define ORDERMANAGER_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

// 前向声明
class ApiClient;

// ============================================================================
// OrderManager 类定义
// ============================================================================

/**
 * @class OrderManager
 * @brief 订单管理器（单例模式）
 *
 * @details
 * 核心功能：
 * - 订单创建：创建新订单
 * - 订单查询：查询订单列表和详情
 * - 订单取消：取消待支付订单
 * - 订单状态：检查订单状态
 *
 * 架构特点：
 * - 单例设计：全局唯一实例
 * - QML 集成：支持 Q_INVOKABLE 和 Q_PROPERTY
 * - 异步操作：所有网络请求都是异步的
 * - 信号通知：操作结果通过信号发送
 *
 * @example C++ 使用示例
 * @code
 * // 连接信号
 * connect(&OrderManager::instance(), &OrderManager::orderCreated,
 *         this, &MyClass::onOrderCreated);
 *
 * // 创建订单
 * OrderManager::instance().createOrder(planId, period, couponCode);
 * @endcode
 */
class OrderManager : public QObject
{
    Q_OBJECT

    // ========================================================================
    // QML 属性
    // ========================================================================

    /**
     * @property isProcessing
     * @brief 是否正在处理订单操作（只读属性）
     * @notify isProcessingChanged()
     */
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)

public:
    // ========================================================================
    // 单例访问
    // ========================================================================

    /**
     * @brief 获取 OrderManager 单例实例
     * @return OrderManager& 全局唯一实例的引用
     * @note 线程安全（C++11 静态局部变量保证）
     */
    static OrderManager& instance();

    // ========================================================================
    // 订单操作方法
    // ========================================================================

    /**
     * @brief 创建订单
     * @param planId 套餐ID
     * @param period 订阅周期（可选，如 "month_price", "quarter_price", "year_price"）
     * @param couponCode 优惠券代码（可选）
     *
     * @details
     * 创建流程：
     * 1. 验证是否已登录
     * 2. 构建请求参数
     * 3. 发送 POST 请求到 /user/order/save
     * 4. 等待响应并触发相应信号
     *
     * 响应信号：
     * - 成功：orderCreated(QJsonObject order)
     * - 失败：orderFailed(QString error)
     *
     * @see orderCreated(), orderFailed()
     */
    Q_INVOKABLE void createOrder(int planId, const QString& period = QString(), const QString& couponCode = QString());

    /**
     * @brief 获取订单列表
     * @param page 页码（默认1）
     * @param pageSize 每页数量（默认20）
     *
     * @details
     * 查询流程：
     * 1. 验证是否已登录
     * 2. 发送 GET 请求到 /user/order/fetch
     * 3. 等待响应并触发相应信号
     *
     * 响应信号：
     * - 成功：ordersLoaded(QJsonArray orders)
     * - 失败：ordersFailed(QString error)
     *
     * @see ordersLoaded(), ordersFailed()
     */
    Q_INVOKABLE void fetchOrders(int page = 1, int pageSize = 20);

    /**
     * @brief 获取订单详情
     * @param orderId 订单ID或订单号
     *
     * @details
     * 查询流程：
     * 1. 验证是否已登录
     * 2. 发送 GET 请求到 /user/order/detail
     * 3. 等待响应并触发相应信号
     *
     * 响应信号：
     * - 成功：orderDetailLoaded(QJsonObject order)
     * - 失败：orderDetailFailed(QString error)
     *
     * @see orderDetailLoaded(), orderDetailFailed()
     */
    Q_INVOKABLE void getOrderDetail(const QString& orderId);

    /**
     * @brief 取消订单
     * @param orderId 订单ID或订单号
     *
     * @details
     * 取消流程：
     * 1. 验证是否已登录
     * 2. 发送 POST 请求到 /user/order/cancel
     * 3. 等待响应并触发相应信号
     *
     * 响应信号：
     * - 成功：orderCancelled(QString orderId)
     * - 失败：orderCancelFailed(QString error)
     *
     * @note 只能取消待支付状态的订单
     *
     * @see orderCancelled(), orderCancelFailed()
     */
    Q_INVOKABLE void cancelOrder(const QString& orderId);

    /**
     * @brief 检查订单状态
     * @param orderId 订单ID或订单号
     *
     * @details
     * 检查流程：
     * 1. 验证是否已登录
     * 2. 发送 GET 请求到 /user/order/check
     * 3. 等待响应并触发相应信号
     *
     * 响应信号：
     * - 成功：orderStatusChecked(QJsonObject status)
     * - 失败：orderCheckFailed(QString error)
     *
     * @see orderStatusChecked(), orderCheckFailed()
     */
    Q_INVOKABLE void checkOrderStatus(const QString& orderId);

    /**
     * @brief 获取支付链接
     * @param tradeNo 订单交易号
     * @param paymentMethod 支付方式 (如 "alipay", "wechat", "stripe")
     *
     * @details
     * 获取流程：
     * 1. 验证是否已登录
     * 2. 发送 POST 请求到 /user/order/checkout
     * 3. 等待响应并触发相应信号
     *
     * 响应信号：
     * - 成功：paymentUrlReady(QString url, QString type)
     * - 失败：paymentFailed(QString error)
     */
    Q_INVOKABLE void getPaymentUrl(const QString& tradeNo, const QString& paymentMethod);

    /**
     * @brief 获取可用支付方式
     *
     * @details
     * 响应信号：
     * - 成功：paymentMethodsLoaded(QJsonArray methods)
     * - 失败：paymentMethodsFailed(QString error)
     */
    Q_INVOKABLE void fetchPaymentMethods();

    /**
     * @brief 检查是否正在处理
     * @return bool true 表示正在处理，false 表示空闲
     */
    bool isProcessing() const;

    // ========================================================================
    // 信号定义
    // ========================================================================

signals:
    /**
     * @brief 订单创建成功信号
     * @param order 订单信息 JSON 对象
     */
    void orderCreated(const QJsonObject& order);

    /**
     * @brief 订单操作失败信号
     * @param error 错误描述信息
     */
    void orderFailed(const QString& error);

    /**
     * @brief 订单列表加载成功信号
     * @param orders 订单列表 JSON 数组
     */
    void ordersLoaded(const QJsonArray& orders);

    /**
     * @brief 订单列表加载失败信号
     * @param error 错误描述信息
     */
    void ordersFailed(const QString& error);

    /**
     * @brief 订单详情加载成功信号
     * @param order 订单详情 JSON 对象
     */
    void orderDetailLoaded(const QJsonObject& order);

    /**
     * @brief 订单详情加载失败信号
     * @param error 错误描述信息
     */
    void orderDetailFailed(const QString& error);

    /**
     * @brief 订单取消成功信号
     * @param orderId 订单ID
     */
    void orderCancelled(const QString& orderId);

    /**
     * @brief 订单取消失败信号
     * @param error 错误描述信息
     */
    void orderCancelFailed(const QString& error);

    /**
     * @brief 订单状态检查成功信号
     * @param status 订单状态信息 JSON 对象
     */
    void orderStatusChecked(const QJsonObject& status);

    /**
     * @brief 订单状态检查失败信号
     * @param error 错误描述信息
     */
    void orderCheckFailed(const QString& error);

    /**
     * @brief 支付链接获取成功信号
     * @param url 支付链接 URL
     * @param type 支付类型 (如 "redirect", "qrcode")
     */
    void paymentUrlReady(const QString& url, const QString& type);

    /**
     * @brief 支付操作失败信号
     * @param error 错误描述信息
     */
    void paymentFailed(const QString& error);

    /**
     * @brief 支付方式列表加载成功信号
     * @param methods 支付方式列表
     */
    void paymentMethodsLoaded(const QJsonArray& methods);

    /**
     * @brief 支付方式列表加载失败信号
     * @param error 错误描述信息
     */
    void paymentMethodsFailed(const QString& error);

    /**
     * @brief 处理状态变化信号
     */
    void isProcessingChanged();

private:
    /**
     * @brief 私有构造函数（单例模式）
     * @param parent 父对象指针
     */
    OrderManager(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~OrderManager();

    // 禁用拷贝和赋值（单例模式）
    OrderManager(const OrderManager&) = delete;
    OrderManager& operator=(const OrderManager&) = delete;

    /**
     * @brief 设置处理状态
     * @param processing 是否正在处理
     */
    void setProcessing(bool processing);

    // ========================================================================
    // 私有槽函数 - 网络请求回调
    // ========================================================================

private slots:
    /**
     * @brief 创建订单成功回调
     * @param response 服务器返回的 JSON 响应
     */
    void onCreateOrderSuccess(const QJsonObject& response);

    /**
     * @brief 创建订单失败回调
     * @param error 错误描述信息
     */
    void onCreateOrderError(const QString& error);

    /**
     * @brief 获取订单列表成功回调
     * @param response 服务器返回的 JSON 响应
     */
    void onFetchOrdersSuccess(const QJsonObject& response);

    /**
     * @brief 获取订单列表失败回调
     * @param error 错误描述信息
     */
    void onFetchOrdersError(const QString& error);

    /**
     * @brief 获取订单详情成功回调
     * @param response 服务器返回的 JSON 响应
     */
    void onOrderDetailSuccess(const QJsonObject& response);

    /**
     * @brief 获取订单详情失败回调
     * @param error 错误描述信息
     */
    void onOrderDetailError(const QString& error);

    /**
     * @brief 取消订单成功回调
     * @param response 服务器返回的 JSON 响应
     */
    void onCancelOrderSuccess(const QJsonObject& response);

    /**
     * @brief 取消订单失败回调
     * @param error 错误描述信息
     */
    void onCancelOrderError(const QString& error);

    /**
     * @brief 检查订单状态成功回调
     * @param response 服务器返回的 JSON 响应
     */
    void onCheckStatusSuccess(const QJsonObject& response);

    /**
     * @brief 检查订单状态失败回调
     * @param error 错误描述信息
     */
    void onCheckStatusError(const QString& error);

    /**
     * @brief 获取支付链接成功回调
     * @param response 服务器返回的 JSON 响应
     */
    void onPaymentUrlSuccess(const QJsonObject& response);

    /**
     * @brief 获取支付链接失败回调
     * @param error 错误描述信息
     */
    void onPaymentUrlError(const QString& error);

    /**
     * @brief 获取支付方式成功回调
     * @param response 服务器返回的 JSON 响应
     */
    void onPaymentMethodsSuccess(const QJsonObject& response);

    /**
     * @brief 获取支付方式失败回调
     * @param error 错误描述信息
     */
    void onPaymentMethodsError(const QString& error);

    // ========================================================================
    // 私有成员变量
    // ========================================================================

private:
    ApiClient& m_apiClient;       ///< API 客户端引用（单例）
    bool m_isProcessing;          ///< 是否正在处理订单操作
    QString m_pendingOrderId;     ///< 待处理的订单ID（用于取消等操作）
};

#endif // ORDERMANAGER_H
