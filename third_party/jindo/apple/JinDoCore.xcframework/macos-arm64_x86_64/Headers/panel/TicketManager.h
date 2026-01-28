/**
 * @file TicketManager.h
 * @brief 工单管理器头文件
 * @details 提供工单创建、查询、回复、关闭等完整功能
 * @author JinGo VPN Team
 * @date 2025
 */

#ifndef TICKETMANAGER_H
#define TICKETMANAGER_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

// 前向声明
class ApiClient;

// ============================================================================
// TicketManager 类定义
// ============================================================================

/**
 * @class TicketManager
 * @brief 工单管理器（单例模式）
 *
 * @details
 * 核心功能：
 * - 工单创建：创建新工单
 * - 工单查询：查询工单列表和详情
 * - 工单回复：回复工单消息
 * - 工单关闭：关闭工单
 *
 * @example C++ 使用示例
 * @code
 * // 创建工单
 * TicketManager::instance().createTicket("连接问题", "无法连接到服务器", 1);
 *
 * // 获取工单列表
 * TicketManager::instance().fetchTickets();
 * @endcode
 */
class TicketManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)

public:
    static TicketManager& instance();

    /**
     * @brief 创建工单
     * @param subject 工单主题
     * @param message 工单内容
     * @param level 工单等级 (0=低, 1=中, 2=高)
     * @param attachmentPath 附件文件路径（可选）
     */
    Q_INVOKABLE void createTicket(const QString& subject, const QString& message, int level = 1, const QString& attachmentPath = QString());

    /**
     * @brief 获取工单列表
     * @param page 页码
     * @param pageSize 每页数量
     */
    Q_INVOKABLE void fetchTickets(int page = 1, int pageSize = 20);

    /**
     * @brief 获取工单详情
     * @param ticketId 工单ID
     */
    Q_INVOKABLE void getTicketDetail(int ticketId);

    /**
     * @brief 回复工单
     * @param ticketId 工单ID
     * @param message 回复内容
     * @param attachmentPath 附件文件路径（可选）
     */
    Q_INVOKABLE void replyTicket(int ticketId, const QString& message, const QString& attachmentPath = QString());

    /**
     * @brief 关闭工单
     * @param ticketId 工单ID
     */
    Q_INVOKABLE void closeTicket(int ticketId);

    bool isProcessing() const;

signals:
    void ticketCreated(const QJsonObject& ticket);
    void ticketFailed(const QString& error);
    void ticketsLoaded(const QJsonArray& tickets);
    void ticketsFailed(const QString& error);
    void ticketDetailLoaded(const QJsonObject& ticket);
    void ticketDetailFailed(const QString& error);
    void ticketReplied(int ticketId);
    void ticketReplyFailed(const QString& error);
    void ticketClosed(int ticketId);
    void ticketCloseFailed(const QString& error);
    void isProcessingChanged();
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);

private:
    TicketManager(QObject* parent = nullptr);
    ~TicketManager();
    TicketManager(const TicketManager&) = delete;
    TicketManager& operator=(const TicketManager&) = delete;

    void setProcessing(bool processing);

private slots:
    void onCreateTicketSuccess(const QJsonObject& response);
    void onCreateTicketError(const QString& error);
    void onFetchTicketsSuccess(const QJsonObject& response);
    void onFetchTicketsError(const QString& error);
    void onTicketDetailSuccess(const QJsonObject& response);
    void onTicketDetailError(const QString& error);
    void onReplyTicketSuccess(const QJsonObject& response);
    void onReplyTicketError(const QString& error);
    void onCloseTicketSuccess(const QJsonObject& response);
    void onCloseTicketError(const QString& error);

private:
    void uploadAndSubmitTicket(const QString& subject, const QString& message, int level, const QString& attachmentPath);
    void uploadAndSubmitReply(int ticketId, const QString& message, const QString& attachmentPath);
    void submitTicketRequest(const QString& subject, const QString& message, int level, const QString& attachmentUrl = QString());
    void submitReplyRequest(int ticketId, const QString& message, const QString& attachmentUrl = QString());

private:
    ApiClient& m_apiClient;
    bool m_isProcessing;
    int m_pendingTicketId;

    // 附件上传相关
    QString m_pendingSubject;
    QString m_pendingMessage;
    int m_pendingLevel;
};

#endif // TICKETMANAGER_H
