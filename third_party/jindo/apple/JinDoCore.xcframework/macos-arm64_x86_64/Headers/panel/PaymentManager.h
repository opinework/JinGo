/**
 * @file PaymentManager.h
 * @brief 支付管理器头文件
 * @details 提供支付方式查询、支付结账等完整功能
 * @author JinGo VPN Team
 * @date 2025
 */

#ifndef PAYMENTMANAGER_H
#define PAYMENTMANAGER_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

// 前向声明
class ApiClient;

// ============================================================================
// PaymentManager 类定义
// ============================================================================

/**
 * @class PaymentManager
 * @brief 支付管理器（单例模式）
 *
 * @details
 * 核心功能：
 * - 支付方式：获取可用支付方式
 * - 支付结账：发起支付流程
 * - 支付回调：处理支付结果通知
 *
 * @example C++ 使用示例
 * @code
 * // 获取支付方式
 * PaymentManager::instance().fetchPaymentMethods();
 *
 * // 发起支付
 * PaymentManager::instance().checkout(orderId, paymentMethod);
 * @endcode
 */
class PaymentManager : public QObject
{
    Q_OBJECT

    // ========================================================================
    // QML 属性
    // ========================================================================

    /**
     * @property isProcessing
     * @brief 是否正在处理支付操作（只读属性）
     * @notify isProcessingChanged()
     */
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)

public:
    // ========================================================================
    // 单例访问
    // ========================================================================

    /**
     * @brief 获取 PaymentManager 单例实例
     * @return PaymentManager& 全局唯一实例的引用
     */
    static PaymentManager& instance();

    // ========================================================================
    // 支付操作方法
    // ========================================================================

    /**
     * @brief 获取支付方式列表
     *
     * @details
     * 查询流程：
     * 1. 验证是否已登录
     * 2. 发送 GET 请求到 /user/payment/methods
     * 3. 等待响应并触发相应信号
     *
     * 响应信号：
     * - 成功：paymentMethodsLoaded(QJsonArray methods)
     * - 失败：paymentMethodsFailed(QString error)
     */
    Q_INVOKABLE void fetchPaymentMethods();

    /**
     * @brief 支付结账
     * @param tradeNo 订单号
     * @param method 支付方式ID
     *
     * @details
     * 支付流程：
     * 1. 验证是否已登录
     * 2. 构建支付请求
     * 3. 发送 POST 请求到 /user/order/checkout
     * 4. 等待响应并触发相应信号
     *
     * 响应信号：
     * - 成功：checkoutSuccess(QJsonObject result)
     * - 失败：checkoutFailed(QString error)
     */
    Q_INVOKABLE void checkout(const QString& tradeNo, int method);

    /**
     * @brief 检查是否正在处理
     * @return bool
     */
    bool isProcessing() const;

signals:
    /**
     * @brief 支付方式加载成功信号
     * @param methods 支付方式列表 JSON 数组
     */
    void paymentMethodsLoaded(const QJsonArray& methods);

    /**
     * @brief 支付方式加载失败信号
     * @param error 错误描述信息
     */
    void paymentMethodsFailed(const QString& error);

    /**
     * @brief 支付结账成功信号
     * @param result 支付结果 JSON 对象（包含支付URL或二维码等）
     */
    void checkoutSuccess(const QJsonObject& result);

    /**
     * @brief 支付结账失败信号
     * @param error 错误描述信息
     */
    void checkoutFailed(const QString& error);

    /**
     * @brief 处理状态变化信号
     */
    void isProcessingChanged();

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    PaymentManager(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~PaymentManager();

    // 禁用拷贝和赋值
    PaymentManager(const PaymentManager&) = delete;
    PaymentManager& operator=(const PaymentManager&) = delete;

    /**
     * @brief 设置处理状态
     */
    void setProcessing(bool processing);

private slots:
    void onPaymentMethodsSuccess(const QJsonObject& response);
    void onPaymentMethodsError(const QString& error);
    void onCheckoutSuccess(const QJsonObject& response);
    void onCheckoutError(const QString& error);

private:
    ApiClient& m_apiClient;
    bool m_isProcessing;
};

#endif // PAYMENTMANAGER_H
