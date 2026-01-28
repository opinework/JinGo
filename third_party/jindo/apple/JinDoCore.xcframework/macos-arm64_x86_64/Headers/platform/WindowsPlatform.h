/**
 * @file WindowsPlatform.h
 * @brief Windows平台实现头文件
 * @details 实现Windows平台特定的VPN功能，使用TAP-Windows适配器
 * @author JinGo VPN Team
 * @date 2025
 */

#ifndef WINDOWSPLATFORM_H
#define WINDOWSPLATFORM_H

#include "PlatformInterface.h"
#include <QString>

#ifdef Q_OS_WIN
    // 必须先包含 winsock2.h，再包含 windows.h
    #include <winsock2.h>
    #include <windows.h>

    // ========================================================================
    // TAP-Windows IOCTL 定义
    // ========================================================================

    /**
     * @def TAP_WIN_IOCTL_GET_MAC
     * @brief 获取TAP设备MAC地址的IOCTL命令
     */
    #define TAP_WIN_IOCTL_GET_MAC               CTL_CODE(FILE_DEVICE_UNKNOWN, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)

    /**
     * @def TAP_WIN_IOCTL_GET_VERSION
     * @brief 获取TAP驱动版本的IOCTL命令
     */
    #define TAP_WIN_IOCTL_GET_VERSION           CTL_CODE(FILE_DEVICE_UNKNOWN, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)

    /**
     * @def TAP_WIN_IOCTL_GET_MTU
     * @brief 获取MTU的IOCTL命令
     */
    #define TAP_WIN_IOCTL_GET_MTU               CTL_CODE(FILE_DEVICE_UNKNOWN, 3, METHOD_BUFFERED, FILE_ANY_ACCESS)

    /**
     * @def TAP_WIN_IOCTL_GET_INFO
     * @brief 获取设备信息的IOCTL命令
     */
    #define TAP_WIN_IOCTL_GET_INFO              CTL_CODE(FILE_DEVICE_UNKNOWN, 4, METHOD_BUFFERED, FILE_ANY_ACCESS)

    /**
     * @def TAP_WIN_IOCTL_CONFIG_POINT_TO_POINT
     * @brief 配置点对点模式的IOCTL命令
     */
    #define TAP_WIN_IOCTL_CONFIG_POINT_TO_POINT CTL_CODE(FILE_DEVICE_UNKNOWN, 5, METHOD_BUFFERED, FILE_ANY_ACCESS)

    /**
     * @def TAP_WIN_IOCTL_SET_MEDIA_STATUS
     * @brief 设置媒体状态（连接/断开）的IOCTL命令
     */
    #define TAP_WIN_IOCTL_SET_MEDIA_STATUS      CTL_CODE(FILE_DEVICE_UNKNOWN, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)

    /**
     * @def TAP_WIN_IOCTL_CONFIG_DHCP_MASQ
     * @brief 配置DHCP伪装的IOCTL命令
     */
    #define TAP_WIN_IOCTL_CONFIG_DHCP_MASQ      CTL_CODE(FILE_DEVICE_UNKNOWN, 7, METHOD_BUFFERED, FILE_ANY_ACCESS)

    /**
     * @def TAP_WIN_IOCTL_GET_LOG_LINE
     * @brief 获取日志行的IOCTL命令
     */
    #define TAP_WIN_IOCTL_GET_LOG_LINE          CTL_CODE(FILE_DEVICE_UNKNOWN, 8, METHOD_BUFFERED, FILE_ANY_ACCESS)

    /**
     * @def TAP_WIN_IOCTL_CONFIG_DHCP_SET_OPT
     * @brief 设置DHCP选项的IOCTL命令
     */
    #define TAP_WIN_IOCTL_CONFIG_DHCP_SET_OPT   CTL_CODE(FILE_DEVICE_UNKNOWN, 9, METHOD_BUFFERED, FILE_ANY_ACCESS)

    /**
     * @def TAP_WIN_IOCTL_CONFIG_TUN
     * @brief 配置TUN模式的IOCTL命令
     */
    #define TAP_WIN_IOCTL_CONFIG_TUN            CTL_CODE(FILE_DEVICE_UNKNOWN, 10, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

/**
 * @class WindowsPlatform
 * @brief Windows平台实现类
 *
 * @details 提供Windows平台的VPN功能实现
 * - TUN设备管理：通过TAP-Windows驱动创建虚拟网卡
 * - 路由管理：使用Windows路由表API
 * - 系统代理：通过注册表配置IE代理设置
 * - 系统功能：通知、设备ID、开机自启动等
 *
 * Windows VPN架构：
 * ```
 * JinGo App (Qt/C++)
 *   ↓ Windows API
 * TAP-Windows Adapter
 *   ↓ NDIS Driver
 * Windows Network Stack
 * ```
 *
 * 主要特点：
 * - TAP驱动：使用TAP-Windows虚拟网卡驱动
 * - IOCTL控制：通过DeviceIoControl配置设备
 * - 注册表访问：管理系统代理和自启动
 * - 管理员权限：某些操作需要提升权限
 *
 * 依赖组件：
 * - TAP-Windows驱动：需要预先安装
 * - 版本支持：Windows 7及以上
 * - 权限要求：某些功能需要管理员权限
 *
 * @note
 * - 仅在Windows平台编译和使用
 * - 需要安装TAP-Windows驱动（OpenVPN提供）
 * - 路由和代理操作需要管理员权限
 *
 * @example
 * @code
 * WindowsPlatform* platform = new WindowsPlatform();
 *
 * // 检查VPN权限（管理员权限）
 * if (platform->hasVPNPermission()) {
 *     // 创建TUN设备
 *     if (platform->createTunDevice("JinGo")) {
 *         // 配置设备
 *         TunDeviceConfig config("172.19.0.1", "255.255.255.0", 1500);
 *         platform->configureTunDevice(config);
 *     }
 * }
 * @endcode
 */
class WindowsPlatform : public PlatformInterface
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     *
     * @details 初始化Windows平台实现
     * - 初始化成员变量
     * - 查找TAP设备
     */
    explicit WindowsPlatform(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     *
     * @details 清理资源
     * - 关闭TUN设备句柄
     * - 清理路由配置
     */
    ~WindowsPlatform() override;

    // ========================================================================
    // TUN 接口实现
    // ========================================================================

    /**
     * @brief 创建TUN设备
     * @param deviceName 设备名称或GUID
     * @return bool 成功返回true，失败返回false
     *
     * @details 打开TAP-Windows虚拟网卡
     * - 通过设备路径打开设备
     * - 设置设备为连接状态
     * - 配置点对点模式
     *
     * @note 需要管理员权限
     *
     * @see configureTunDevice, closeTunDevice
     */
    bool createTunDevice(const QString& deviceName) override;

    /**
     * @brief 关闭TUN设备
     * @return bool 成功返回true，失败返回false
     *
     * @details 关闭TAP设备句柄
     * - 设置设备为断开状态
     * - 关闭设备句柄
     * - 清理路由
     *
     * @see createTunDevice
     */
    bool closeTunDevice() override;

    /**
     * @brief 配置TUN设备
     * @param config TUN设备配置
     * @return bool 成功返回true，失败返回false
     *
     * @details 配置TAP设备的网络参数
     * - 使用netsh命令配置IP地址
     * - 设置MTU
     * - 配置子网掩码
     *
     * @note 需要管理员权限
     *
     * @see TunDeviceConfig
     */
    bool configureTunDevice(const TunDeviceConfig& config) override;

    /**
     * @brief 从TUN设备读取数据包
     * @return QByteArray 数据包内容，失败返回空数组
     *
     * @details 从TAP设备读取以太网帧
     * - 使用ReadFile阻塞读取
     * - 剥离以太网头（14字节）得到IP数据包
     * - 自动更新接收字节统计
     *
     * @note 应在独立线程中调用以避免阻塞UI
     */
    QByteArray readPacket() override;

    /**
     * @brief 向TUN设备写入数据包
     * @param packet 数据包内容（IP数据包）
     * @return bool 成功返回true，失败返回false
     *
     * @details 向TAP设备写入以太网帧
     * - 添加以太网头（14字节）
     * - 使用WriteFile写入
     * - 自动更新发送字节统计
     */
    bool writePacket(const QByteArray& packet) override;

    /**
     * @brief 添加路由
     * @param route 路由配置
     * @return bool 成功返回true，失败返回false
     *
     * @details 向Windows路由表添加路由
     * - 使用route add命令
     * - 需要管理员权限
     *
     * @note 需要管理员权限
     */
    bool addRoute(const RouteConfig& route) override;

    /**
     * @brief 删除路由
     * @param route 路由配置
     * @return bool 成功返回true，失败返回false
     *
     * @details 从Windows路由表删除路由
     * - 使用route delete命令
     * - 需要管理员权限
     *
     * @note 需要管理员权限
     */
    bool deleteRoute(const RouteConfig& route) override;

    /**
     * @brief 获取TUN设备文件描述符
     * @return int 始终返回-1（Windows不使用文件描述符）
     *
     * @details Windows使用HANDLE而非文件描述符
     */
    int getTunFileDescriptor() override;

    /**
     * @brief 检查TUN设备是否已创建
     * @return bool 已创建返回true，否则返回false
     */
    bool isTunDeviceCreated() const override;

    /**
     * @brief 获取TUN设备名称
     * @return QString 设备名称或GUID
     */
    QString getTunDeviceName() const override;

    // ========================================================================
    // 统计信息
    // ========================================================================

    /**
     * @brief 获取接收字节数
     * @return quint64 接收的字节数
     */
    quint64 getBytesReceived() const override;

    /**
     * @brief 获取发送字节数
     * @return quint64 发送的字节数
     */
    quint64 getBytesSent() const override;

    /**
     * @brief 重置统计信息
     */
    void resetStatistics() override;

    // ========================================================================
    // VPN 权限
    // ========================================================================

    /**
     * @brief 请求VPN权限（管理员权限）
     * @return bool 成功返回true，失败返回false
     *
     * @details Windows上的VPN权限即管理员权限
     * - 显示UAC提升权限对话框
     * - 用户授权后以管理员权限重启应用
     *
     * @note 此操作会重启应用
     */
    bool requestVPNPermission() override;

    /**
     * @brief 检查是否有VPN权限
     * @return bool 有权限返回true，无权限返回false
     *
     * @details 检查应用是否以管理员权限运行
     */
    bool hasVPNPermission() override;

    // ========================================================================
    // 系统功能
    // ========================================================================

    /**
     * @brief 设置系统代理
     * @param host 代理服务器地址
     * @param port 代理服务器端口
     * @return bool 成功返回true，失败返回false
     *
     * @details 配置Windows系统代理
     * - 修改注册表Internet Settings
     * - 影响IE和其他使用系统代理的应用
     * - 广播WM_SETTINGCHANGE消息通知其他应用
     */
    bool setupSystemProxy(const QString& host, int port) override;

    /**
     * @brief 清除系统代理
     * @return bool 成功返回true，失败返回false
     *
     * @details 清除Windows系统代理配置
     * - 修改注册表禁用代理
     * - 广播WM_SETTINGCHANGE消息
     */
    bool clearSystemProxy() override;

    /**
     * @brief 设置开机自启动
     * @param enable true启用，false禁用
     * @return bool 成功返回true，失败返回false
     *
     * @details 配置Windows开机自启动
     * - 修改注册表Run键
     * - 路径：HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run
     */
    bool setAutoStart(bool enable) override;

    /**
     * @brief 检查是否已启用开机自启动
     * @return bool 已启用返回true，未启用返回false
     *
     * @details 检查注册表Run键中是否存在应用项
     */
    bool isAutoStartEnabled() override;

    /**
     * @brief 显示系统通知
     * @param title 通知标题
     * @param message 通知内容
     *
     * @details 显示Windows 10/11操作中心通知
     * - Windows 10+：使用Toast通知
     * - Windows 7/8：使用托盘气泡通知
     */
    void showNotification(const QString& title, const QString& message) override;

    /**
     * @brief 获取设备ID
     * @return QString 设备唯一标识符
     *
     * @details 返回Windows机器GUID
     * - 从注册表读取MachineGuid
     * - 路径：HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Cryptography
     */
    QString getDeviceId() override;

    /**
     * @brief 获取平台名称
     * @return QString 平台名称（"Windows"）
     */
    QString getPlatformName() override;

    // ========================================================================
    // Windows 特有方法
    // ========================================================================

    /**
     * @brief 获取Windows版本
     * @return QString Windows版本（例如："10", "11"）
     *
     * @details 返回Windows操作系统版本
     * - 格式：主版本号或名称
     * - 例如：Windows 10返回"10"
     */
    QString osVersion() const;

private:
#ifdef Q_OS_WIN
    // ========================================================================
    // 成员变量（Windows）
    // ========================================================================

    // TUN 设备相关
    HANDLE m_tunHandle;          ///< TAP设备句柄
    QString m_deviceName;        ///< 设备名称或GUID
    bool m_tunCreated;           ///< TUN设备是否已创建

    // 统计信息
    quint64 m_bytesReceived;     ///< 接收字节数
    quint64 m_bytesSent;         ///< 发送字节数

    // ========================================================================
    // 私有方法（Windows）
    // ========================================================================

    /**
     * @brief 查找TAP设备
     * @return QString TAP设备GUID，失败返回空字符串
     *
     * @details 从注册表查找可用的TAP-Windows设备
     * - 枚举网络适配器
     * - 查找TAP驱动的设备
     */
    QString findTapDevice();

    /**
     * @brief 获取TAP适配器名称
     * @return QString 适配器友好名称
     *
     * @details 从注册表获取网络适配器的显示名称
     */
    QString getTapAdapterName();

    /**
     * @brief 获取机器GUID
     * @return QString 机器唯一标识符
     *
     * @details 从注册表读取MachineGuid
     */
    QString getMachineGuid();

    /**
     * @brief 通知系统代理设置已更改
     * @return bool 成功返回true
     *
     * @details 广播WM_SETTINGCHANGE消息
     * - 通知所有应用代理设置已更改
     */
    bool notifyProxyChange();
#else
    // ========================================================================
    // 成员变量（非Windows平台的占位符）
    // ========================================================================

    void* m_tunHandle;           ///< 占位符
    QString m_deviceName;        ///< 占位符
    bool m_tunCreated;           ///< 占位符
    quint64 m_bytesReceived;     ///< 占位符
    quint64 m_bytesSent;         ///< 占位符
#endif
};

#endif // WINDOWSPLATFORM_H
