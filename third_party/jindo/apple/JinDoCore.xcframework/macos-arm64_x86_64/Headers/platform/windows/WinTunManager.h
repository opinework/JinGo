/**
 * @file WinTunManager.h
 * @brief Windows wintun虚拟网卡管理器
 * @details 使用wintun驱动创建和管理虚拟网卡，实现TUN功能
 *
 * @author JinGo VPN Team
 * @date 2025
 * @copyright Copyright © 2025 JinGo Team. All rights reserved.
 */

#pragma once

#ifdef _WIN32

// 必须先包含 winsock2.h，再包含 windows.h
#include <winsock2.h>
#include <Windows.h>
#include <wintun.h>
#include <string>
#include <vector>
#include <memory>
#include <atomic>

namespace JinGo {

/**
 * Windows wintun管理器
 *
 * 负责：
 * - 加载wintun.dll
 * - 创建和管理虚拟网卡
 * - 配置IP地址、路由、DNS
 * - 读写数据包
 * - 收集统计信息
 */
class WinTunManager {
public:
    WinTunManager();
    ~WinTunManager();

    // 禁止拷贝
    WinTunManager(const WinTunManager&) = delete;
    WinTunManager& operator=(const WinTunManager&) = delete;

    /**
     * 初始化wintun
     * @param adapterName 适配器名称（如"JinGo VPN"）
     * @param tunnelType 隧道类型（如"JinGo"）
     * @return 成功返回true
     */
    bool initialize(const std::wstring& adapterName, const std::wstring& tunnelType);

    /**
     * 启动会话
     * @param capacity 环形缓冲区容量（默认0x400000 = 4MB）
     * @return 成功返回true
     */
    bool startSession(DWORD capacity = 0x400000);

    /**
     * 停止会话
     */
    void stopSession();

    /**
     * 关闭适配器
     */
    void shutdown();

    /**
     * 获取适配器句柄
     */
    WINTUN_ADAPTER_HANDLE getAdapter() const { return adapter_; }

    /**
     * 获取会话句柄
     */
    WINTUN_SESSION_HANDLE getSession() const { return session_; }

    /**
     * 获取适配器LUID
     */
    NET_LUID getLuid();

    /**
     * 获取适配器的Windows显示名称
     * @return 适配器显示名称（用于netsh等命令），失败返回空字符串
     */
    std::wstring getAdapterDisplayName();

    /**
     * 配置IP地址
     * @param ipAddress IP地址字符串（如"172.19.0.1"）
     * @param prefixLength 前缀长度（如24）
     * @return 成功返回true
     */
    bool setIPAddress(const std::string& ipAddress, UINT8 prefixLength);

    /**
     * 添加路由
     * @param destination 目标网络（如"0.0.0.0"表示默认路由）
     * @param prefixLength 前缀长度（0表示默认路由）
     * @param gateway 网关地址
     * @return 成功返回true
     */
    bool addRoute(const std::string& destination, UINT8 prefixLength, const std::string& gateway);

    /**
     * 删除路由
     */
    bool deleteRoute(const std::string& destination, UINT8 prefixLength);

    /**
     * 获取默认网关
     * @return 默认网关IP地址，失败返回空字符串
     */
    std::string getDefaultGateway();

    /**
     * 设置DNS服务器
     * @param dnsServers DNS服务器列表
     * @return 成功返回true
     */
    bool setDNS(const std::vector<std::string>& dnsServers);

    /**
     * 读取数据包
     * @param timeout 超时时间（毫秒），0表示不等待，INFINITE表示永久等待
     * @return 数据包指针，使用完后需调用releasePacket释放
     */
    BYTE* receivePacket(DWORD* size, DWORD timeout = INFINITE);

    /**
     * 释放接收的数据包
     */
    void releasePacket(const BYTE* packet);

    /**
     * 分配发送数据包缓冲区
     */
    BYTE* allocateSendPacket(DWORD size);

    /**
     * 发送数据包
     */
    void sendPacket(const BYTE* packet);

    /**
     * 获取等待事件句柄（用于异步I/O）
     */
    HANDLE getReadWaitEvent();

    /**
     * 检查是否正在运行
     */
    bool isRunning() const { return session_ != nullptr; }

    /**
     * 统计信息
     */
    struct Statistics {
        uint64_t bytesReceived;
        uint64_t bytesSent;
        uint64_t packetsReceived;
        uint64_t packetsSent;
        uint64_t errorsReceived;
        uint64_t errorsSent;
    };

    Statistics getStatistics() const;

private:
    // wintun函数指针
    HMODULE wintunDll_;
    WINTUN_CREATE_ADAPTER_FUNC* WintunCreateAdapter_;
    WINTUN_CLOSE_ADAPTER_FUNC* WintunCloseAdapter_;
    WINTUN_OPEN_ADAPTER_FUNC* WintunOpenAdapter_;
    WINTUN_GET_ADAPTER_LUID_FUNC* WintunGetAdapterLUID_;
    WINTUN_GET_RUNNING_DRIVER_VERSION_FUNC* WintunGetRunningDriverVersion_;
    WINTUN_DELETE_DRIVER_FUNC* WintunDeleteDriver_;
    WINTUN_SET_LOGGER_FUNC* WintunSetLogger_;
    WINTUN_START_SESSION_FUNC* WintunStartSession_;
    WINTUN_END_SESSION_FUNC* WintunEndSession_;
    WINTUN_GET_READ_WAIT_EVENT_FUNC* WintunGetReadWaitEvent_;
    WINTUN_RECEIVE_PACKET_FUNC* WintunReceivePacket_;
    WINTUN_RELEASE_RECEIVE_PACKET_FUNC* WintunReleaseReceivePacket_;
    WINTUN_ALLOCATE_SEND_PACKET_FUNC* WintunAllocateSendPacket_;
    WINTUN_SEND_PACKET_FUNC* WintunSendPacket_;

    // 适配器和会话
    WINTUN_ADAPTER_HANDLE adapter_;
    WINTUN_SESSION_HANDLE session_;
    std::wstring adapterName_;
    std::wstring tunnelType_;

    // 统计信息
    std::atomic<uint64_t> bytesReceived_;
    std::atomic<uint64_t> bytesSent_;
    std::atomic<uint64_t> packetsReceived_;
    std::atomic<uint64_t> packetsSent_;
    std::atomic<uint64_t> errorsReceived_;
    std::atomic<uint64_t> errorsSent_;

    // 辅助函数
    bool loadWintunDll();
    void unloadWintunDll();
    static void CALLBACK wintunLogger(WINTUN_LOGGER_LEVEL level, DWORD64 timestamp, const WCHAR* message);
};

} // namespace JinGo

#endif // _WIN32
