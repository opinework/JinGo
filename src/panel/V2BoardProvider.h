/**
 * @file V2BoardProvider.h
 * @brief V2Board 面板提供者头文件
 * @details 实现 V2Board 面板的 API 对接（JinGo 扩展）
 * @author JinGo VPN Team
 * @date 2025
 */

#ifndef V2BOARDPROVIDER_H
#define V2BOARDPROVIDER_H

#include <panel/IPanelProvider.h>
#include <QNetworkAccessManager>

/**
 * @class V2BoardProvider
 * @brief V2Board 面板提供者实现
 *
 * @details
 * V2Board API 与 XBoard 的主要差异：
 *
 * API 端点差异：
 * - 登录: POST /api/v1/passport/auth/login
 * - 注册: POST /api/v1/passport/auth/register
 * - 用户信息: GET /api/v1/user/info (返回格式不同)
 * - 订阅: GET /api/v1/user/getSubscribe
 * - 套餐: GET /api/v1/user/plan/fetch
 *
 * 数据格式差异：
 * - 流量单位：V2Board 使用字节，XBoard 可能使用 GB
 * - 时间格式：时间戳 vs 日期字符串
 * - Token 格式：auth_data vs token
 */
class V2BoardProvider : public IPanelProvider
{
    Q_OBJECT

public:
    explicit V2BoardProvider(QObject* parent = nullptr);
    ~V2BoardProvider() override;

    // ========================================================================
    // 面板信息实现
    // ========================================================================

    PanelType panelType() const override { return PanelType::V2Board; }
    QString panelName() const override { return "V2Board"; }
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
     * @brief 转换 V2Board 响应格式为统一格式
     */
    QJsonObject normalizeResponse(const QJsonObject& response);

    /**
     * @brief 转换用户信息格式
     */
    QJsonObject normalizeUserInfo(const QJsonObject& v2boardUser);

private:
    QString m_baseUrl;
    QString m_authToken;
    QNetworkAccessManager* m_networkManager;
};

#endif // V2BOARDPROVIDER_H
