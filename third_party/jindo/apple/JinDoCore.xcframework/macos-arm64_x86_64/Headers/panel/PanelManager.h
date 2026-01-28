/**
 * @file PanelManager.h
 * @brief 面板管理器头文件
 * @details 统一管理面板提供者，支持运行时切换面板、面板注册和配置
 * @author JinGo VPN Team
 * @date 2025
 */

#ifndef PANELMANAGER_H
#define PANELMANAGER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QJsonObject>
#include <memory>
#include "IPanelProvider.h"

/**
 * @class PanelManager
 * @brief 面板管理器（单例模式）
 *
 * @details
 * 核心功能：
 * - 面板注册：注册新的面板提供者
 * - 面板切换：运行时切换当前使用的面板
 * - 配置管理：加载和保存面板配置
 * - 统一接口：代理当前面板的所有 API 调用
 *
 * 使用流程：
 * 1. 获取单例：PanelManager::instance()
 * 2. 注册面板：registerProvider("xboard", new XBoardProvider())
 * 3. 设置当前面板：setCurrentProvider("xboard")
 * 4. 调用 API：currentProvider()->login(...)
 *
 * @example
 * @code
 * // 注册多个面板
 * PanelManager::instance().registerProvider("xboard", new XBoardProvider());
 * PanelManager::instance().registerProvider("v2board", new V2BoardProvider());
 *
 * // 切换面板
 * PanelManager::instance().setCurrentProvider("v2board");
 *
 * // 使用当前面板
 * auto provider = PanelManager::instance().currentProvider();
 * provider->login(email, password, onSuccess, onError);
 * @endcode
 */
class PanelManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString currentProviderName READ currentProviderName NOTIFY currentProviderChanged)
    Q_PROPERTY(QStringList availableProviders READ availableProviders NOTIFY providersChanged)

public:
    /**
     * @brief 获取 PanelManager 单例实例
     * @return PanelManager& 全局唯一实例的引用
     */
    static PanelManager& instance();

    // ========================================================================
    // 面板注册和管理
    // ========================================================================

    /**
     * @brief 注册面板提供者
     * @param name 面板名称（唯一标识符）
     * @param provider 面板提供者实例
     * @return bool 注册成功返回 true
     */
    bool registerProvider(const QString& name, IPanelProvider* provider);

    /**
     * @brief 注销面板提供者
     * @param name 面板名称
     * @return bool 注销成功返回 true
     */
    bool unregisterProvider(const QString& name);

    /**
     * @brief 获取已注册的面板提供者
     * @param name 面板名称
     * @return IPanelProvider* 面板提供者指针，不存在返回 nullptr
     */
    IPanelProvider* getProvider(const QString& name) const;

    /**
     * @brief 获取所有已注册的面板名称
     * @return QStringList 面板名称列表
     */
    QStringList availableProviders() const;

    // ========================================================================
    // 当前面板管理
    // ========================================================================

    /**
     * @brief 设置当前使用的面板
     * @param name 面板名称
     * @return bool 设置成功返回 true
     */
    Q_INVOKABLE bool setCurrentProvider(const QString& name);

    /**
     * @brief 获取当前面板提供者
     * @return IPanelProvider* 当前面板提供者指针
     */
    IPanelProvider* currentProvider() const;

    /**
     * @brief 获取当前面板名称
     * @return QString 当前面板名称
     */
    QString currentProviderName() const;

    // ========================================================================
    // 配置管理
    // ========================================================================

    /**
     * @brief 加载面板配置
     * @details 从数据库或配置文件加载面板设置
     */
    void loadConfig();

    /**
     * @brief 保存面板配置
     * @details 保存当前面板设置到数据库或配置文件
     */
    void saveConfig();

    /**
     * @brief 设置面板 API 地址
     * @param name 面板名称
     * @param baseUrl API 基础地址
     */
    Q_INVOKABLE void setPanelUrl(const QString& name, const QString& baseUrl);

    /**
     * @brief 获取面板 API 地址
     * @param name 面板名称
     * @return QString API 基础地址
     */
    Q_INVOKABLE QString getPanelUrl(const QString& name) const;

    /**
     * @brief 获取面板配置
     * @param name 面板名称
     * @return QJsonObject 面板配置
     */
    QJsonObject getPanelConfig(const QString& name) const;

    /**
     * @brief 设置面板配置
     * @param name 面板名称
     * @param config 面板配置
     */
    void setPanelConfig(const QString& name, const QJsonObject& config);

    // ========================================================================
    // 便捷方法（代理到当前面板）
    // ========================================================================

    /**
     * @brief 用户登录
     */
    Q_INVOKABLE void login(const QString& email, const QString& password);

    /**
     * @brief 用户登出
     */
    Q_INVOKABLE void logout();

    /**
     * @brief 获取用户信息
     */
    Q_INVOKABLE void getUserInfo();

    /**
     * @brief 获取订阅信息
     */
    Q_INVOKABLE void getSubscribeInfo();

signals:
    /**
     * @brief 当前面板变化信号
     * @param name 新的面板名称
     */
    void currentProviderChanged(const QString& name);

    /**
     * @brief 面板列表变化信号
     */
    void providersChanged();

    /**
     * @brief 登录成功信号
     * @param response 服务器响应
     */
    void loginSucceeded(const QJsonObject& response);

    /**
     * @brief 登录失败信号
     * @param error 错误信息
     */
    void loginFailed(const QString& error);

    /**
     * @brief 登出完成信号
     */
    void logoutCompleted();

    /**
     * @brief 用户信息获取成功信号
     * @param userInfo 用户信息
     */
    void userInfoLoaded(const QJsonObject& userInfo);

    /**
     * @brief 订阅信息获取成功信号
     * @param subscribeInfo 订阅信息
     */
    void subscribeInfoLoaded(const QJsonObject& subscribeInfo);

    /**
     * @brief 错误信号
     * @param error 错误信息
     */
    void errorOccurred(const QString& error);

private:
    PanelManager(QObject* parent = nullptr);
    ~PanelManager();

    PanelManager(const PanelManager&) = delete;
    PanelManager& operator=(const PanelManager&) = delete;

    /**
     * @brief 初始化默认面板提供者
     */
    void initDefaultProviders();

private:
    QMap<QString, IPanelProvider*> m_providers;  ///< 已注册的面板提供者
    QMap<QString, QJsonObject> m_panelConfigs;   ///< 面板配置
    QString m_currentProviderName;               ///< 当前面板名称
    IPanelProvider* m_currentProvider;           ///< 当前面板提供者
};

#endif // PANELMANAGER_H
