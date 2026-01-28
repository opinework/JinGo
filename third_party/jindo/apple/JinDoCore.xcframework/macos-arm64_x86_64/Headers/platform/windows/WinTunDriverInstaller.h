/**
 * @file WinTunDriverInstaller.h
 * @brief WinTun驱动安装辅助工具
 * @details 提供驱动检查和安装功能
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
#include <string>

namespace JinGo {

/**
 * WinTun驱动安装器
 */
class WinTunDriverInstaller {
public:
    /**
     * 检查WinTun驱动是否已安装
     * @return true表示已安装
     */
    static bool isDriverInstalled();

    /**
     * 获取驱动版本
     * @return 版本号（高16位主版本，低16位次版本），0表示未安装
     */
    static DWORD getDriverVersion();

    /**
     * 检查wintun.dll是否存在
     * @return wintun.dll路径，如果不存在返回空字符串
     */
    static std::wstring findWintunDll();

    /**
     * 检查是否有管理员权限
     * @return true表示有管理员权限
     */
    static bool isRunningAsAdministrator();

    /**
     * 验证系统要求
     * @param errorMessage 错误信息（如果验证失败）
     * @return true表示满足要求
     */
    static bool checkSystemRequirements(std::string& errorMessage);
};

} // namespace JinGo

#endif // _WIN32
