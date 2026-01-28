/**
 * @file SystemConfigManager.h
 * @brief 系统配置管理器头文件
 * @details 提供版本检查、系统配置、知识库等功能
 * @author JinGo VPN Team
 * @date 2025
 */

#ifndef SYSTEMCONFIGMANAGER_H
#define SYSTEMCONFIGMANAGER_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QHash>
#include <QDir>
#include <QSet>
#include <QNetworkAccessManager>

// 前向声明
class ApiClient;

// ============================================================================
// SystemConfigManager 类定义
// ============================================================================

/**
 * @class SystemConfigManager
 * @brief 系统配置管理器（单例模式）
 *
 * @details
 * 核心功能：
 * - 版本检查：检查应用最新版本
 * - 系统配置：获取客户端全局配置
 * - 知识库：获取帮助文档和FAQ
 * - 服务器状态：检查服务器节点状态
 * - 流量上报：上报流量统计数据
 */
class SystemConfigManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantMap systemConfig READ systemConfig NOTIFY systemConfigChanged)

    // Guest 配置属性（xboard/v2board）
    Q_PROPERTY(bool isRecaptchaEnabled READ isRecaptchaEnabled NOTIFY guestConfigChanged)
    Q_PROPERTY(QString recaptchaSiteKey READ recaptchaSiteKey NOTIFY guestConfigChanged)
    Q_PROPERTY(bool isEmailVerifyEnabled READ isEmailVerifyEnabled NOTIFY guestConfigChanged)
    Q_PROPERTY(bool isInviteForce READ isInviteForce NOTIFY guestConfigChanged)
    Q_PROPERTY(bool registerEnabled READ registerEnabled NOTIFY guestConfigChanged)
    Q_PROPERTY(QString siteName READ siteName NOTIFY guestConfigChanged)
    Q_PROPERTY(QString siteDescription READ siteDescription NOTIFY guestConfigChanged)
    Q_PROPERTY(QString tosUrl READ tosUrl NOTIFY guestConfigChanged)
    Q_PROPERTY(bool guestConfigLoaded READ guestConfigLoaded NOTIFY guestConfigChanged)

public:
    static SystemConfigManager& instance();

    /**
     * @brief 检查应用版本
     * @param platform 平台标识 (macos, ios, windows, linux, android)
     */
    Q_INVOKABLE void checkVersion(const QString& platform);

    /**
     * @brief 获取系统配置
     */
    Q_INVOKABLE void fetchSystemConfig();

    /**
     * @brief 获取 Guest 配置（xboard/v2board 的 /guest/comm/config）
     * @details 获取验证码设置、邮箱验证、邀请码等配置
     */
    Q_INVOKABLE void fetchGuestConfig();

    // Guest 配置 Getters
    bool isRecaptchaEnabled() const { return m_isRecaptchaEnabled; }
    QString recaptchaSiteKey() const { return m_recaptchaSiteKey; }
    bool isEmailVerifyEnabled() const { return m_isEmailVerifyEnabled; }
    bool isInviteForce() const { return m_isInviteForce; }
    bool registerEnabled() const { return m_registerEnabled; }
    QString siteName() const { return m_siteName; }
    QString siteDescription() const { return m_siteDescription; }
    QString tosUrl() const { return m_tosUrl; }
    bool guestConfigLoaded() const { return m_guestConfigLoaded; }

    /**
     * @brief 获取知识库文章列表
     */
    Q_INVOKABLE void fetchKnowledge();

    /**
     * @brief 获取知识库文章详情
     * @param articleId 文章ID
     */
    Q_INVOKABLE void getKnowledgeArticle(int articleId);

    /**
     * @brief 提交知识库文章反馈
     * @param articleId 文章ID
     * @param isHelpful 是否有帮助 (true=有用, false=无用)
     */
    Q_INVOKABLE void submitKnowledgeFeedback(int articleId, bool isHelpful);

    /**
     * @brief 检查服务器状态
     */
    Q_INVOKABLE void checkServerStatus();

    /**
     * @brief 上报流量统计
     * @param serverId 服务器ID
     * @param upload 上传流量（字节）
     * @param download 下载流量（字节）
     * @param duration 会话时长（秒）
     */
    Q_INVOKABLE void reportTraffic(int serverId, qint64 upload, qint64 download, int duration);

    QVariantMap systemConfig() const;

    /**
     * @brief 获取API基础URL（用于修复相对路径图片）
     * @return API基础URL
     */
    Q_INVOKABLE QString getBaseUrl() const;

    /**
     * @brief 获取面板根域名（用于拼接图片URL）
     * @return 面板根域名，如 https://cp.jingo.cfd
     */
    Q_INVOKABLE QString getPanelDomain() const;

    /**
     * @brief 缓存远程图片到本地
     * @param imageUrl 远程图片URL
     */
    Q_INVOKABLE void cacheImage(const QString& imageUrl);

    /**
     * @brief 批量缓存远程图片（并行下载）
     * @param imageUrls 远程图片URL列表
     */
    Q_INVOKABLE void cacheImages(const QStringList& imageUrls);

    /**
     * @brief 获取已缓存图片的本地路径
     * @param imageUrl 远程图片URL
     * @return 本地文件路径，如果未缓存返回空字符串
     */
    Q_INVOKABLE QString getCachedImagePath(const QString& imageUrl) const;

signals:
    void versionChecked(const QJsonObject& versionInfo);
    void versionCheckFailed(const QString& error);
    void systemConfigLoaded(const QJsonObject& config);
    void systemConfigFailed(const QString& error);
    void knowledgeLoaded(const QJsonArray& articles);
    void knowledgeFailed(const QString& error);
    void knowledgeArticleLoaded(const QJsonObject& article);
    void knowledgeArticleFailed(const QString& error);
    void knowledgeFeedbackSubmitted(int articleId, bool isHelpful);
    void knowledgeFeedbackFailed(const QString& error);
    void serverStatusLoaded(const QJsonArray& status);
    void serverStatusFailed(const QString& error);
    void trafficReported();
    void trafficReportFailed(const QString& error);
    void systemConfigChanged();
    void guestConfigChanged();
    void guestConfigLoaded(const QJsonObject& config);
    void guestConfigFailed(const QString& error);
    void imageCached(const QString& imageUrl, const QString& localPath);
    void imageCacheFailed(const QString& imageUrl, const QString& error);

private:
    SystemConfigManager(QObject* parent = nullptr);
    ~SystemConfigManager();
    SystemConfigManager(const SystemConfigManager&) = delete;
    SystemConfigManager& operator=(const SystemConfigManager&) = delete;

private slots:
    void onVersionCheckSuccess(const QJsonObject& response);
    void onVersionCheckError(const QString& error);
    void onSystemConfigSuccess(const QJsonObject& response);
    void onSystemConfigError(const QString& error);
    void onKnowledgeSuccess(const QJsonObject& response);
    void onKnowledgeError(const QString& error);
    void onKnowledgeArticleSuccess(const QJsonObject& response);
    void onKnowledgeArticleError(const QString& error);
    void onKnowledgeFeedbackSuccess(const QJsonObject& response, int articleId, bool isHelpful);
    void onKnowledgeFeedbackError(const QString& error);
    void onServerStatusSuccess(const QJsonObject& response);
    void onServerStatusError(const QString& error);
    void onTrafficReportSuccess(const QJsonObject& response);
    void onTrafficReportError(const QString& error);
    void onGuestConfigSuccess(const QJsonObject& response);
    void onGuestConfigError(const QString& error);

private:
    ApiClient& m_apiClient;
    QVariantMap m_systemConfig;
    QHash<QString, QString> m_imageCache;  // URL -> data URI
    QString m_cacheDir;  // 图片缓存目录
    QSet<QString> m_pendingDownloads;  // 正在下载中的URL
    QNetworkAccessManager* m_imageDownloader;  // 专用于图片下载的网络管理器

    // Guest 配置（xboard/v2board）
    bool m_isRecaptchaEnabled = false;
    QString m_recaptchaSiteKey;
    bool m_isEmailVerifyEnabled = false;
    bool m_isInviteForce = false;
    bool m_registerEnabled = true;
    QString m_siteName;
    QString m_siteDescription;
    QString m_tosUrl;
    bool m_guestConfigLoaded = false;
};

#endif // SYSTEMCONFIGMANAGER_H
