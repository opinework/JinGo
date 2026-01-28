/**
 * @file VPNManager.h
 * @brief VPN连接管理器头文件
 * @details 提供VPN连接的完整生命周期管理，包括连接建立、状态监控、
 *          流量统计、自动重连等功能。支持TUN模式和系统代理模式两种工作方式。
 *
 * @author VPN Client Team
 * @version 2.0
 * @date 2024
 *
 * @copyright Copyright (c) 2024
 */

#ifndef VPNMANAGER_H
#define VPNMANAGER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QTimer>
#include <QPointer>
#include <QFuture>
#include <QProcess>
#include "../models/Server.h"

// ============================================================================
// 前向声明
// ============================================================================

class VPNCore;        ///< VPN核心（包装XrayCBridge）
class ConfigManager;  ///< 配置管理器
class IServerProvider; ///< 服务器提供者接口（用于解耦 SubscriptionManager）
// TUN设备现在由SuperRay在Network Extension内部处理
// 桌面平台(Windows/Linux)将使用SuperRay的TUN接口
class PlatformInterface;  ///< 平台相关接口

// macOS/iOS Network Extension
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
#ifdef __OBJC__
@class NetworkExtensionManager;
#else
class NetworkExtensionManager;
#endif
#endif

// Windows TUN components
#ifdef Q_OS_WIN
namespace JinGo {
    class WinTunManager;
}
#endif

// ============================================================================
// VPNManager 类定义
// ============================================================================

/**
 * @class VPNManager
 * @brief VPN连接管理器类（单例模式）
 *
 * @details
 * VPNManager是整个VPN客户端的核心管理类，负责：
 * - VPN连接的建立、维护和断开
 * - 连接状态的监控和上报
 * - 流量统计和速度监控
 * - 自动重连机制
 * - TUN设备管理
 * - 系统代理配置
 * - 路由表管理
 *
 * 该类采用单例模式，通过instance()方法获取全局唯一实例。
 *
 * @note 线程安全：该类的大多数方法应在主线程中调用
 *
 * 使用示例：
 * @code
 * VPNManager& manager = VPNManager::instance();
 * connect(&manager, &VPNManager::connected, this, &MyClass::onConnected);
 * manager.connecting(serverObject);
 * @endcode
 */
class VPNManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Server* currentServer READ currentServer NOTIFY currentServerChanged)
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY stateChanged)
    Q_PROPERTY(bool isConnecting READ isConnecting NOTIFY stateChanged)
    Q_PROPERTY(bool isDisconnecting READ isDisconnecting NOTIFY stateChanged)

    // 流量统计属性
    Q_PROPERTY(quint64 uploadBytes READ uploadBytes NOTIFY statsUpdated)
    Q_PROPERTY(quint64 downloadBytes READ downloadBytes NOTIFY statsUpdated)
    Q_PROPERTY(quint64 uploadSpeed READ uploadSpeed NOTIFY statsUpdated)
    Q_PROPERTY(quint64 downloadSpeed READ downloadSpeed NOTIFY statsUpdated)
    Q_PROPERTY(quint64 totalUpload READ uploadBytes NOTIFY statsUpdated)
    Q_PROPERTY(quint64 totalDownload READ downloadBytes NOTIFY statsUpdated)
    Q_PROPERTY(qint64 connectedDuration READ connectedDuration NOTIFY statsUpdated)

    // 连接信息属性
    Q_PROPERTY(QString currentIP READ currentIP NOTIFY connectionInfoUpdated)
    Q_PROPERTY(QString ipInfo READ ipInfo NOTIFY connectionInfoUpdated)  // IP详情：ASN | 组织 | 地区
    Q_PROPERTY(int currentDelay READ currentDelay NOTIFY connectionInfoUpdated)
    Q_PROPERTY(QVariantList latencyHistory READ latencyHistory NOTIFY latencyHistoryChanged)

    // 核心版本信息
    Q_PROPERTY(QString coreVersion READ coreVersion CONSTANT)

public:
    // ========================================================================
    // 枚举类型
    // ========================================================================

    /**
     * @enum ConnectionState
     * @brief VPN连接状态枚举
     *
     * 定义VPN连接的各种可能状态，用于状态机管理和UI状态显示
     */
    enum ConnectionState {
        Disconnected,   ///< 未连接状态 - 初始状态或断开连接后的状态
        Connecting,     ///< 正在连接 - 正在建立VPN连接
        Connected,      ///< 已连接状态 - VPN连接已成功建立
        Disconnecting,  ///< 正在断开 - 正在断开VPN连接
        Reconnecting,   ///< 正在重连 - 连接断开后自动重连中
        Error          ///< 错误状态 - 发生错误，连接失败
    };
    Q_ENUM(ConnectionState)

    // ========================================================================
    // 单例模式
    // ========================================================================

    /**
     * @brief 获取VPNManager单例实例
     * @return VPNManager& 全局唯一的VPNManager实例引用
     *
     * @details 该方法采用Meyer's Singleton实现，线程安全（C++11及以上）
     *
     * @note 首次调用时创建实例，之后返回同一实例
     */
    static VPNManager& instance();

    /**
     * @brief 禁用拷贝构造函数
     * @details 单例模式不允许拷贝
     */
    VPNManager(const VPNManager&) = delete;

    /**
     * @brief 禁用赋值操作符
     * @details 单例模式不允许赋值
     */
    VPNManager& operator=(const VPNManager&) = delete;

    /**
     * @brief 析构函数
     * @details 清理所有资源，断开活动连接
     */
    ~VPNManager();

    // ========================================================================
    // 依赖注入
    // ========================================================================

    /**
     * @brief 设置服务器提供者
     * @param provider 实现 IServerProvider 接口的对象指针
     *
     * @details
     * 通过依赖注入设置服务器提供者，解耦 VPNManager 对 SubscriptionManager 的直接依赖。
     * JinGo 应用在初始化时调用此方法注入 SubscriptionManager。
     *
     * @note 如果不设置，restoreLastSelectedServer() 等需要获取服务器的方法将无法工作
     */
    void setServerProvider(IServerProvider* provider);

    /**
     * @brief 获取服务器提供者
     * @return IServerProvider* 服务器提供者指针，可能为 nullptr
     */
    IServerProvider* serverProvider() const;

    // ========================================================================
    // 连接控制方法
    // ========================================================================

    /**
     * @brief 连接到指定服务器
     *
     * @param server 服务器配置对象指针，包含连接所需的所有参数
     *
     * @details
     * 启动VPN连接流程，包括：
     * 1. 验证服务器配置
     * 2. 生成Xray配置
     * 3. 启动Xray核心
     * 4. 根据模式启动TUN或系统代理
     * 5. 配置路由（如需要）
     *
     * @note
     * - 如果已有活动连接，会先断开现有连接
     * - 该方法是异步的，连接结果通过信号通知
     * - 服务器指针不能为nullptr
     *
     * @warning 确保在调用前server对象有效
     *
     * @see connected(), connectFailed()
     */
    Q_INVOKABLE void connecting(Server* server);

    /**
     * @brief 断开当前VPN连接
     *
     * @details
     * 执行完整的断开流程：
     * 1. 停止TUN桥接或清除系统代理
     * 2. 停止Xray核心
     * 3. 关闭TUN设备
     * 4. 清除路由表
     * 5. 保存流量统计到数据库
     *
     * @note
     * - 如果当前没有连接，此方法不执行任何操作
     * - 断开过程是同步的
     *
     * @see disconnected()
     */
    Q_INVOKABLE void disconnect();

    /**
     * @brief 重新连接到当前服务器
     *
     * @details
     * 断开当前连接后立即重连到同一服务器
     * 常用于配置更改后应用新配置
     *
     * @note 如果没有当前服务器，则不执行任何操作
     *
     * @see connecting()
     */
    Q_INVOKABLE void reconnect();

    // ========================================================================
    // 状态查询方法
    // ========================================================================

    /**
     * @brief 获取当前连接状态
     * @return ConnectionState 当前连接状态枚举值
     *
     * @see ConnectionState
     */
    Q_INVOKABLE ConnectionState state() const;

    /**
     * @brief 获取状态描述信息
     * @return QString 当前状态的文本描述，已本地化
     *
     * @details 返回适合显示给用户的状态描述文本
     *
     * @note 文本已根据系统语言本地化
     */
    Q_INVOKABLE QString stateMessage() const;

    /**
     * @brief 获取当前连接的服务器
     * @return Server* 当前服务器对象指针，未连接时返回nullptr
     *
     * @warning 返回的指针可能为nullptr，使用前需检查
     */
    Q_INVOKABLE Server* currentServer() const;

    /**
     * @brief 选择服务器（不立即连接）
     * @param server 要选择的服务器对象指针
     *
     * @details 将指定服务器设置为当前服务器，并保存到本地存储
     * - 保存服务器ID，下次启动时自动恢复
     * - 不会立即建立连接，仅设置为选中状态
     * - 发出currentServerChanged信号
     *
     * @note 如果需要连接，应该调用connecting()方法
     */
    Q_INVOKABLE void selectServer(Server* server);

    /**
     * @brief 检查是否正在连接
     * @return bool true=正在连接或重连中，false=其他状态
     *
     * @details 包括Connecting和Reconnecting状态
     */
    Q_INVOKABLE bool isConnecting() const;

    /**
     * @brief 检查是否已连接
     * @return bool true=已连接，false=未连接
     *
     * @details 仅在Connected状态时返回true
     */
    Q_INVOKABLE bool isConnected() const;

    /**
     * @brief 检查是否正在断开连接
     * @return bool true=正在断开，false=其他状态
     */
    Q_INVOKABLE bool isDisconnecting() const;

    // ========================================================================
    // 网络测试方法
    // ========================================================================

    /**
     * @brief 测试代理延时
     *
     * @details
     * 通过SOCKS5代理连接测试服务器延时
     * 测试完成后发出proxyDelayTestCompleted信号
     *
     * @note 需要在VPN连接状态下使用
     */
    Q_INVOKABLE void testProxyDelay();

    /**
     * @brief 执行下载速度测试
     *
     * @details
     * 下载指定大小的文件来测量实际下载速度
     * 测试完成后发出speedTestCompleted信号
     *
     * @note 需要在VPN连接状态下使用
     */
    Q_INVOKABLE void performSpeedTest();

    /**
     * @brief 测试指定服务器的下载速度（不需要VPN连接）
     *
     * @param serverId 要测试的服务器ID
     *
     * @details
     * 创建临时Xray实例，测试指定服务器的下载速度
     * 测试完成后发出serverSpeedTestCompleted信号
     *
     * @note 不需要VPN处于连接状态
     */
    Q_INVOKABLE void testServerSpeed(const QString& serverId);

    /**
     * @brief 直接Ping测试服务器IP
     *
     * @details
     * 使用系统ping命令直接测试服务器IP延时
     * 测试完成后发出directPingTestCompleted信号
     *
     * @note 可以在任何状态下使用
     */
    Q_INVOKABLE void testDirectPing();

    /**
     * @brief 测试特定服务器的延时
     *
     * @param server 要测试的服务器对象
     *
     * @details
     * 根据ConfigManager中的delayTestMethod设置选择测试方法：
     * - method=0: 使用CONNECT测试（通过代理连接到测试URL）
     * - method=1: 使用PING测试（直接ping服务器IP）
     * 测试完成后发出serverDelayTestCompleted信号
     *
     * @note 不使用本地代理，直接测试到服务器的延时
     */
    Q_INVOKABLE void testServerDelay(Server* server);

    // ========================================================================
    // 流量统计方法
    // ========================================================================

    /**
     * @brief 获取累计上传字节数
     * @return quint64 当前会话的上传字节总数
     *
     * @details 从连接建立开始累计，断开连接时重置为0
     *
     * @note 单位：字节（Bytes）
     */
    quint64 uploadBytes() const;

    /**
     * @brief 获取累计下载字节数
     * @return quint64 当前会话的下载字节总数
     *
     * @details 从连接建立开始累计，断开连接时重置为0
     *
     * @note 单位：字节（Bytes）
     */
    quint64 downloadBytes() const;

    /**
     * @brief 获取当前上传速度
     * @return quint64 实时上传速度
     *
     * @details 基于最近的流量采样计算得出
     *
     * @note 单位：字节/秒（Bytes/s）
     */
    quint64 uploadSpeed() const;

    /**
     * @brief 获取当前下载速度
     * @return quint64 实时下载速度
     *
     * @details 基于最近的流量采样计算得出
     *
     * @note 单位：字节/秒（Bytes/s）
     */
    quint64 downloadSpeed() const;

    /**
     * @brief 获取已连接时长
     * @return qint64 从连接建立到现在的秒数，未连接时返回0
     *
     * @note 单位：秒（Seconds）
     */
    qint64 connectedDuration() const;

    /**
     * @brief 获取当前外网IP地址
     * @return QString 当前的公网IP地址，未连接或获取失败返回空字符串
     */
    QString currentIP() const;

    /**
     * @brief 获取核心版本信息
     * @return QString 格式化的核心版本字符串，如 "SuperRay v1.0.0 | Xray-core v1.8.0"
     */
    QString coreVersion() const;

    /**
     * @brief 获取IP详情（ASN、组织、地区）
     * @return QString 格式化的IP详情字符串，如 "AS12345 | Company | City, Country"
     */
    QString ipInfo() const;

    /**
     * @brief 获取当前延时
     * @return int 当前的网络延时（毫秒），未连接或测量失败返回-1
     */
    int currentDelay() const;

    /**
     * @brief 获取延时历史数据
     * @return QVariantList 包含 timestamp 和 latency 的 QVariantMap 列表
     */
    QVariantList latencyHistory() const;

    /**
     * @brief 添加延时数据到历史
     * @param latency 延时值（毫秒）
     */
    void addLatencyToHistory(int latency);

    /**
     * @brief 清空延时历史数据
     */
    void clearLatencyHistory();

    // ========================================================================
    // 配置选项方法
    // ========================================================================

    /**
     * @brief 获取自动重连设置
     * @return bool true=已启用，false=已禁用
     *
     * @details 启用后，连接断开时会自动尝试重连
     */
    bool autoReconnect() const;

    /**
     * @brief 设置自动重连
     * @param enabled true=启用自动重连，false=禁用自动重连
     *
     * @details
     * 启用自动重连后，以下情况会触发自动重连：
     * - 连接意外断开
     * - Xray核心错误
     * - TUN设备错误
     *
     * @note 设置变更会触发autoReconnectChanged信号
     *
     * @see autoReconnectChanged()
     */
    void setAutoReconnect(bool enabled);

    /**
     * @brief 获取TUN模式设置
     * @return bool true=TUN模式，false=系统代理模式
     *
     * @details
     * - TUN模式：创建虚拟网卡，通过tun2socks桥接到SOCKS5
     * - 系统代理模式：配置系统HTTP/SOCKS代理
     */
    bool isTunMode() const;

    /**
     * @brief 设置VPN工作模式
     * @param enabled true=TUN模式，false=系统代理模式
     *
     * @details
     * 模式切换需要重新连接才能生效
     *
     * @note
     * - 设置会保存到配置文件
     * - 如果当前已连接，需要手动重连应用新设置
     * - 设置变更会触发tunModeChanged信号
     *
     * @see tunModeChanged(), reconnect()
     */
    void setTunMode(bool enabled);

    /**
     * @brief 获取TUN设备名称
     * @return QString TUN设备名称（如"utun3"、"tun0"等）
     *
     * @details
     * 设备名称由平台接口创建时分配，不同平台格式不同：
     * - macOS: utunX
     * - Linux: tunX
     * - Windows: 自定义名称
     *
     * @note 仅在TUN模式下有效
     */
    QString tunDeviceName() const;

    // ========================================================================
    // 系统诊断方法
    // ========================================================================

    /**
     * @brief 执行系统诊断检查
     * @return bool 系统就绪返回true，存在关键问题返回false
     *
     * @details
     * 检查系统是否满足TUN模式的先决条件
     * 包括权限、驱动、网络等检查
     */
    Q_INVOKABLE bool checkSystemRequirements();

    /**
     * @brief 获取上次诊断的详细报告
     * @return QString 格式化的诊断报告文本
     *
     * @details
     * 返回最后一次运行的诊断结果，包括所有检查项和建议
     */
    Q_INVOKABLE QString getDiagnosticReport();

    /**
     * @brief 诊断特定错误
     * @param errorMessage 错误消息
     * @return QString 诊断和建议
     *
     * @details
     * 根据错误消息分析问题根源并提出解决方案
     */
    Q_INVOKABLE QString diagnoseError(const QString& errorMessage);

    /**
     * @brief 读取 Extension 诊断日志 (iOS/macOS)
     * @return QString 诊断日志内容
     *
     * @details
     * 读取存储在 App Group 共享容器中的 extension_early_diag.log 和 extension.log 文件
     * 该日志记录了 Extension 进程启动的早期阶段，用于诊断 VPN 连接问题
     */
    Q_INVOKABLE QString getExtensionLog();

    /**
     * @brief 清除 Extension 诊断日志 (iOS/macOS)
     *
     * @details
     * 删除 App Group 共享容器中的日志文件
     */
    Q_INVOKABLE void clearExtensionLog();

signals:
    // ========================================================================
    // 连接状态信号
    // ========================================================================

    /**
     * @brief 连接状态改变信号
     * @param state 新的连接状态
     *
     * @details
     * 当连接状态发生变化时发出，用于更新UI状态
     * 状态转换通常遵循以下流程：
     * Disconnected -> Connecting -> Connected -> Disconnecting -> Disconnected
     *
     * @see ConnectionState
     */
    void stateChanged(ConnectionState state);

    /**
     * @brief 状态消息改变信号
     * @param message 新的状态描述消息
     *
     * @details
     * 提供更详细的状态描述，适合显示给用户
     * 消息已本地化
     */
    void stateMessageChanged(const QString& message);

    /**
     * @brief 连接成功信号
     *
     * @details
     * 当VPN连接完全建立后发出
     * 此时所有组件（Xray、TUN/代理、路由）都已就绪
     *
     * @note 在状态变为Connected后立即发出
     */
    void connected();

    /**
     * @brief 连接失败信号
     * @param reason 失败原因描述
     *
     * @details
     * 连接建立过程中发生错误时发出
     * 如果启用自动重连，可能会自动触发重连
     *
     * @see autoReconnect()
     */
    void connectFailed(const QString& reason);

    /**
     * @brief 断开连接信号
     *
     * @details
     * 当VPN连接完全断开后发出
     * 此时所有资源都已清理
     *
     * @note 在状态变为Disconnected后立即发出
     */
    void disconnected();

    /**
     * @brief 开始重连信号
     * @param attempt 当前尝试次数（从1开始）
     * @param maxAttempts 最大尝试次数
     *
     * @details
     * 自动重连开始时发出
     * 可用于显示重连倒计时UI
     */
    void reconnectStarted(int attempt, int maxAttempts);

    /**
     * @brief 发生错误信号
     * @param error 错误信息描述
     *
     * @details
     * 运行过程中发生错误时发出
     * 包括但不限于：
     * - Xray核心错误
     * - TUN设备错误
     * - 配置错误
     * - 网络错误
     */
    void errorOccurred(const QString& error);

    // ========================================================================
    // 统计信号
    // ========================================================================

    /**
     * @brief 流量统计更新信号
     * @param uploadBytes 累计上传字节数
     * @param downloadBytes 累计下载字节数
     *
     * @details
     * 定期发出（通常每秒一次）
     * 用于更新UI上的流量显示
     *
     * @note 连接期间持续发出，断开后停止
     */
    void statsUpdated(quint64 uploadBytes, quint64 downloadBytes);

    /**
     * @brief 速度更新信号
     * @param uploadSpeed 当前上传速度（字节/秒）
     * @param downloadSpeed 当前下载速度（字节/秒）
     *
     * @details
     * 定期发出（通常每秒一次）
     * 用于更新UI上的速度显示
     */
    void speedUpdated(quint64 uploadSpeed, quint64 downloadSpeed);

    /**
     * @brief 速度测试完成信号
     * @param speedBps 测试速度（字节/秒），失败时为0
     * @param error 错误信息，成功时为空字符串
     */
    void speedTestCompleted(double speedBps, QString error);

    /**
     * @brief 连接信息更新信号
     *
     * @details
     * 当IP地址或延时信息更新时发出
     * 用于更新UI上的IP和延时显示
     */
    void connectionInfoUpdated();

    /**
     * @brief 延时历史数据更新信号
     * 当延时历史列表有新数据时发出
     */
    void latencyHistoryChanged();

    /**
     * @brief 代理延时测试完成信号
     * @param delay 测试延时（毫秒），-1表示测试失败
     * @param errorMessage 错误信息（如果测试失败）
     *
     * @details
     * 当testProxyDelay()测试完成时发出
     */
    void proxyDelayTestCompleted(int delay, const QString& errorMessage);

    /**
     * @brief 直接Ping测试完成信号
     * @param delay 测试延时（毫秒），-1表示测试失败
     * @param errorMessage 错误信息（如果测试失败）
     *
     * @details
     * 当testDirectPing()测试完成时发出
     */
    void directPingTestCompleted(int delay, const QString& errorMessage);

    /**
     * @brief 服务器延时测试完成信号
     * @param serverId 服务器ID
     * @param delay 测试延时（毫秒），-1表示测试失败
     * @param errorMessage 错误信息（如果测试失败）
     *
     * @details
     * 当testServerDelay()测试完成时发出，包含服务器ID以便更新相应的服务器延时显示
     */
    void serverDelayTestCompleted(const QString& serverId, int delay, const QString& errorMessage);

    // ========================================================================
    // 配置信号
    // ========================================================================

    /**
     * @brief 当前服务器改变信号
     *
     * @details
     * 当连接到新服务器时发出
     * 用于更新UI显示当前服务器信息
     */
    void currentServerChanged();

    /**
     * @brief 自动重连设置改变信号
     * @param enabled 新的设置值
     *
     * @details
     * 调用setAutoReconnect()后发出
     */
    void autoReconnectChanged(bool enabled);

    /**
     * @brief TUN模式设置改变信号
     * @param enabled 新的设置值
     *
     * @details
     * 调用setTunMode()后发出
     *
     * @note 设置变更需要重新连接才能生效
     */
    void tunModeChanged(bool enabled);

    // ========================================================================
    // TUN设备信号
    // ========================================================================

    /**
     * @brief TUN设备创建成功信号
     * @param deviceName 创建的设备名称
     *
     * @details
     * TUN设备创建成功后发出
     * 仅在TUN模式下有效
     */
    void tunDeviceCreated(const QString& deviceName);

    /**
     * @brief TUN设备关闭信号
     *
     * @details
     * TUN设备关闭后发出
     * 仅在TUN模式下有效
     */
    void tunDeviceClosed();

    /**
     * @brief TUN设备错误信号
     * @param error 错误信息
     *
     * @details
     * TUN设备操作失败时发出
     * 通常会导致连接失败或断开
     */
    void tunDeviceError(const QString& error);

    /**
     * @brief TUN桥接启动信号
     *
     * @details
     * tun2socks进程启动成功后发出
     */
    void tunBridgeStarted();

    /**
     * @brief TUN桥接停止信号
     *
     * @details
     * tun2socks进程停止后发出
     */
    void tunBridgeStopped();

    /**
     * @brief TUN桥接错误信号
     * @param error 错误信息
     *
     * @details
     * tun2socks进程异常时发出
     * 通常会导致连接断开
     */
    void tunBridgeError(const QString& error);

private:
    /**
     * @brief 私有构造函数（单例模式）
     * @param parent 父对象指针，通常为nullptr
     *
     * @details
     * 初始化所有成员变量和组件
     * 设置信号连接
     * 加载配置
     */
    explicit VPNManager(QObject* parent = nullptr);

    // ========================================================================
    // 私有方法 - 连接管理
    // ========================================================================

    /**
     * @brief 启动连接流程（内部方法）
     * @param server 目标服务器配置
     * @return bool 启动成功返回true，失败返回false
     *
     * @details
     * 执行完整的连接建立流程：
     * 1. 生成Xray配置
     * 2. 启动Xray核心
     * 3. 等待SOCKS5端口就绪
     * 4. 根据模式启动TUN或系统代理
     * 5. 配置路由（如需要）
     */
    bool startConnection(Server* server);

    /**
     * @brief 停止连接（内部方法）
     *
     * @details
     * 执行完整的断开流程并清理所有资源
     */
    void stopConnection();

    /**
     * @brief 启动TUN模式（内部方法 - 平台分发）
     * @param server VPN服务器指针，不能为NULL
     * @return bool 成功返回true，失败返回false
     *
     * @details
     * 根据平台调用对应的实现函数：
     * - macOS/iOS: startTunMode_Apple()
     * - Windows: startTunMode_Windows()
     * - Android: startTunMode_Android()
     * - Linux: startTunMode_Linux()
     */
    bool startTunMode(Server* server);

    // ========================================================================
    // 平台特定的 TUN 模式实现
    // ========================================================================

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    /**
     * @brief macOS/iOS 平台 TUN 模式启动实现
     * @param server VPN服务器指针，不能为NULL
     * @return bool 成功返回true，失败返回false
     *
     * @details 使用 Network Extension 框架实现 VPN
     */
    bool startTunMode_Apple(Server* server);

    /**
     * @brief macOS/iOS 平台 TUN 模式停止实现
     */
    void stopTunMode_Apple();
#endif

#ifdef Q_OS_WIN
    /**
     * @brief Windows 平台 TUN 模式启动实现
     * @param server VPN服务器指针，不能为NULL
     * @return bool 成功返回true，失败返回false
     *
     * @details 使用 WinTun + tun2socks + Xray 实现 VPN
     */
    bool startTunMode_Windows(Server* server);

    /**
     * @brief Windows 平台 TUN 模式停止实现
     */
    void stopTunMode_Windows();

    /**
     * @brief Windows 平台执行隐藏控制台的外部命令
     * @param program 程序名称（如"cmd"、"netsh"、"powershell"）
     * @param arguments 命令参数列表
     * @return bool 执行成功返回true，失败返回false
     *
     * @details
     * 执行系统命令但不显示控制台窗口，避免频繁出现黑窗口
     * 通常用于执行route、netsh等网络配置命令
     *
     * @note 仅在Windows平台有效
     */
    bool executeCommand(const QString& program, const QStringList& arguments);
#endif

#ifdef Q_OS_ANDROID
    /**
     * @brief Android 平台 TUN 模式启动实现
     * @param server VPN服务器指针，不能为NULL
     * @return bool 成功返回true，失败返回false
     *
     * @details 使用 VPN Service + SuperRay TUN 实现 VPN
     */
    bool startTunMode_Android(Server* server);

    /**
     * @brief Android 平台 TUN 模式停止实现
     */
    void stopTunMode_Android();
#endif

#ifdef Q_OS_LINUX
    /**
     * @brief Linux 平台 TUN 模式启动实现
     * @param server VPN服务器指针，不能为NULL
     * @return bool 成功返回true，失败返回false
     *
     * @details 使用传统 TUN 设备 + SuperRay 实现 VPN
     */
    bool startTunMode_Linux(Server* server);

    /**
     * @brief Linux 平台 TUN 模式停止实现
     */
    void stopTunMode_Linux();
#endif

#if defined(Q_OS_ANDROID) || defined(Q_OS_LINUX)
    /**
     * @brief Linux/Android 通用 TUN 模式实现
     * @param server VPN服务器指针，不能为NULL
     * @return bool 成功返回true，失败返回false
     *
     * @details
     * Android 和 Linux 使用相同的 TUN 实现逻辑：
     * 1. 创建并配置 TUN 设备
     * 2. 启动 SuperRay TUN 桥接
     * 3. 配置系统路由
     */
    bool startTunMode_Linux_Common(Server* server);

    /**
     * @brief Linux/Android 通用 TUN 停止实现
     */
    void stopTunMode_Linux_Common();
#endif

    /**
     * @brief 停止TUN模式（内部方法 - 平台分发）
     *
     * @details
     * 停止tun2socks桥接，关闭TUN设备
     */
    void stopTunMode();

    /**
     * @brief 启动系统代理模式（内部方法）
     * @return bool 成功返回true，失败返回false
     *
     * @details
     * 配置系统HTTP/SOCKS代理
     */
    bool startSystemProxyMode();

    /**
     * @brief 停止系统代理模式（内部方法）
     *
     * @details
     * 清除系统代理配置
     */
    void stopSystemProxyMode();

    // ========================================================================
    // 私有方法 - TUN设备管理
    // ========================================================================

    /**
     * @brief 创建TUN设备（内部方法）
     * @return bool 成功返回true，失败返回false
     *
     * @details
     * 调用平台接口创建TUN虚拟网卡
     */
    bool createTunDevice();

    /**
     * @brief 配置TUN设备（内部方法）
     * @return bool 成功返回true，失败返回false
     *
     * @details
     * 设置TUN设备的IP地址、子网掩码和MTU
     */
    bool configureTunDevice();

    /**
     * @brief 关闭TUN设备（内部方法）
     *
     * @details
     * 销毁TUN虚拟网卡
     */
    void closeTunDevice();

    /**
     * @brief 检查TUN设备是否就绪（内部方法）
     * @return bool 就绪返回true，否则返回false
     *
     * @details
     * 验证TUN设备可用性
     */
    bool isTunDeviceReady();

    // ========================================================================
    // 私有方法 - 系统代理管理
    // ========================================================================

    /**
     * @brief 配置系统代理（内部方法）
     *
     * @details
     * 设置系统级HTTP和SOCKS代理
     */
    void setupSystemProxy();

    /**
     * @brief 清除系统代理（内部方法）
     *
     * @details
     * 恢复原有代理设置
     */
    void clearSystemProxy();

    // ========================================================================
    // 私有方法 - 路由管理
    // ========================================================================

    /**
     * @brief 配置路由表（内部方法）
     * @return bool 成功返回true，失败返回false
     *
     * @details
     * 添加路由规则，将流量导向TUN设备
     * 使用分片路由（0.0.0.0/1 + 128.0.0.0/1）避免路由循环
     */
    bool setupRouting();

    /**
     * @brief 清除路由表（内部方法）
     * @return bool 成功返回true，失败返回false
     *
     * @details
     * 移除之前添加的路由规则
     */
    bool clearRouting();

    // ========================================================================
    // 私有方法 - 辅助功能
    // ========================================================================

    /**
     * @brief 恢复上次选中的服务器（内部方法）
     *
     * @details
     * 从本地存储中读取上次选中的服务器ID
     * 从数据库中查找对应的服务器并设置为当前服务器
     */
    void restoreLastSelectedServer();

    /**
     * @brief 设置信号连接（内部方法）
     *
     * @details
     * 连接所有内部组件的信号到相应的槽函数
     */
    void setupConnections();

    /**
     * @brief 加载配置（内部方法）
     *
     * @details
     * 从配置文件加载各项设置
     */
    void loadSettings();

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    /**
     * @brief 在启动时同步VPN状态
     *
     * @details 检查VPN是否已经连接（例如系统自动重连），如果是则同步状态并启动统计定时器
     */
    void syncVPNStateOnStartup();
#endif

    /**
     * @brief 等待SOCKS5端口就绪（内部方法）
     * @param port SOCKS5监听端口
     * @return bool 就绪返回true，超时返回false
     *
     * @details
     * 轮询检查SOCKS5端口是否可连接
     * 最多重试25次，每次间隔200ms
     */
    bool waitForSocksReady(int port);

    /**
     * @brief 设置连接状态（内部方法）
     * @param state 新状态
     *
     * @details
     * 更新状态并发出stateChanged信号
     */
    void setState(ConnectionState state);

    /**
     * @brief 设置状态消息（内部方法）
     * @param message 新消息
     *
     * @details
     * 更新消息并发出stateMessageChanged信号
     */
    void setStateMessage(const QString& message);

    /**
     * @brief 启动重连定时器（内部方法）
     *
     * @details
     * 设置延迟后触发重连
     */
    void startReconnectTimer();

    /**
     * @brief 停止重连定时器（内部方法）
     */
    void stopReconnectTimer();

    /**
     * @brief 重置重连计数器（内部方法）
     *
     * @details
     * 将重连尝试次数重置为0
     */
    void resetReconnectCounter();

    /**
     * @brief 检测当前外网IP地址（内部方法）
     *
     * @details
     * 通过HTTP请求IP查询服务获取当前公网IP
     * 成功后更新m_currentIP并发出connectionInfoUpdated信号
     */
    void detectCurrentIP();

    /**
     * @brief 检测当前网络延时（内部方法）
     *
     * @details
     * 通过测量HTTP请求延时或ping服务器来获取网络延时
     * 成功后更新m_currentDelay并发出connectionInfoUpdated信号
     */
    void detectCurrentDelay();

    /**
     * @brief 处理延迟测试结果
     * @param delay 延迟时间（毫秒），-1表示失败
     */
    void handleDelayResult(int delay);

    /**
     * @brief 处理IP检测失败
     * @param error 错误信息
     */
    void handleIPDetectionFailure(const QString& error);

private slots:
    // ========================================================================
    // 槽函数 - 健康检查
    // ========================================================================

    /**
     * @brief 连接健康检查槽函数
     *
     * @details
     * 定期检查连接健康状态，包括：
     * - 检查 SuperRay/Xray 是否正常运行
     * - 连续失败3次后自动重连
     */
    void checkConnectionHealth();

    // ========================================================================
    // 槽函数 - VPNCore信号处理
    // ========================================================================

    /**
     * @brief VPNCore状态改变槽函数
     * @param state 新状态
     */
    void onVPNCoreStateChanged(int state);

    /**
     * @brief VPNCore流量统计更新槽函数
     * @param uploadBytes 上传字节数
     * @param downloadBytes 下载字节数
     */
    void onVPNCoreStatsUpdated(quint64 uploadBytes, quint64 downloadBytes);

    /**
     * @brief VPNCore速度更新槽函数
     * @param uploadSpeed 上传速度
     * @param downloadSpeed 下载速度
     */
    void onVPNCoreSpeedUpdated(quint64 uploadSpeed, quint64 downloadSpeed);

    /**
     * @brief VPNCore错误槽函数
     * @param error 错误信息
     */
    void onVPNCoreError(const QString& error);

    // ========================================================================
    // 槽函数 - 定时器
    // ========================================================================

    /**
     * @brief 重连定时器超时槽函数
     *
     * @details
     * 触发自动重连
     */
    void onReconnectTimerTimeout();

    /**
     * @brief 连接超时槽函数
     *
     * @details
     * 连接建立超时时触发
     */
    void onConnectionTimeout();

    /**
     * @brief TUN模式统计数据更新槽函数（macOS/iOS）
     *
     * @details
     * 从SharedDefaults读取Network Extension写入的统计数据
     * 计算速度并发送信号更新UI
     */
    void onTunnelStatsTimerTimeout();

    /**
     * @brief 统计数据定期保存槽函数
     *
     * @details
     * 定期（每60秒）保存统计数据增量到数据库
     * 防止应用崩溃或强制退出时丢失流量统计数据
     */
    void onStatsSaveTimerTimeout();

    // ========================================================================
    // 槽函数 - TUN设备
    // ========================================================================

    /**
     * @brief TUN设备错误槽函数
     * @param error 错误信息
     */
    void onTunDeviceError(const QString& error);

    /**
     * @brief TUN数据包接收槽函数
     * @param packet 接收到的数据包
     */
    void onTunPacketReceived(const QByteArray& packet);

    // ========================================================================
    // 私有辅助方法 - SuperRay API 响应解析
    // ========================================================================

    /**
     * @brief 检查SuperRay API响应是否包含错误
     * @param jsonResult SuperRay API返回的JSON字符串
     * @return true表示包含错误，false表示成功
     *
     * @details
     * SuperRay API返回JSON格式的响应，可能为NULL或包含error字段
     * 使用此方法统一检查所有API调用的错误状态
     */
    bool hasSuperRayError(const char* jsonResult);

    /**
     * @brief 从SuperRay API响应中提取错误信息
     * @param jsonResult SuperRay API返回的JSON字符串
     * @return 错误信息字符串，如果没有错误则返回空字符串
     */
    QString parseSuperRayError(const char* jsonResult);

    /**
     * @brief 从SuperRay API响应中提取数据
     * @param jsonResult SuperRay API返回的JSON字符串
     * @param key 要提取的字段名（可使用"path/to/field"格式访问嵌套字段）
     * @return 提取的值，如果不存在则返回空字符串
     *
     * @example
     * char* result = SuperRay_CreateSystemTUN(config);
     * QString deviceId = parseSuperRayData(result, "data/deviceId");
     */
    QString parseSuperRayData(const char* jsonResult, const QString& key = "data");

    /**
     * @brief 检查并处理SuperRay API调用结果
     * @param result SuperRay API返回的JSON字符串
     * @param operationName 操作名称（用于日志）
     * @return true表示成功，false表示失败
     *
     * @details
     * 这是一个统一的错误处理方法，结合日志记录和错误提取
     * 如果出错会自动记录错误日志
     */
    bool checkSuperRayResult(const char* result, const QString& operationName);

private:
    // ========================================================================
    // 成员变量 - 核心组件
    // ========================================================================

    VPNCore& m_vpnCore;                      ///< VPN核心引擎（包装XrayCBridge）
    ConfigManager& m_configManager;          ///< 配置管理器引用
    PlatformInterface* m_platformInterface;  ///< 平台接口指针
    IServerProvider* m_serverProvider;       ///< 服务器提供者（通过依赖注入设置）

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    NetworkExtensionManager* m_networkExtensionManager;  ///< Network Extension管理器（macOS/iOS TUN模式）
#endif

#ifdef Q_OS_WIN
    std::shared_ptr<JinGo::WinTunManager> m_winTunManager;  ///< WinTun设备管理器（Windows TUN模式，SuperRay DLL通过C API直接调用）
#endif


    // ========================================================================
    // 成员变量 - 状态信息
    // ========================================================================

    ConnectionState m_state;          ///< 当前连接状态
    QString m_stateMessage;           ///< 当前状态描述消息
    QPointer<Server> m_currentServer; ///< 当前连接的服务器指针（安全指针，防止野指针）
    mutable bool m_serverRestored;    ///< 是否已从存储恢复服务器
    bool m_useTUN;                    ///< 是否使用TUN模式（true=TUN，false=SystemProxy）
    QDateTime m_connectionStartTime;  ///< 连接开始时间
    bool m_licenseValidatedThisSession; ///< 本次会话是否已验证授权（连接时验证一次）

    // ========================================================================
    // 成员变量 - 统计信息
    // ========================================================================

    quint64 m_uploadBytes;      ///< 累计上传字节数
    quint64 m_downloadBytes;    ///< 累计下载字节数
    quint64 m_uploadSpeed;      ///< 当前上传速度（字节/秒）
    quint64 m_downloadSpeed;    ///< 当前下载速度（字节/秒）
    qint64 m_connectedTime;     ///< 已连接时长（秒）

    // 定期保存统计数据使用（防止崩溃丢失数据）
    quint64 m_lastSavedUploadBytes;    ///< 上次保存时的上传字节数
    quint64 m_lastSavedDownloadBytes;  ///< 上次保存时的下载字节数

    QString m_currentIP;        ///< 当前外网IP地址
    QString m_ipInfo;           ///< IP详情（ASN | 组织 | 地区）
    int m_currentDelay;         ///< 当前网络延时（毫秒）
    QList<QPair<qint64, int>> m_latencyHistory;  ///< 延时历史数据 (时间戳毫秒, 延时毫秒)
    int m_ipDetectionRetryCount;    ///< IP检测重试次数
    int m_delayDetectionRetryCount; ///< 延时检测重试次数
    int m_statsQueryCount;          ///< 统计查询计数（用于控制 IP/延迟查询频率）

    // ========================================================================
    // 成员变量 - 配置选项
    // ========================================================================

    bool m_autoReconnect;           ///< 是否启用自动重连
    int m_reconnectDelay;           ///< 重连延迟（毫秒）
    int m_maxReconnectAttempts;     ///< 最大重连尝试次数
    int m_reconnectAttempts;        ///< 当前重连尝试次数
    int m_connectionTimeout;        ///< 连接超时时间（毫秒）
    int m_consecutiveHealthCheckFailures;  ///< 连续健康检查失败计数

    // ========================================================================
    // 成员变量 - 定时器
    // ========================================================================

    QTimer* m_reconnectTimer;           ///< 重连定时器
    QTimer* m_connectionTimeoutTimer;   ///< 连接超时定时器
    QTimer* m_healthCheckTimer;         ///< 健康检查定时器
    QTimer* m_tunnelStatsTimer;         ///< TUN模式统计数据定时器（macOS/iOS）
    QTimer* m_connectionInfoTestTimer;  ///< 连接信息测试定时器（延迟和IP）
    QTimer* m_latencyTestTimer;         ///< 延时测试定时器（用户配置间隔）
    QTimer* m_statsSaveTimer;           ///< 统计数据定期保存定时器（每60秒，防止崩溃丢失数据）
    bool m_initialTestDone;             ///< 初始测试是否完成（连接后30秒）

    // ========================================================================
    // 成员变量 - 异步任务
    // ========================================================================

    QFuture<void> m_connectionTask;     ///< 连接任务的Future对象

    // ========================================================================
    // 成员变量 - TUN配置
    // ========================================================================

    bool m_tunEnabled;          ///< 是否启用TUN模式
    QString m_tunDeviceName;    ///< TUN设备名称
    QString m_tunIpAddress;     ///< TUN设备IP地址
    QString m_tunNetmask;       ///< TUN设备子网掩码
    QString m_tunGateway;       ///< TUN网关地址
    int m_tunMtu;               ///< TUN设备MTU值
    QString m_originalDefaultGateway; ///< 原始系统默认网关（用于VPN断开时恢复）
    QString m_originalInterfaceName;  ///< 原始物理网卡名称（用于恢复默认路由时指定正确的接口）
    int m_originalInterfaceIndex;     ///< 原始物理网卡接口索引（备用，用于route命令的if参数）
    QString m_superrayInstanceId; ///< SuperRay实例ID（用于Windows TUN模式）

    // ========================================================================
    // 成员变量 - 诊断信息
    // ========================================================================

    QString m_diagnosticReport;  ///< 上次诊断的详细报告
};

// 声明ConnectionState为Qt元类型，支持跨线程信号槽
Q_DECLARE_METATYPE(VPNManager::ConnectionState)

#endif // VPNMANAGER_H