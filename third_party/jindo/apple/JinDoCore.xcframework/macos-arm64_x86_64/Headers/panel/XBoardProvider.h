/**
 * @file XBoardProvider.h
 * @brief XBoard 面板提供者头文件
 * @details 实现 XBoard 面板的 API 对接
 * @author JinGo VPN Team
 * @date 2025
 */

#ifndef XBOARDPROVIDER_H
#define XBOARDPROVIDER_H

#include "IPanelProvider.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>

class ApiClient;

/**
 * @class XBoardProvider
 * @brief XBoard 面板提供者实现
 *
 * @details
 * 实现 XBoard (V2Board fork) 面板的所有 API 接口：
 *
 * API 端点映射：
 * - 登录: POST /passport/auth/login
 * - 注册: POST /passport/auth/register
 * - 登出: GET /passport/auth/logout (非必须)
 * - 用户信息: GET /user/info
 * - 订阅信息: GET /user/getSubscribe
 * - 套餐列表: GET /user/plan/fetch
 * - 订单创建: POST /user/order/save
 * - 订单列表: GET /user/order/fetch
 * - 支付方式: GET /user/order/getPaymentMethod
 * - 工单列表: GET /user/ticket/fetch
 * - 系统配置: GET /guest/comm/config
 */
class XBoardProvider : public IPanelProvider
{
    Q_OBJECT

public:
    explicit XBoardProvider(QObject* parent = nullptr);
    ~XBoardProvider() override;

    // ========================================================================
    // 面板信息实现
    // ========================================================================

    PanelType panelType() const override { return PanelType::XBoard; }
    QString panelName() const override { return "XBoard"; }
    QString panelVersion() const override { return "1.0"; }

    // ========================================================================
    // 配置实现
    // ========================================================================

    void setBaseUrl(const QString& url) override;
    QString baseUrl() const override;
    void setAuthToken(const QString& token) override;
    QString authToken() const override;

    // ========================================================================
    // 用户认证 API 实现
    // ========================================================================

    void login(const QString& email,
               const QString& password,
               SuccessCallback onSuccess,
               ErrorCallback onError) override;

    void register_(const QString& email,
                   const QString& password,
                   const QString& inviteCode,
                   const QString& emailCode,
                   SuccessCallback onSuccess,
                   ErrorCallback onError) override;

    void logout(SuccessCallback onSuccess,
                ErrorCallback onError) override;

    void sendEmailVerifyCode(const QString& email,
                             SuccessCallback onSuccess,
                             ErrorCallback onError) override;

    void forgetPassword(const QString& email,
                        const QString& emailCode,
                        const QString& newPassword,
                        SuccessCallback onSuccess,
                        ErrorCallback onError) override;

    // ========================================================================
    // 用户信息 API 实现
    // ========================================================================

    void getUserInfo(SuccessCallback onSuccess,
                     ErrorCallback onError) override;

    void getSubscribeInfo(SuccessCallback onSuccess,
                          ErrorCallback onError) override;

    void resetSecurity(SuccessCallback onSuccess,
                       ErrorCallback onError) override;

    // ========================================================================
    // 套餐计划 API 实现
    // ========================================================================

    void fetchPlans(SuccessCallback onSuccess,
                    ErrorCallback onError) override;

    // ========================================================================
    // 订单 API 实现
    // ========================================================================

    void createOrder(int planId,
                     const QString& period,
                     const QString& couponCode,
                     SuccessCallback onSuccess,
                     ErrorCallback onError) override;

    void fetchOrders(int page,
                     int pageSize,
                     SuccessCallback onSuccess,
                     ErrorCallback onError) override;

    void getOrderDetail(const QString& orderId,
                        SuccessCallback onSuccess,
                        ErrorCallback onError) override;

    void cancelOrder(const QString& orderId,
                     SuccessCallback onSuccess,
                     ErrorCallback onError) override;

    void checkOrderStatus(const QString& orderId,
                          SuccessCallback onSuccess,
                          ErrorCallback onError) override;

    // ========================================================================
    // 支付 API 实现
    // ========================================================================

    void fetchPaymentMethods(SuccessCallback onSuccess,
                             ErrorCallback onError) override;

    void getPaymentUrl(const QString& tradeNo,
                       const QString& paymentMethod,
                       SuccessCallback onSuccess,
                       ErrorCallback onError) override;

    // ========================================================================
    // 工单 API 实现
    // ========================================================================

    void fetchTickets(int page,
                      int pageSize,
                      SuccessCallback onSuccess,
                      ErrorCallback onError) override;

    void createTicket(const QString& subject,
                      int level,
                      const QString& message,
                      SuccessCallback onSuccess,
                      ErrorCallback onError) override;

    void getTicketDetail(const QString& ticketId,
                         SuccessCallback onSuccess,
                         ErrorCallback onError) override;

    void replyTicket(const QString& ticketId,
                     const QString& message,
                     SuccessCallback onSuccess,
                     ErrorCallback onError) override;

    void closeTicket(const QString& ticketId,
                     SuccessCallback onSuccess,
                     ErrorCallback onError) override;

    // ========================================================================
    // 系统配置 API 实现
    // ========================================================================

    void getSystemConfig(SuccessCallback onSuccess,
                         ErrorCallback onError) override;

    void fetchNotices(SuccessCallback onSuccess,
                      ErrorCallback onError) override;

    // ========================================================================
    // 邀请相关 API 实现
    // ========================================================================

    void fetchInviteInfo(SuccessCallback onSuccess,
                         ErrorCallback onError) override;

    void withdrawCommission(double amount,
                            int withdrawMethod,
                            SuccessCallback onSuccess,
                            ErrorCallback onError) override;

private:
    /**
     * @brief 发起 HTTP 请求
     * @param method HTTP 方法
     * @param endpoint API 端点
     * @param data 请求数据
     * @param onSuccess 成功回调
     * @param onError 失败回调
     */
    void request(const QString& method,
                 const QString& endpoint,
                 const QJsonObject& data,
                 SuccessCallback onSuccess,
                 ErrorCallback onError);

    /**
     * @brief 处理响应数据
     * @param response 原始响应
     * @return QJsonObject 处理后的数据
     */
    QJsonObject processResponse(const QJsonObject& response);

private:
    QString m_baseUrl;      ///< API 基础地址
    QString m_authToken;    ///< 认证 Token
    ApiClient& m_apiClient; ///< API 客户端引用
};

#endif // XBOARDPROVIDER_H
