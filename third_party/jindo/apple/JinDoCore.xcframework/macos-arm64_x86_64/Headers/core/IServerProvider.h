/**
 * @file IServerProvider.h
 * @brief 服务器提供者接口
 * @details 定义获取服务器的抽象接口，用于解耦 VPNManager 对具体实现的依赖
 *
 * @author JinDo Core Team
 * @date 2025
 */

#ifndef ISERVERPROVIDER_H
#define ISERVERPROVIDER_H

#include <QList>
#include <QString>

class Server;

/**
 * @brief 服务器提供者接口
 *
 * 该接口定义了获取服务器信息的方法，VPNManager 通过此接口获取服务器，
 * 而不是直接依赖 SubscriptionManager，实现松耦合设计。
 */
class IServerProvider
{
public:
    virtual ~IServerProvider() = default;

    /**
     * @brief 获取所有可用服务器
     * @return 服务器列表
     */
    virtual QList<Server*> getAllServers() const = 0;

    /**
     * @brief 通过 ID 获取服务器
     * @param serverId 服务器 ID
     * @return 服务器指针，如果未找到返回 nullptr
     */
    virtual Server* getServerById(const QString& serverId) const = 0;
};

#endif // ISERVERPROVIDER_H
