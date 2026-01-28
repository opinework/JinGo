/**
 * @file ConnectionViewModel.h
 * @brief 连接视图模型头文件
 * @details 管理VPN连接状态、流量统计和连接时间，为QML界面提供实时数据绑定
 * @author JinGo VPN Team
 * @date 2025
 */

#ifndef CONNECTIONVIEWMODEL_H
#define CONNECTIONVIEWMODEL_H

#include <QObject>
#include <QString>
#include "core/VPNManager.h"

/**
 * @class ConnectionViewModel
 * @brief 连接视图模型
 * @details 提供VPN连接的管理和监控功能，包括：
 * - VPN连接状态监控和控制
 * - 实时流量统计（上传/下载字节数和速度）
 * - 连接时长计时
 * - 当前服务器信息显示
 *
 * 使用两个定时器分别更新统计数据和连接时间，确保UI实时更新
 */
class ConnectionViewModel : public QObject
{
    Q_OBJECT

    /// 是否已连接（只读）
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY isConnectedChanged)

    /// 连接状态文本（只读）
    Q_PROPERTY(QString connectionState READ connectionState NOTIFY connectionStateChanged)

    /// 当前服务器名称（只读）
    Q_PROPERTY(QString currentServerName READ currentServerName NOTIFY currentServerNameChanged)

    /// 上传字节数（只读）
    Q_PROPERTY(qint64 uploadBytes READ uploadBytes NOTIFY uploadBytesChanged)

    /// 下载字节数（只读）
    Q_PROPERTY(qint64 downloadBytes READ downloadBytes NOTIFY downloadBytesChanged)

    /// 上传速度（字节/秒）（只读）
    Q_PROPERTY(int uploadSpeed READ uploadSpeed NOTIFY uploadSpeedChanged)

    /// 下载速度（字节/秒）（只读）
    Q_PROPERTY(int downloadSpeed READ downloadSpeed NOTIFY downloadSpeedChanged)

    /// 已连接时间（格式：HH:mm:ss）（只读）
    Q_PROPERTY(QString connectedTime READ connectedTime NOTIFY connectedTimeChanged)

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     *
     * @details 初始化视图模型：
     * - 连接VPN管理器的状态变化信号
     * - 设置统计更新定时器（每秒更新）
     * - 设置连接时间更新定时器（每秒更新）
     */
    explicit ConnectionViewModel(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ConnectionViewModel() override = default;

    /**
     * @brief 获取是否已连接
     * @return true表示已连接，false表示未连接
     */
    bool isConnected() const;

    /**
     * @brief 获取连接状态文本
     * @return 本地化的连接状态字符串
     *
     * @details 可能的返回值：
     * - "未连接"
     * - "正在连接..."
     * - "已连接"
     * - "正在断开..."
     * - "正在重连..."
     * - "连接错误"
     * - "未知状态"
     */
    QString connectionState() const;

    /**
     * @brief 获取当前服务器名称
     * @return 服务器名称，如果没有选中服务器则返回"无"
     */
    QString currentServerName() const;

    /**
     * @brief 获取上传字节数
     * @return 累计上传字节数
     */
    qint64 uploadBytes() const { return m_uploadBytes; }

    /**
     * @brief 获取下载字节数
     * @return 累计下载字节数
     */
    qint64 downloadBytes() const { return m_downloadBytes; }

    /**
     * @brief 获取上传速度
     * @return 当前上传速度（字节/秒）
     */
    int uploadSpeed() const { return m_uploadSpeed; }

    /**
     * @brief 获取下载速度
     * @return 当前下载速度（字节/秒）
     */
    int downloadSpeed() const { return m_downloadSpeed; }

    /**
     * @brief 获取已连接时间
     * @return 格式化的时间字符串（HH:mm:ss）
     *
     * @details 如果未连接则返回"00:00:00"
     */
    QString connectedTime() const;
    
public slots:
    /**
     * @brief 连接VPN
     *
     * @details 连接到当前选中的服务器。如果没有选中服务器则记录警告
     */
    void connect();

    /**
     * @brief 断开VPN连接
     */
    void disconnect();

    /**
     * @brief 重新连接VPN
     *
     * @details 断开当前连接并重新连接到同一服务器
     */
    void reconnect();

signals:
    /**
     * @brief 连接状态变化信号
     */
    void isConnectedChanged();

    /**
     * @brief 连接状态文本变化信号
     */
    void connectionStateChanged();

    /**
     * @brief 当前服务器名称变化信号
     */
    void currentServerNameChanged();

    /**
     * @brief 上传字节数变化信号
     */
    void uploadBytesChanged();

    /**
     * @brief 下载字节数变化信号
     */
    void downloadBytesChanged();

    /**
     * @brief 上传速度变化信号
     */
    void uploadSpeedChanged();

    /**
     * @brief 下载速度变化信号
     */
    void downloadSpeedChanged();

    /**
     * @brief 连接时间变化信号
     */
    void connectedTimeChanged();

private slots:
    /**
     * @brief VPN连接状态变化处理
     * @param state 新的连接状态
     *
     * @details 当VPN管理器的状态改变时触发，发出connectionStateChanged信号
     */
    void onConnectionStateChanged(VPNManager::ConnectionState state);

    /**
     * @brief 更新流量统计数据
     *
     * @details 从VPN管理器获取最新的流量数据和速度，更新后发出相应信号
     * - 每秒由m_statsTimer触发
     */
    void updateStatistics();

    /**
     * @brief 更新连接时间
     *
     * @details 每秒由m_timeTimer触发，发出connectedTimeChanged信号
     */
    void updateConnectionTime();

private:
    VPNManager* m_vpnManager;      ///< VPN管理器指针
    QTimer* m_statsTimer;          ///< 统计更新定时器（1秒间隔）
    QTimer* m_timeTimer;           ///< 连接时间更新定时器（1秒间隔）

    qint64 m_uploadBytes;          ///< 累计上传字节数
    qint64 m_downloadBytes;        ///< 累计下载字节数
    int m_uploadSpeed;             ///< 当前上传速度（字节/秒）
    int m_downloadSpeed;           ///< 当前下载速度（字节/秒）
    QDateTime m_connectionStartTime; ///< 连接开始时间
};

#endif // CONNECTIONVIEWMODEL_H
