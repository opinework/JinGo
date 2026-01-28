/**
 * @file V2BoardProvider.cpp
 * @brief V2Board 面板提供者实现（JinGo 扩展）
 * @author JinGo VPN Team
 * @date 2025
 */

#include "V2BoardProvider.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUrlQuery>
#include <QDebug>

// V2Board API 端点定义
namespace V2BoardEndpoints {
    // 注意：V2Board 使用 /api/v1 前缀
    const QString ApiPrefix = "/api/v1";

    // 认证相关
    const QString Login = "/passport/auth/login";
    const QString Register = "/passport/auth/register";
    const QString Logout = "/passport/auth/logout";
    const QString SendEmailVerify = "/passport/comm/sendEmailVerify";
    const QString ForgetPassword = "/passport/auth/forget";

    // 用户相关
    const QString UserInfo = "/user/info";
    const QString Subscribe = "/user/getSubscribe";
    const QString ResetSecurity = "/user/resetSecurity";

    // 套餐相关
    const QString Plans = "/user/plan/fetch";

    // 订单相关
    const QString OrderSave = "/user/order/save";
    const QString OrderFetch = "/user/order/fetch";
    const QString OrderDetail = "/user/order/detail";
    const QString OrderCancel = "/user/order/cancel";
    const QString OrderCheck = "/user/order/check";
    const QString Checkout = "/user/order/checkout";
    const QString PaymentMethods = "/user/order/getPaymentMethod";

    // 工单相关
    const QString TicketFetch = "/user/ticket/fetch";
    const QString TicketSave = "/user/ticket/save";
    const QString TicketReply = "/user/ticket/reply";
    const QString TicketClose = "/user/ticket/close";

    // 系统相关
    const QString Config = "/guest/comm/config";
    const QString Notices = "/user/notice/fetch";

    // 邀请相关
    const QString InviteFetch = "/user/invite/fetch";
    const QString InviteWithdraw = "/user/invite/withdraw";
}

V2BoardProvider::V2BoardProvider(QObject* parent)
    : IPanelProvider(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

V2BoardProvider::~V2BoardProvider()
{
}

// ============================================================================
// 配置实现
// ============================================================================

void V2BoardProvider::setBaseUrl(const QString& url)
{
    m_baseUrl = url;
    // V2Board 通常不需要 /api/v1 后缀，它在每个端点中添加
    if (m_baseUrl.endsWith("/")) {
        m_baseUrl.chop(1);
    }
}

QString V2BoardProvider::baseUrl() const
{
    return m_baseUrl;
}

void V2BoardProvider::setAuthToken(const QString& token)
{
    m_authToken = token;
}

QString V2BoardProvider::authToken() const
{
    return m_authToken;
}

// ============================================================================
// 辅助方法
// ============================================================================

namespace {
    QNetworkRequest createRequest(const QString& url, const QString& token)
    {
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Accept", "application/json");

        if (!token.isEmpty()) {
            request.setRawHeader("Authorization", token.toUtf8());
        }

        return request;
    }

    void handleResponse(QNetworkReply* reply,
                        V2BoardProvider::SuccessCallback onSuccess,
                        V2BoardProvider::ErrorCallback onError)
    {
        QObject::connect(reply, &QNetworkReply::finished, [reply, onSuccess, onError]() {
            reply->deleteLater();

            if (reply->error() != QNetworkReply::NoError) {
                if (onError) {
                    onError(reply->errorString());
                }
                return;
            }

            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);

            if (!doc.isObject()) {
                if (onError) {
                    onError("Invalid JSON response");
                }
                return;
            }

            QJsonObject response = doc.object();

            // V2Board 响应格式检查
            // 成功: {"data": {...}} 或 {"data": [...]}
            // 失败: {"message": "error message"}
            if (response.contains("message") && !response.contains("data")) {
                if (onError) {
                    onError(response["message"].toString());
                }
                return;
            }

            if (onSuccess) {
                onSuccess(response);
            }
        });
    }
}

// ============================================================================
// 用户认证 API 实现
// ============================================================================

void V2BoardProvider::login(const QString& email,
                            const QString& password,
                            SuccessCallback onSuccess,
                            ErrorCallback onError)
{
    QString url = m_baseUrl + V2BoardEndpoints::ApiPrefix + V2BoardEndpoints::Login;

    QJsonObject data;
    data["email"] = email;
    data["password"] = password;

    QNetworkRequest request = createRequest(url, "");
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(data).toJson());

    handleResponse(reply,
        [this, onSuccess](const QJsonObject& response) {
            // V2Board 登录响应: {"data": {"token": "xxx", "auth_data": "xxx"}}
            QJsonObject responseData = response["data"].toObject();
            QString token = responseData["auth_data"].toString();
            if (token.isEmpty()) {
                token = responseData["token"].toString();
            }

            if (!token.isEmpty()) {
                setAuthToken(token);
                emit authenticationChanged(true);
                emit tokenUpdated(token);
            }

            if (onSuccess) {
                onSuccess(response);
            }
        },
        onError);
}

void V2BoardProvider::register_(const QString& email,
                                const QString& password,
                                const QString& inviteCode,
                                const QString& emailCode,
                                SuccessCallback onSuccess,
                                ErrorCallback onError)
{
    QString url = m_baseUrl + V2BoardEndpoints::ApiPrefix + V2BoardEndpoints::Register;

    QJsonObject data;
    data["email"] = email;
    data["password"] = password;
    if (!inviteCode.isEmpty()) {
        data["invite_code"] = inviteCode;
    }
    if (!emailCode.isEmpty()) {
        data["email_code"] = emailCode;
    }

    QNetworkRequest request = createRequest(url, "");
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(data).toJson());
    handleResponse(reply, onSuccess, onError);
}

void V2BoardProvider::logout(SuccessCallback onSuccess,
                             ErrorCallback onError)
{
    setAuthToken("");
    emit authenticationChanged(false);

    // V2Board 可能需要服务端登出
    QString url = m_baseUrl + V2BoardEndpoints::ApiPrefix + V2BoardEndpoints::Logout;
    QNetworkRequest request = createRequest(url, m_authToken);
    QNetworkReply* reply = m_networkManager->get(request);

    handleResponse(reply,
        [onSuccess](const QJsonObject& response) {
            if (onSuccess) onSuccess(response);
        },
        [onSuccess](const QString&) {
            // 即使失败也视为登出成功
            if (onSuccess) onSuccess(QJsonObject());
        });
}

void V2BoardProvider::sendEmailVerifyCode(const QString& email,
                                          SuccessCallback onSuccess,
                                          ErrorCallback onError)
{
    QString url = m_baseUrl + V2BoardEndpoints::ApiPrefix + V2BoardEndpoints::SendEmailVerify;

    QJsonObject data;
    data["email"] = email;

    QNetworkRequest request = createRequest(url, "");
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(data).toJson());
    handleResponse(reply, onSuccess, onError);
}

void V2BoardProvider::forgetPassword(const QString& email,
                                     const QString& emailCode,
                                     const QString& newPassword,
                                     SuccessCallback onSuccess,
                                     ErrorCallback onError)
{
    QString url = m_baseUrl + V2BoardEndpoints::ApiPrefix + V2BoardEndpoints::ForgetPassword;

    QJsonObject data;
    data["email"] = email;
    data["email_code"] = emailCode;
    data["password"] = newPassword;

    QNetworkRequest request = createRequest(url, "");
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(data).toJson());
    handleResponse(reply, onSuccess, onError);
}

// ============================================================================
// 用户信息 API 实现
// ============================================================================

void V2BoardProvider::getUserInfo(SuccessCallback onSuccess,
                                  ErrorCallback onError)
{
    QString url = m_baseUrl + V2BoardEndpoints::ApiPrefix + V2BoardEndpoints::UserInfo;
    QNetworkRequest request = createRequest(url, m_authToken);
    QNetworkReply* reply = m_networkManager->get(request);

    handleResponse(reply,
        [this, onSuccess](const QJsonObject& response) {
            // 转换为统一格式
            QJsonObject normalized = normalizeUserInfo(response["data"].toObject());
            QJsonObject result;
            result["data"] = normalized;
            if (onSuccess) onSuccess(result);
        },
        onError);
}

void V2BoardProvider::getSubscribeInfo(SuccessCallback onSuccess,
                                       ErrorCallback onError)
{
    QString url = m_baseUrl + V2BoardEndpoints::ApiPrefix + V2BoardEndpoints::Subscribe;
    QNetworkRequest request = createRequest(url, m_authToken);
    QNetworkReply* reply = m_networkManager->get(request);
    handleResponse(reply, onSuccess, onError);
}

void V2BoardProvider::resetSecurity(SuccessCallback onSuccess,
                                    ErrorCallback onError)
{
    QString url = m_baseUrl + V2BoardEndpoints::ApiPrefix + V2BoardEndpoints::ResetSecurity;
    QNetworkRequest request = createRequest(url, m_authToken);
    QNetworkReply* reply = m_networkManager->get(request);
    handleResponse(reply, onSuccess, onError);
}

// ============================================================================
// 套餐计划 API 实现
// ============================================================================

void V2BoardProvider::fetchPlans(SuccessCallback onSuccess,
                                 ErrorCallback onError)
{
    QString url = m_baseUrl + V2BoardEndpoints::ApiPrefix + V2BoardEndpoints::Plans;
    QNetworkRequest request = createRequest(url, m_authToken);
    QNetworkReply* reply = m_networkManager->get(request);
    handleResponse(reply, onSuccess, onError);
}

// ============================================================================
// 订单 API 实现
// ============================================================================

void V2BoardProvider::createOrder(int planId,
                                  const QString& period,
                                  const QString& couponCode,
                                  SuccessCallback onSuccess,
                                  ErrorCallback onError)
{
    QString url = m_baseUrl + V2BoardEndpoints::ApiPrefix + V2BoardEndpoints::OrderSave;

    QJsonObject data;
    data["plan_id"] = planId;
    if (!period.isEmpty()) {
        data["period"] = period;
    }
    if (!couponCode.isEmpty()) {
        data["coupon_code"] = couponCode;
    }

    QNetworkRequest request = createRequest(url, m_authToken);
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(data).toJson());
    handleResponse(reply, onSuccess, onError);
}

void V2BoardProvider::fetchOrders(int page,
                                  int pageSize,
                                  SuccessCallback onSuccess,
                                  ErrorCallback onError)
{
    QString url = QString("%1%2%3?page=%4&pageSize=%5")
        .arg(m_baseUrl)
        .arg(V2BoardEndpoints::ApiPrefix)
        .arg(V2BoardEndpoints::OrderFetch)
        .arg(page)
        .arg(pageSize);

    QNetworkRequest request = createRequest(url, m_authToken);
    QNetworkReply* reply = m_networkManager->get(request);
    handleResponse(reply, onSuccess, onError);
}

void V2BoardProvider::getOrderDetail(const QString& orderId,
                                     SuccessCallback onSuccess,
                                     ErrorCallback onError)
{
    QString url = QString("%1%2%3?trade_no=%4")
        .arg(m_baseUrl)
        .arg(V2BoardEndpoints::ApiPrefix)
        .arg(V2BoardEndpoints::OrderDetail)
        .arg(orderId);

    QNetworkRequest request = createRequest(url, m_authToken);
    QNetworkReply* reply = m_networkManager->get(request);
    handleResponse(reply, onSuccess, onError);
}

void V2BoardProvider::cancelOrder(const QString& orderId,
                                  SuccessCallback onSuccess,
                                  ErrorCallback onError)
{
    QString url = m_baseUrl + V2BoardEndpoints::ApiPrefix + V2BoardEndpoints::OrderCancel;

    QJsonObject data;
    data["trade_no"] = orderId;

    QNetworkRequest request = createRequest(url, m_authToken);
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(data).toJson());
    handleResponse(reply, onSuccess, onError);
}

void V2BoardProvider::checkOrderStatus(const QString& orderId,
                                       SuccessCallback onSuccess,
                                       ErrorCallback onError)
{
    QString url = QString("%1%2%3?trade_no=%4")
        .arg(m_baseUrl)
        .arg(V2BoardEndpoints::ApiPrefix)
        .arg(V2BoardEndpoints::OrderCheck)
        .arg(orderId);

    QNetworkRequest request = createRequest(url, m_authToken);
    QNetworkReply* reply = m_networkManager->get(request);
    handleResponse(reply, onSuccess, onError);
}

// ============================================================================
// 支付 API 实现
// ============================================================================

void V2BoardProvider::fetchPaymentMethods(SuccessCallback onSuccess,
                                          ErrorCallback onError)
{
    QString url = m_baseUrl + V2BoardEndpoints::ApiPrefix + V2BoardEndpoints::PaymentMethods;
    QNetworkRequest request = createRequest(url, m_authToken);
    QNetworkReply* reply = m_networkManager->get(request);
    handleResponse(reply, onSuccess, onError);
}

void V2BoardProvider::getPaymentUrl(const QString& tradeNo,
                                    const QString& paymentMethod,
                                    SuccessCallback onSuccess,
                                    ErrorCallback onError)
{
    QString url = m_baseUrl + V2BoardEndpoints::ApiPrefix + V2BoardEndpoints::Checkout;

    QJsonObject data;
    data["trade_no"] = tradeNo;
    data["method"] = paymentMethod;

    QNetworkRequest request = createRequest(url, m_authToken);
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(data).toJson());
    handleResponse(reply, onSuccess, onError);
}

// ============================================================================
// 工单 API 实现
// ============================================================================

void V2BoardProvider::fetchTickets(int page,
                                   int pageSize,
                                   SuccessCallback onSuccess,
                                   ErrorCallback onError)
{
    QString url = QString("%1%2%3?page=%4&pageSize=%5")
        .arg(m_baseUrl)
        .arg(V2BoardEndpoints::ApiPrefix)
        .arg(V2BoardEndpoints::TicketFetch)
        .arg(page)
        .arg(pageSize);

    QNetworkRequest request = createRequest(url, m_authToken);
    QNetworkReply* reply = m_networkManager->get(request);
    handleResponse(reply, onSuccess, onError);
}

void V2BoardProvider::createTicket(const QString& subject,
                                   int level,
                                   const QString& message,
                                   SuccessCallback onSuccess,
                                   ErrorCallback onError)
{
    QString url = m_baseUrl + V2BoardEndpoints::ApiPrefix + V2BoardEndpoints::TicketSave;

    QJsonObject data;
    data["subject"] = subject;
    data["level"] = level;
    data["message"] = message;

    QNetworkRequest request = createRequest(url, m_authToken);
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(data).toJson());
    handleResponse(reply, onSuccess, onError);
}

void V2BoardProvider::getTicketDetail(const QString& ticketId,
                                      SuccessCallback onSuccess,
                                      ErrorCallback onError)
{
    QString url = QString("%1%2%3?id=%4")
        .arg(m_baseUrl)
        .arg(V2BoardEndpoints::ApiPrefix)
        .arg(V2BoardEndpoints::TicketFetch)
        .arg(ticketId);

    QNetworkRequest request = createRequest(url, m_authToken);
    QNetworkReply* reply = m_networkManager->get(request);
    handleResponse(reply, onSuccess, onError);
}

void V2BoardProvider::replyTicket(const QString& ticketId,
                                  const QString& message,
                                  SuccessCallback onSuccess,
                                  ErrorCallback onError)
{
    QString url = m_baseUrl + V2BoardEndpoints::ApiPrefix + V2BoardEndpoints::TicketReply;

    QJsonObject data;
    data["id"] = ticketId.toInt();
    data["message"] = message;

    QNetworkRequest request = createRequest(url, m_authToken);
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(data).toJson());
    handleResponse(reply, onSuccess, onError);
}

void V2BoardProvider::closeTicket(const QString& ticketId,
                                  SuccessCallback onSuccess,
                                  ErrorCallback onError)
{
    QString url = m_baseUrl + V2BoardEndpoints::ApiPrefix + V2BoardEndpoints::TicketClose;

    QJsonObject data;
    data["id"] = ticketId.toInt();

    QNetworkRequest request = createRequest(url, m_authToken);
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(data).toJson());
    handleResponse(reply, onSuccess, onError);
}

// ============================================================================
// 系统配置 API 实现
// ============================================================================

void V2BoardProvider::getSystemConfig(SuccessCallback onSuccess,
                                      ErrorCallback onError)
{
    QString url = m_baseUrl + V2BoardEndpoints::ApiPrefix + V2BoardEndpoints::Config;
    QNetworkRequest request = createRequest(url, "");
    QNetworkReply* reply = m_networkManager->get(request);
    handleResponse(reply, onSuccess, onError);
}

void V2BoardProvider::fetchNotices(SuccessCallback onSuccess,
                                   ErrorCallback onError)
{
    QString url = m_baseUrl + V2BoardEndpoints::ApiPrefix + V2BoardEndpoints::Notices;
    QNetworkRequest request = createRequest(url, m_authToken);
    QNetworkReply* reply = m_networkManager->get(request);
    handleResponse(reply, onSuccess, onError);
}

// ============================================================================
// 邀请相关 API 实现
// ============================================================================

void V2BoardProvider::fetchInviteInfo(SuccessCallback onSuccess,
                                      ErrorCallback onError)
{
    QString url = m_baseUrl + V2BoardEndpoints::ApiPrefix + V2BoardEndpoints::InviteFetch;
    QNetworkRequest request = createRequest(url, m_authToken);
    QNetworkReply* reply = m_networkManager->get(request);
    handleResponse(reply, onSuccess, onError);
}

void V2BoardProvider::withdrawCommission(double amount,
                                         int withdrawMethod,
                                         SuccessCallback onSuccess,
                                         ErrorCallback onError)
{
    QString url = m_baseUrl + V2BoardEndpoints::ApiPrefix + V2BoardEndpoints::InviteWithdraw;

    QJsonObject data;
    data["withdraw_amount"] = amount;
    data["withdraw_method"] = withdrawMethod;

    QNetworkRequest request = createRequest(url, m_authToken);
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(data).toJson());
    handleResponse(reply, onSuccess, onError);
}

// ============================================================================
// 私有方法 - 数据格式转换
// ============================================================================

QJsonObject V2BoardProvider::normalizeResponse(const QJsonObject& response)
{
    // V2Board 响应格式: {"data": {...}}
    if (response.contains("data")) {
        return response["data"].toObject();
    }
    return response;
}

QJsonObject V2BoardProvider::normalizeUserInfo(const QJsonObject& v2boardUser)
{
    // V2Board 用户信息字段映射到统一格式
    // V2Board: {email, transfer_enable, u, d, expired_at, ...}
    // 统一格式: {email, total_traffic, used_upload, used_download, expire_time, ...}

    QJsonObject normalized;
    normalized["email"] = v2boardUser["email"];
    normalized["uuid"] = v2boardUser["uuid"];

    // 流量转换（V2Board 使用字节）
    qint64 transferEnable = v2boardUser["transfer_enable"].toVariant().toLongLong();
    qint64 uploaded = v2boardUser["u"].toVariant().toLongLong();
    qint64 downloaded = v2boardUser["d"].toVariant().toLongLong();

    normalized["total_traffic"] = transferEnable;
    normalized["used_upload"] = uploaded;
    normalized["used_download"] = downloaded;
    normalized["used_traffic"] = uploaded + downloaded;
    normalized["remaining_traffic"] = transferEnable - uploaded - downloaded;

    // 过期时间（V2Board 使用时间戳）
    normalized["expire_time"] = v2boardUser["expired_at"];

    // 其他字段
    normalized["balance"] = v2boardUser["balance"];
    normalized["commission_balance"] = v2boardUser["commission_balance"];
    normalized["is_admin"] = v2boardUser["is_admin"];
    normalized["is_staff"] = v2boardUser["is_staff"];

    return normalized;
}
