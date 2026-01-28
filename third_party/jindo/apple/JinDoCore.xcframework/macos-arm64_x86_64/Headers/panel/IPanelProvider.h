/**
 * @file IPanelProvider.h
 * @brief 面板提供者接口定义
 * @details 定义面板对接的统一接口，支持多种面板系统（XBoard、V2Board、SSPanel等）
 * @author JinGo VPN Team
 * @date 2025
 */

#ifndef IPANELPROVIDER_H
#define IPANELPROVIDER_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <functional>

/**
 * @class IPanelProvider
 * @brief 面板提供者接口（抽象基类）
 *
 * @details
 * 定义了与面板系统交互的标准接口，包括：
 * - 用户认证：登录、注册、登出
 * - 订阅管理：获取订阅信息、订阅链接
 * - 订单管理：创建订单、查询订单
 * - 支付管理：获取支付方式、支付链接
 * - 工单系统：创建工单、查询工单
 * - 系统配置：获取面板配置
 *
 * 实现方式：
 * - 子类实现具体面板的 API 调用
 * - 通过 PanelManager 统一管理
 * - 支持运行时切换面板
 *
 * 支持的面板：
 * - XBoard (默认)
 * - V2Board
 * - SSPanel-UIM
 * - 自定义面板
 */
class IPanelProvider : public QObject
{
    Q_OBJECT

public:
    // 回调类型定义
    using SuccessCallback = std::function<void(const QJsonObject&)>;
    using ErrorCallback = std::function<void(const QString&)>;
    using ArrayCallback = std::function<void(const QJsonArray&)>;

    /**
     * @enum PanelType
     * @brief 面板类型枚举
     */
    enum PanelType {
        XBoard,      ///< XBoard 面板
        V2Board,     ///< V2Board 面板
        SSPanel,     ///< SSPanel-UIM 面板
        Custom       ///< 自定义面板
    };
    Q_ENUM(PanelType)

    explicit IPanelProvider(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IPanelProvider() = default;

    // ========================================================================
    // 面板信息
    // ========================================================================

    /**
     * @brief 获取面板类型
     * @return PanelType 面板类型枚举值
     */
    virtual PanelType panelType() const = 0;

    /**
     * @brief 获取面板名称
     * @return QString 面板显示名称
     */
    virtual QString panelName() const = 0;

    /**
     * @brief 获取面板版本
     * @return QString 面板版本号
     */
    virtual QString panelVersion() const = 0;

    // ========================================================================
    // 配置
    // ========================================================================

    /**
     * @brief 设置面板 API 基础 URL
     * @param url API 基础地址
     */
    virtual void setBaseUrl(const QString& url) = 0;

    /**
     * @brief 获取面板 API 基础 URL
     * @return QString API 基础地址
     */
    virtual QString baseUrl() const = 0;

    /**
     * @brief 设置认证 Token
     * @param token JWT 或其他格式的认证令牌
     */
    virtual void setAuthToken(const QString& token) = 0;

    /**
     * @brief 获取认证 Token
     * @return QString 当前 Token
     */
    virtual QString authToken() const = 0;

    // ========================================================================
    // 用户认证 API
    // ========================================================================

    /**
     * @brief 用户登录
     * @param email 邮箱
     * @param password 密码
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void login(const QString& email,
                       const QString& password,
                       SuccessCallback onSuccess,
                       ErrorCallback onError) = 0;

    /**
     * @brief 用户注册
     * @param email 邮箱
     * @param password 密码
     * @param inviteCode 邀请码（可选）
     * @param emailCode 邮箱验证码（可选）
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void register_(const QString& email,
                           const QString& password,
                           const QString& inviteCode,
                           const QString& emailCode,
                           SuccessCallback onSuccess,
                           ErrorCallback onError) = 0;

    /**
     * @brief 用户登出
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void logout(SuccessCallback onSuccess,
                        ErrorCallback onError) = 0;

    /**
     * @brief 发送邮箱验证码
     * @param email 邮箱地址
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void sendEmailVerifyCode(const QString& email,
                                     SuccessCallback onSuccess,
                                     ErrorCallback onError) = 0;

    /**
     * @brief 忘记密码
     * @param email 邮箱
     * @param emailCode 邮箱验证码
     * @param newPassword 新密码
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void forgetPassword(const QString& email,
                                const QString& emailCode,
                                const QString& newPassword,
                                SuccessCallback onSuccess,
                                ErrorCallback onError) = 0;

    // ========================================================================
    // 用户信息 API
    // ========================================================================

    /**
     * @brief 获取用户信息
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void getUserInfo(SuccessCallback onSuccess,
                             ErrorCallback onError) = 0;

    /**
     * @brief 获取订阅信息
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void getSubscribeInfo(SuccessCallback onSuccess,
                                  ErrorCallback onError) = 0;

    /**
     * @brief 重置订阅安全信息
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void resetSecurity(SuccessCallback onSuccess,
                               ErrorCallback onError) = 0;

    // ========================================================================
    // 套餐计划 API
    // ========================================================================

    /**
     * @brief 获取套餐列表
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void fetchPlans(SuccessCallback onSuccess,
                            ErrorCallback onError) = 0;

    // ========================================================================
    // 订单 API
    // ========================================================================

    /**
     * @brief 创建订单
     * @param planId 套餐ID
     * @param period 订阅周期
     * @param couponCode 优惠券代码
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void createOrder(int planId,
                             const QString& period,
                             const QString& couponCode,
                             SuccessCallback onSuccess,
                             ErrorCallback onError) = 0;

    /**
     * @brief 获取订单列表
     * @param page 页码
     * @param pageSize 每页数量
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void fetchOrders(int page,
                             int pageSize,
                             SuccessCallback onSuccess,
                             ErrorCallback onError) = 0;

    /**
     * @brief 获取订单详情
     * @param orderId 订单ID
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void getOrderDetail(const QString& orderId,
                                SuccessCallback onSuccess,
                                ErrorCallback onError) = 0;

    /**
     * @brief 取消订单
     * @param orderId 订单ID
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void cancelOrder(const QString& orderId,
                             SuccessCallback onSuccess,
                             ErrorCallback onError) = 0;

    /**
     * @brief 检查订单状态
     * @param orderId 订单ID
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void checkOrderStatus(const QString& orderId,
                                  SuccessCallback onSuccess,
                                  ErrorCallback onError) = 0;

    // ========================================================================
    // 支付 API
    // ========================================================================

    /**
     * @brief 获取支付方式列表
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void fetchPaymentMethods(SuccessCallback onSuccess,
                                     ErrorCallback onError) = 0;

    /**
     * @brief 获取支付链接
     * @param tradeNo 订单交易号
     * @param paymentMethod 支付方式
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void getPaymentUrl(const QString& tradeNo,
                               const QString& paymentMethod,
                               SuccessCallback onSuccess,
                               ErrorCallback onError) = 0;

    // ========================================================================
    // 工单 API
    // ========================================================================

    /**
     * @brief 获取工单列表
     * @param page 页码
     * @param pageSize 每页数量
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void fetchTickets(int page,
                              int pageSize,
                              SuccessCallback onSuccess,
                              ErrorCallback onError) = 0;

    /**
     * @brief 创建工单
     * @param subject 主题
     * @param level 优先级
     * @param message 内容
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void createTicket(const QString& subject,
                              int level,
                              const QString& message,
                              SuccessCallback onSuccess,
                              ErrorCallback onError) = 0;

    /**
     * @brief 获取工单详情
     * @param ticketId 工单ID
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void getTicketDetail(const QString& ticketId,
                                 SuccessCallback onSuccess,
                                 ErrorCallback onError) = 0;

    /**
     * @brief 回复工单
     * @param ticketId 工单ID
     * @param message 回复内容
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void replyTicket(const QString& ticketId,
                             const QString& message,
                             SuccessCallback onSuccess,
                             ErrorCallback onError) = 0;

    /**
     * @brief 关闭工单
     * @param ticketId 工单ID
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void closeTicket(const QString& ticketId,
                             SuccessCallback onSuccess,
                             ErrorCallback onError) = 0;

    // ========================================================================
    // 系统配置 API
    // ========================================================================

    /**
     * @brief 获取系统配置
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void getSystemConfig(SuccessCallback onSuccess,
                                 ErrorCallback onError) = 0;

    /**
     * @brief 获取公告列表
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void fetchNotices(SuccessCallback onSuccess,
                              ErrorCallback onError) = 0;

    // ========================================================================
    // 邀请相关 API
    // ========================================================================

    /**
     * @brief 获取邀请信息
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void fetchInviteInfo(SuccessCallback onSuccess,
                                 ErrorCallback onError) = 0;

    /**
     * @brief 佣金提现
     * @param amount 提现金额
     * @param withdrawMethod 提现方式
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    virtual void withdrawCommission(double amount,
                                    int withdrawMethod,
                                    SuccessCallback onSuccess,
                                    ErrorCallback onError) = 0;

signals:
    /**
     * @brief 认证状态变化信号
     * @param authenticated 是否已认证
     */
    void authenticationChanged(bool authenticated);

    /**
     * @brief Token 更新信号
     * @param token 新的 Token
     */
    void tokenUpdated(const QString& token);

    /**
     * @brief 错误信号
     * @param error 错误信息
     */
    void errorOccurred(const QString& error);
};

#endif // IPANELPROVIDER_H
