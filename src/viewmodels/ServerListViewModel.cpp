/**
 * @file ServerListViewModel.cpp
 * @brief 服务器列表视图模型实现文件
 * @details 实现服务器列表的管理、筛选、排序和连接功能
 * @author JinGo VPN Team
 * @date 2025
 */

#include "ServerListViewModel.h"
#include "panel/SubscriptionManager.h"
#include "core/VPNManager.h"
#include "core/XrayCBridge.h"
#include "core/ConfigManager.h"
#include "core/BundleConfig.h"
#include "core/Logger.h"
#include "utils/CountryUtils.h"
#include <QTimer>
#include <QTcpSocket>
#include <QElapsedTimer>
#include <QtConcurrent>
#include <QPointer>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <memory>  // for std::shared_ptr
#include <QNetworkRequest>
#include <algorithm>
#include <functional>
#include <cstdio>

// macOS/iOS: 延时测试桥接函数声明（实现在 VPNManagerHelper.mm）
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
extern void testServerLatencyViaExtension(const QString& address, int timeout, std::function<void(int)> callback);
#endif

// Android: 使用被保护的 socket 绕过 VPN 测试真正的延时
#ifdef Q_OS_ANDROID
extern "C" int Android_ProtectedTcpPing(const char* host, int port, int timeout_ms);
#endif

/**
 * @brief 构造函数
 * @param parent 父对象
 *
 * @details 初始化视图模型：
 * - 设置成员变量初始值
 * - 获取订阅管理器和VPN管理器的单例引用
 * - 从数据库加载已有的服务器列表
 *
 * @note 不自动连接订阅更新信号，避免死循环
 * @see loadServersFromManager()
 */
ServerListViewModel::ServerListViewModel(QObject* parent)
    : QObject(parent)
    , m_selectedServer(nullptr)  // QPointer会自动初始化为nullptr
    , m_isLoading(false)
    , m_isUpdating(false)
    , m_isBatchTesting(false)
    , m_totalTestCount(0)
    , m_completedTestCount(0)
    , m_subscriptionManager(&SubscriptionManager::instance())
    , m_vpnManager(&VPNManager::instance())
{
    // 初始加载 - 从本地数据库加载已有的服务器
    loadServersFromManager();

    // 加载之前保存的测速结果
    loadSpeedTestResults();

    // ⭐ 监听订阅更新信号（增量更新模式）
    // 每次订阅更新后立即刷新，确保 UI 显示最新服务器
    if (m_subscriptionManager) {
        connect(m_subscriptionManager, &SubscriptionManager::subscriptionUpdated,
                this, [this](Subscription* subscription) {
            Q_UNUSED(subscription);
            LOG_DEBUG("Subscription updated, reloading servers from manager");
            loadServersFromManager();
        }, Qt::QueuedConnection);
    }

    // 连接VPNManager的状态变化信号，以便更新UI
    connect(m_vpnManager, &VPNManager::stateChanged,
            this, &ServerListViewModel::isConnectedChanged);
    connect(m_vpnManager, &VPNManager::stateChanged,
            this, &ServerListViewModel::connectionStateTextChanged);
    connect(m_vpnManager, &VPNManager::stateMessageChanged,
            this, &ServerListViewModel::connectionStateTextChanged);
}

/**
 * @brief 设置筛选文本
 * @param text 新的筛选文本
 *
 * @details 当筛选文本改变时：
 * - 更新m_filterText成员变量
 * - 发出filterTextChanged信号
 * - 重新应用筛选条件
 */
void ServerListViewModel::setFilterText(const QString& text)
{
    if (m_filterText != text) {
        m_filterText = text;
        emit filterTextChanged();
        applyFilter();
    }
}

/**
 * @brief 设置加载状态
 * @param loading 是否正在加载
 *
 * @details 当加载状态改变时发出isLoadingChanged信号
 */
void ServerListViewModel::setIsLoading(bool loading)
{
    if (m_isLoading != loading) {
        m_isLoading = loading;
        emit isLoadingChanged();
    }
}

/**
 * @brief 刷新服务器列表
 *
 * @details 刷新流程：
 * 1. 设置加载状态为true
 * 2. 使用QTimer异步触发订阅更新，避免阻塞UI线程
 * 3. 监听batchUpdateCompleted信号，更新完成后重新加载服务器列表
 *
 * @see loadServersFromManager()
 */
void ServerListViewModel::refreshServers()
{
    LOG_INFO("Manual refresh triggered - reloading from manager");

    setIsLoading(true);

    // 直接从 SubscriptionManager 重新加载服务器
    // 注意：信号连接已在构造函数中建立，这里无需重复连接
    if (m_subscriptionManager) {
        loadServersFromManager();
    } else {
        LOG_ERROR("SubscriptionManager is null");
        setIsLoading(false);
    }
}

/**
 * @brief 选择服务器
 * @param server 要选择的服务器指针
 *
 * @details 当选中的服务器改变时发出selectedServerChanged信号
 * @note 使用QPointer安全持有，如果server被删除，QPointer会自动置空
 */
void ServerListViewModel::selectServer(Server* server)
{
    if (m_selectedServer.data() != server) {
        m_selectedServer = server;  // QPointer会安全地持有指针
        emit selectedServerChanged();
    }
}

/**
 * @brief 连接到服务器
 * @param server 要连接的服务器指针
 *
 * @details 连接流程：
 * 1. 检查服务器指针是否有效
 * 2. 选中该服务器
 * 3. 调用VPN管理器的connecting方法建立连接
 */
void ServerListViewModel::connectToServer(Server* server)
{
    if (!server) {
        LOG_WARNING("Cannot connect: no server specified");
        return;
    }

    LOG_INFO(QString("Connecting to server: %1").arg(server->name()));

    // 【关键修复】如果VPN已连接，selectServer会自动处理断开和重连
    // 不需要再调用connecting，否则会触发两次连接尝试
    bool alreadyConnected = m_vpnManager->isConnected();

    selectServer(server);

    // 只有在VPN未连接时才主动调用connecting
    if (!alreadyConnected) {
        m_vpnManager->connecting(server);
    }
}

/**
 * @brief 断开当前VPN连接
 *
 * @details 调用VPNManager的disconnect方法断开当前VPN连接
 */
void ServerListViewModel::disconnect()
{
    LOG_INFO("Disconnecting from VPN");
    m_vpnManager->disconnect();
}

/**
 * @brief 快速连接到当前选中的服务器
 *
 * @details 如果有选中的服务器，则连接到该服务器；否则不执行任何操作
 * @note 使用QPointer::data()安全获取指针，如果对象已删除则返回nullptr
 */
void ServerListViewModel::connectToSelected()
{
    Server* server = m_selectedServer.data();
    if (!server) {
        LOG_WARNING("Cannot connect: no server selected or server object deleted");
        return;
    }

    connectToServer(server);
}

/**
 * @brief 切换连接状态
 *
 * @details 如果当前已连接或正在连接，则断开；否则连接到选中的服务器
 */
void ServerListViewModel::toggleConnection()
{
    if (isConnected() || m_vpnManager->isConnecting()) {
        disconnect();
    } else {
        connectToSelected();
    }
}

/**
 * @brief 获取是否已连接到VPN
 * @return true表示已连接，false表示未连接
 */
bool ServerListViewModel::isConnected() const
{
    return m_vpnManager && m_vpnManager->isConnected();
}

/**
 * @brief 获取连接状态文本描述
 * @return 状态文本（如"已连接"、"正在连接"、"未连接"等）
 */
QString ServerListViewModel::connectionStateText() const
{
    if (!m_vpnManager) {
        return tr("Unknown");
    }
    return m_vpnManager->stateMessage();
}

/**
 * @brief 测试服务器延迟
 * @param serverId 要测试的服务器ID
 *
 * @details 延迟测试实现：
 * - 在 TUN 模式下且 VPN 已连接时，通过 Extension 执行 TCP ping（macOS/iOS）
 * - 否则使用本地 TCP/HTTP ping 测试
 * - 测试完成后更新服务器的延迟值和可用性状态
 *
 * @note TUN 模式下网络流量通过 Extension 路由，需要在 Extension 内部测试
 */
void ServerListViewModel::testServerLatency(const QString& serverId)
{
    // Find server by ID
    Server* server = nullptr;
    for (const QPointer<Server>& serverPtr : m_allServers) {
        if (!serverPtr.isNull() && serverPtr->id() == serverId) {
            server = serverPtr.data();
            break;
        }
    }

    if (!server) {
        LOG_WARNING(QString("[ServerListViewModel] Server not found for ID: %1").arg(serverId));
        return;
    }

    // 使用QPointer安全地持有Server指针，防止对象被删除后访问
    QPointer<Server> serverPtr(server);

    // 标记为测试中
    server->setIsTestingSpeed(true);

    LOG_INFO(QString("[testServerLatency] Testing server: %1 (%2:%3)")
                 .arg(server->name())
                 .arg(server->address())
                 .arg(server->port()));

    // 获取测速方法配置
    ConfigManager& configManager = ConfigManager::instance();
    int timeout = configManager.testTimeout() * 1000; // 转换为毫秒

    // 获取测试参数
    QString address = server->address();
    int port = server->port();
    QString testAddress = QString("%1:%2").arg(address).arg(port);

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    // macOS/iOS: VPN 已连接时，需要通过 Extension 执行延时测试
    // - TUN 模式：主进程网络通过 VPN 隧道
    // 注意：系统代理模式不影响主进程
#if defined(Q_OS_IOS)
    bool useExtension = m_vpnManager && m_vpnManager->isConnected();
#else
    // macOS: 只有 TUN 模式需要通过 Extension 测试
    bool useExtension = m_vpnManager && m_vpnManager->isTunMode() && m_vpnManager->isConnected();
#endif
    if (useExtension) {
        LOG_INFO(QString("[testServerLatency] VPN connected, using Extension for test: %1").arg(testAddress));

        // 使用桥接函数调用 NetworkExtensionManager
        testServerLatencyViaExtension(testAddress, timeout, [serverPtr, this](int latency) {
            // 回调已经在主线程执行
            if (!serverPtr) {
                LOG_WARNING("Server object deleted during latency test");
                return;
            }

            if (latency > 0) {
                LOG_INFO(QString("Server %1 latency (via Extension): %2 ms")
                             .arg(serverPtr->name()).arg(latency));
                serverPtr->setLatency(latency);
                serverPtr->setIsAvailable(true);
            } else {
                LOG_WARNING(QString("Failed to test server %1 via Extension: latency=%2")
                                .arg(serverPtr->name()).arg(latency));
                serverPtr->setLatency(0);
                serverPtr->setIsAvailable(false);
            }

            serverPtr->setIsTestingSpeed(false);
            serverPtr->updateLastTested();
            emit serverTestCompleted(serverPtr.data());

            // 如果是批量测试，更新进度
            handleBatchTestProgress();
        });
        return;
    }
#endif

    // 非 TUN 模式或 VPN 未连接：使用本地 SuperRay 测试
    ConfigManager::LatencyTestMethod testMethod = configManager.latencyTestMethod();

    QString testMethodName = (testMethod == ConfigManager::TCPTest) ? "TCP" : "HTTP";
    LOG_INFO(QString("[testServerLatency] Test method: %1, timeout: %2ms")
                 .arg(testMethodName)
                 .arg(timeout));

    // 在后台线程执行测试
    // 使用 QPointer 安全地捕获 this，避免对象销毁后访问无效指针
    QPointer<ServerListViewModel> safeThis = this;
    auto future = QtConcurrent::run([serverPtr, testMethod, address, timeout, safeThis]() {
        QString serverName = serverPtr ? serverPtr->name() : "NULL";
        int latency = -1;

        // 检查VPN状态
        bool vpnConnected = safeThis && safeThis->m_vpnManager && safeThis->m_vpnManager->isConnected();

        if (testMethod == ConfigManager::TCPTest) {
            // ========== TCP 测试 ==========
            // TCP 连接到服务器测试延迟（需要地址和端口）
            int port = serverPtr ? serverPtr->port() : 0;
            LOG_INFO(QString("[TCP Test] Testing server %1 (%2:%3)").arg(serverName).arg(address).arg(port));

            // 验证地址有效性
            if (address.isEmpty() || port <= 0) {
                LOG_WARNING(QString("[TCP Test] Invalid address/port for server %1").arg(serverName));
                latency = -1;
            } else {
                QByteArray addressData = address.toUtf8();
#ifdef Q_OS_ANDROID
                // Android: 根据 VPN 连接状态选择测试方法
                if (vpnConnected) {
                    // VPN 已连接：使用被保护的 socket 绕过 VPN 测试真正的延时
                    LOG_INFO(QString("[TCP Test] VPN connected, using Android_ProtectedTcpPing"));
                    latency = Android_ProtectedTcpPing(addressData.constData(), port, timeout);
                } else {
                    // VPN 未连接：直接使用普通 TCP Ping
                    LOG_INFO(QString("[TCP Test] VPN not connected, using Xray_TCPPing"));
                    latency = Xray_TCPPing(addressData.constData(), port, timeout);
                }
#else
                // 其他平台：使用 Xray_TCPPing
                latency = Xray_TCPPing(addressData.constData(), port, timeout);
#endif
            }

            LOG_INFO(QString("[TCP Test] Server %1 result: %2 ms").arg(serverName).arg(latency));
        } else {
            // ========== HTTP 测试 (使用 SuperRay_HTTPPing) ==========
            // 服务器列表测试：总是直接测试目标服务器，不通过当前VPN代理
            // 这样可以准确测量每个服务器到互联网的延时
            QString testUrl = BundleConfig::instance().latencyTestUrl();
            QString proxyAddr = "";  // 直连测试，不使用代理

            LOG_INFO(QString("[HTTP Test] Testing %1 direct (no proxy) for server %2").arg(testUrl).arg(serverName));

            QByteArray urlData = testUrl.toUtf8();
            QByteArray proxyData = proxyAddr.toUtf8();
            latency = Xray_HTTPPing(urlData.constData(), proxyData.constData(), timeout);

            LOG_INFO(QString("[HTTP Test] Server %1 result: %2 ms").arg(serverName).arg(latency));
        }

        // 在主线程更新UI - 检查 ViewModel 是否仍然有效
        if (!safeThis) {
            LOG_WARNING("ServerListViewModel was destroyed during latency test");
            return;
        }
        QMetaObject::invokeMethod(safeThis.data(), [serverPtr, latency, safeThis]() {
            if (!safeThis) return;
            if (!serverPtr) {
                LOG_WARNING("Server object deleted during latency test");
                return;
            }

            if (latency > 0) {
                LOG_INFO(QString("Server %1 latency: %2 ms").arg(serverPtr->name()).arg(latency));
                serverPtr->setLatency(latency);
                serverPtr->setIsAvailable(true);
            } else {
                LOG_WARNING(QString("Failed to test server %1: ping returned %2")
                                .arg(serverPtr->name())
                                .arg(latency));
                serverPtr->setLatency(0);
                serverPtr->setIsAvailable(false);
            }

            serverPtr->setIsTestingSpeed(false);
            serverPtr->updateLastTested();
            emit safeThis->serverTestCompleted(serverPtr.data());

            // 如果是批量测试，更新进度
            safeThis->handleBatchTestProgress();

        }, Qt::QueuedConnection);
    });
    Q_UNUSED(future);
}

/**
 * @brief 测试服务器吞吐量（下载速度）
 * @param serverId 要测试的服务器ID
 *
 * @details 吞吐量测试实现：
 * - 如果当前 VPN 已连接到此服务器，使用现有代理测试
 * - 如果未连接，需要先连接到该服务器才能测试吞吐量
 * - 通过下载测试文件来测量实际下载速度
 * - iOS/macOS/Android 使用 QNetworkAccessManager（流量自动通过 TUN 隧道）
 * - Windows/Linux 使用 Xray_SpeedTest（通过 SOCKS5 代理）
 */
void ServerListViewModel::testServerThroughput(const QString& serverId)
{
    // Find server by ID
    Server* server = nullptr;
    for (const QPointer<Server>& serverPtr : m_allServers) {
        if (!serverPtr.isNull() && serverPtr->id() == serverId) {
            server = serverPtr.data();
            break;
        }
    }

    if (!server) {
        LOG_WARNING(QString("[testServerThroughput] Server not found for ID: %1").arg(serverId));
        return;
    }

    // 使用QPointer安全地持有Server指针
    QPointer<Server> serverPtr(server);

    // 标记为测试中
    server->setIsTestingSpeed(true);

    LOG_INFO(QString("[testServerThroughput] Testing server throughput: %1 (%2:%3)")
                 .arg(server->name())
                 .arg(server->address())
                 .arg(server->port()));

    // 检查 VPN 是否已连接（QML 端会先连接再调用此函数）
    if (!m_vpnManager || !m_vpnManager->isConnected()) {
        LOG_WARNING(QString("[testServerThroughput] VPN is not connected, cannot test throughput for server %1")
                        .arg(server->name()));
        server->setIsTestingSpeed(false);

        // 保存错误结果
        QVariantMap result;
        result["speed"] = "N/A";
        result["error"] = tr("VPN not connected");
        setSpeedTestResult(serverId, result);
        emit serverThroughputTestCompleted(server, -1);
        return;
    }


    // 获取速度测试 URL（使用 Cloudflare 或配置的 URL）
    QString speedTestBaseUrl = BundleConfig::instance().speedTestBaseUrl();
    // 默认下载 10MB 的文件进行测试
    QString downloadURL = speedTestBaseUrl + "10000000";

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    // ========== Windows/Linux: 使用 Xray_SpeedTest + SOCKS5 代理 ==========
    ConfigManager& configManager = ConfigManager::instance();
    int socksPort = configManager.localSocksPort();
    QString proxyAddr = QString("127.0.0.1:%1").arg(socksPort);

    LOG_INFO(QString("[testServerThroughput] Using Xray_SpeedTest with proxy: %1, URL: %2")
                 .arg(proxyAddr).arg(downloadURL));

    // 在后台线程执行测试
    QPointer<ServerListViewModel> safeThis = this;
    auto future = QtConcurrent::run([serverPtr, proxyAddr, downloadURL, safeThis]() {
        QString serverName = serverPtr ? serverPtr->name() : "NULL";
        QString serverId = serverPtr ? serverPtr->id() : "";
        double speedMbps = -1;

        LOG_INFO(QString("[testServerThroughput] Starting speed test for server %1").arg(serverName));

        // 调用 Xray_SpeedTest
        char resultBuffer[4096];
        QByteArray urlData = downloadURL.toUtf8();
        QByteArray proxyData = proxyAddr.toUtf8();

        int ret = Xray_SpeedTest(urlData.constData(), proxyData.constData(), 10, resultBuffer, sizeof(resultBuffer));

        if (ret == 0) {
            // 解析结果
            QString resultStr = QString::fromUtf8(resultBuffer);
            LOG_INFO(QString("[testServerThroughput] Speed test result: %1").arg(resultStr.left(200)));

            QJsonDocument doc = QJsonDocument::fromJson(resultStr.toUtf8());
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                if (obj.value("success").toBool()) {
                    QJsonObject data = obj.value("data").toObject();
                    speedMbps = data.value("speed_mbps").toDouble(-1);
                    if (speedMbps < 0) {
                        // 尝试其他字段名
                        speedMbps = data.value("download_mbps").toDouble(-1);
                    }
                }
            }
        } else {
            LOG_WARNING(QString("[testServerThroughput] Speed test failed for server %1").arg(serverName));
        }

        LOG_INFO(QString("[testServerThroughput] Server %1 throughput: %2 Mbps").arg(serverName).arg(speedMbps));

        // 在主线程更新UI
        if (!safeThis) {
            LOG_WARNING("ServerListViewModel was destroyed during throughput test");
            return;
        }
        QMetaObject::invokeMethod(safeThis.data(), [serverPtr, speedMbps, serverId, safeThis]() {
            if (!safeThis) return;
            if (!serverPtr) {
                LOG_WARNING("Server object deleted during throughput test");
                return;
            }

            serverPtr->setIsTestingSpeed(false);

            // 保存结果
            QVariantMap result;
            if (speedMbps > 0) {
                result["speed"] = QString::number(speedMbps, 'f', 2) + " Mbps";
                result["speedValue"] = speedMbps;
                LOG_INFO(QString("Server %1 throughput: %2 Mbps").arg(serverPtr->name()).arg(speedMbps));
            } else {
                result["speed"] = "Failed";
                result["error"] = "Speed test failed";
                LOG_WARNING(QString("Failed to test throughput for server %1").arg(serverPtr->name()));
            }

            safeThis->setSpeedTestResult(serverId, result);
            emit safeThis->serverThroughputTestCompleted(serverPtr.data(), speedMbps);

        }, Qt::QueuedConnection);
    });
    Q_UNUSED(future);
#else
    // ========== iOS/macOS/Android: 使用 QNetworkAccessManager ==========
    // TUN 模式下，Qt 的网络请求会自动通过系统 VPN 路由
    // Go 运行时（SuperRay）的 HTTP 请求可能不会通过 TUN 隧道
    LOG_INFO(QString("[testServerThroughput] Using QNetworkAccessManager (TUN mode), URL: %1").arg(downloadURL));

    QPointer<ServerListViewModel> safeThis = this;

    // 创建网络管理器（在主线程）
    QNetworkAccessManager* networkManager = new QNetworkAccessManager(this);

    // 使用 shared_ptr 避免内存泄漏
    auto timer = std::make_shared<QElapsedTimer>();
    timer->start();

    // 创建请求
    QUrl speedTestUrl(downloadURL);
    QNetworkRequest request(speedTestUrl);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    // 设置超时（Qt 5.15+ 支持）
    request.setTransferTimeout(30000);  // 30 秒超时

    // 发起 GET 请求
    QNetworkReply* reply = networkManager->get(request);

    // 使用 shared_ptr 跟踪下载的字节数，避免内存泄漏
    auto totalBytesReceived = std::make_shared<qint64>(0);

    // 连接进度信号（实时统计接收的字节数）
    connect(reply, &QNetworkReply::downloadProgress, this, [totalBytesReceived](qint64 bytesReceived, qint64 bytesTotal) {
        Q_UNUSED(bytesTotal);
        *totalBytesReceived = bytesReceived;
    });

    // 保存当前测试的服务器ID（lambda捕获）
    QString testServerId = serverId;
    QString testServerName = server->name();

    // 连接完成信号（明确捕获需要的变量，避免隐式捕获）
    connect(reply, &QNetworkReply::finished, this, [reply, networkManager, timer, totalBytesReceived, serverPtr, safeThis, testServerId, testServerName, this]() {
        double speedMbps = -1;
        qint64 elapsed = timer->elapsed();

        if (reply->error() == QNetworkReply::NoError) {
            qint64 bytesReceived = *totalBytesReceived;
            if (bytesReceived > 0 && elapsed > 0) {
                double elapsedSec = elapsed / 1000.0;
                speedMbps = (bytesReceived * 8.0) / (elapsedSec * 1000000.0);
            }
        } else {
            LOG_WARNING(QString("Speed test download failed: %1").arg(reply->errorString()));
        }

        // 清理 - shared_ptr 会自动释放内存
        reply->deleteLater();
        networkManager->deleteLater();

        if (!safeThis || !serverPtr) return;

        serverPtr->setIsTestingSpeed(false);

        QVariantMap result;
        if (speedMbps > 0) {
            result["speed"] = QString::number(speedMbps, 'f', 2) + " Mbps";
            result["speedValue"] = speedMbps;
            LOG_INFO(QString("Server %1 throughput: %2 Mbps").arg(testServerName).arg(speedMbps, 0, 'f', 2));
        } else {
            result["speed"] = "Failed";
            result["error"] = "Speed test failed";
        }

        safeThis->setSpeedTestResult(testServerId, result);
        emit safeThis->serverThroughputTestCompleted(serverPtr.data(), speedMbps);
    });
#endif
}

/**
 * @brief 处理批量测试进度更新
 *
 * @details 检查并更新批量测试进度，当所有测试完成时执行排序
 */
void ServerListViewModel::handleBatchTestProgress()
{
    if (!m_isBatchTesting) {
        return;
    }

    m_completedTestCount++;
    m_testingProgressText = QString("Testing %1 of %2 servers...")
        .arg(m_completedTestCount)
        .arg(m_totalTestCount);
    emit testingProgressTextChanged();

    LOG_INFO(QString("Batch test progress: %1/%2").arg(m_completedTestCount).arg(m_totalTestCount));

    // 如果所有测试都完成了，执行排序
    if (m_completedTestCount >= m_totalTestCount) {
        LOG_INFO("所有服务器测试完成，开始排序...");

        // 按延迟排序
        std::sort(m_filteredServers.begin(), m_filteredServers.end(),
                  [](const QPointer<Server>& a, const QPointer<Server>& b) {
                      if (a.isNull()) return false;
                      if (b.isNull()) return true;

                      int latencyA = a->latency();
                      int latencyB = b->latency();

                      if (latencyA == 0 && latencyB == 0) return false;
                      if (latencyA == 0) return false;
                      if (latencyB == 0) return true;

                      return latencyA < latencyB;
                  });

        emit serversChanged();
        LOG_INFO("服务器列表已按延时排序");

        // 重置批量测试状态
        m_isBatchTesting = false;
        m_testingProgressText = QString("Completed! Tested %1 servers").arg(m_totalTestCount);
        emit isBatchTestingChanged();
        emit testingProgressTextChanged();
        emit allTestsCompleted();
    }
}

/**
 * @brief 批量测试所有服务器延迟
 *
 * @details 测试所有服务器的延迟，不进行排序
 */
void ServerListViewModel::testAllServersLatency()
{
    LOG_INFO("========== testAllServersLatency() CALLED ==========");
    LOG_INFO(QString("Filtered servers count: %1").arg(m_filteredServers.count()));

    if (m_isBatchTesting) {
        LOG_WARNING("Batch testing already in progress, skipping");
        return;
    }

    if (m_filteredServers.isEmpty()) {
        LOG_WARNING("No servers to test");
        return;
    }

    // 检查测试方法和VPN状态
    ConfigManager& configManager = ConfigManager::instance();
    ConfigManager::LatencyTestMethod testMethod = configManager.latencyTestMethod();
    bool vpnConnected = m_vpnManager && m_vpnManager->isConnected();

    // HTTP测试时，如果VPN已连接，需要先断开
    // 因为HTTP请求会通过VPN隧道，无法测试其他服务器的真实延时
    if (testMethod == ConfigManager::HTTPTest && vpnConnected) {
        LOG_INFO("[Batch Test] HTTP test mode, disconnecting VPN first...");
        m_vpnManager->disconnect();
        // 等待断开后再开始测试
        QTimer::singleShot(500, this, &ServerListViewModel::testAllServersLatency);
        return;
    }

    // 初始化批量测试状态
    m_isBatchTesting = true;
    m_totalTestCount = 0;
    m_completedTestCount = 0;
    emit isBatchTestingChanged();

    // 先统计需要测试的服务器数量
    for (const auto& server : m_filteredServers) {
        if (!server.isNull()) {
            m_totalTestCount++;
        }
    }

    LOG_INFO(QString("Starting batch test for %1 servers").arg(m_totalTestCount));

    // 更新进度文本
    m_testingProgressText = QString("Testing 0 of %1 servers...").arg(m_totalTestCount);
    emit testingProgressTextChanged();

    // 启动所有测试
    for (const auto& server : m_filteredServers) {
        if (!server.isNull()) {
            LOG_INFO(QString("Testing server: %1").arg(server->name()));
            testServerLatency(server->id());
        }
    }
}

/**
 * @brief 按延迟排序服务器列表
 *
 * @details 先测试所有服务器的延迟，然后按延迟升序排序
 * @note 添加空指针检查，确保排序安全
 */
void ServerListViewModel::sortByLatency()
{
    LOG_INFO("========== sortByLatency() CALLED ==========");
    LOG_INFO(QString("Filtered servers count: %1").arg(m_filteredServers.count()));

    // 检查是否有服务器需要测试延迟（latency < 0 表示未测试）
    QList<QPointer<Server>> serversNeedTest;
    for (const auto& server : m_filteredServers) {
        if (!server.isNull() && server->latency() < 0) {
            serversNeedTest.append(server);
        }
    }

    // 如果所有服务器都已有延迟数据，直接排序
    if (serversNeedTest.isEmpty()) {
        LOG_INFO("All servers have latency data, sorting directly");

        // 按延迟排序：有效延迟 < 超时(0) < 未测试(-1)
        std::sort(m_filteredServers.begin(), m_filteredServers.end(),
                  [](const QPointer<Server>& a, const QPointer<Server>& b) {
                      if (a.isNull()) return false;
                      if (b.isNull()) return true;

                      int latencyA = a->latency();
                      int latencyB = b->latency();

                      // 未测试(-1) 排最后
                      if (latencyA < 0) return false;
                      if (latencyB < 0) return true;

                      // 超时(0) 排在有效延迟之后
                      if (latencyA == 0) return false;
                      if (latencyB == 0) return true;

                      // 有效延迟按从小到大排序
                      return latencyA < latencyB;
                  });

        emit serversChanged();
        return;
    }

    // 有服务器需要测试，进入批量测试模式
    LOG_INFO(QString("Need to test %1 servers, starting batch test").arg(serversNeedTest.count()));

    // 初始化批量测试状态
    m_isBatchTesting = true;
    m_totalTestCount = static_cast<int>(serversNeedTest.count());
    m_completedTestCount = 0;
    emit isBatchTestingChanged();

    // 更新进度文本
    m_testingProgressText = QString("Testing 0 of %1 servers...").arg(m_totalTestCount);
    emit testingProgressTextChanged();

    // 只测试需要测试的服务器
    for (const auto& server : serversNeedTest) {
        if (!server.isNull()) {
            LOG_INFO(QString("Testing server: %1").arg(server->name()));
            testServerLatency(server->id());
        }
    }
}

/**
 * @brief 按名称排序服务器列表
 *
 * @details 使用std::sort对过滤后的服务器列表按名称字母顺序排序
 * @note 添加空指针检查，确保排序安全
 */
void ServerListViewModel::sortByName()
{
    std::sort(m_filteredServers.begin(), m_filteredServers.end(),
              [](const QPointer<Server>& a, const QPointer<Server>& b) {
                  // QPointer 空指针检查：空指针排在后面
                  if (a.isNull()) return false;
                  if (b.isNull()) return true;
                  return a->name() < b->name();
              });

    emit serversChanged();
}

/**
 * @brief 获取按距离排序的大洲列表
 * @return 按距离从近到远排序的大洲列表
 */
QStringList ServerListViewModel::getSortedContinents() const
{
    QString userCountryCode = ConfigManager::instance().userCountryCode();
    return CountryUtils::getSortedContinents(userCountryCode);
}

/**
 * @brief 手动添加服务器
 * @param jsonOrLink JSON配置字符串或分享链接
 * @return 成功返回服务器指针，失败返回nullptr
 *
 * @details 将添加请求委托给订阅管理器，添加成功后重新加载服务器列表
 */
Server* ServerListViewModel::addManualServer(const QString& jsonOrLink)
{
    Server* server = m_subscriptionManager->addServerManually(jsonOrLink);

    if (server) {
        loadServersFromManager();
    }

    return server;
}

/**
 * @brief 从数据库重新加载服务器列表（公开方法）
 * @details 供QML调用，不触发网络更新，只从本地SubscriptionManager的缓存中加载
 */
void ServerListViewModel::loadServersFromDatabase()
{
    LOG_INFO("loadServersFromDatabase: Reloading servers from local cache");
    // 直接调用内部的加载方法
    loadServersFromManager();
}

/**
 * @brief 从订阅管理器加载服务器
 *
 * @details 加载流程：
 * 1. 检查是否已在更新中（防重入）
 * 2. 设置加载状态
 * 3. 使用QTimer异步从订阅管理器获取所有服务器，避免死锁
 * 4. 应用当前的筛选条件
 * 5. 发出serversChanged信号
 *
 * @note 使用m_isUpdating标志防止重复调用
 */
void ServerListViewModel::loadServersFromManager()
{
    if (m_isUpdating) {
        LOG_DEBUG("Already updating, skipping loadServersFromManager");
        return;
    }

    m_isUpdating = true;
    setIsLoading(true);

    // 【核心修复】不要立即清空列表，而是先获取新列表，然后原子性替换
    // 这样可以避免QML在遍历过程中访问到已删除的对象
    //
    // 使用QTimer异步加载，避免在可能持有锁的情况下调用
    QTimer::singleShot(0, this, [this]() {
        // 第一步：从 SubscriptionManager 获取所有服务器（新列表）
        QList<QPointer<Server>> newServers;
        if (m_subscriptionManager) {
            // getAllServers() 返回列表副本，不是引用
            QList<Server*> rawServers = m_subscriptionManager->getAllServers();

            // 转换为 QPointer 并过滤空指针
            for (Server* server : rawServers) {
                if (server) {
                    newServers.append(QPointer<Server>(server));
                }
            }

            LOG_INFO(QString("Loaded %1 valid servers from SubscriptionManager").arg(newServers.count()));
        } else {
            LOG_ERROR("SubscriptionManager is null");
        }

        // 第二步：原子性地替换服务器列表（使用副本赋值）
        // Qt的QList赋值会创建副本，这样即使QML正在访问旧列表，也不会崩溃
        // 使用 QPointer 后，当 Server 被删除时，QPointer 会自动变为 null
        m_allServers = newServers;

        // 第三步：应用过滤并通知 QML
        applyFilter();

        setIsLoading(false);
        m_isUpdating = false;
    });
}

/**
 * @brief 应用筛选条件
 *
 * @details 筛选逻辑：
 * - 如果filterText为空，显示所有服务器
 * - 否则，筛选出名称、地址或位置包含filterText的服务器
 * - 筛选后发出serversChanged信号
 *
 * @note 筛选是大小写不敏感的
 */
void ServerListViewModel::applyFilter()
{
    m_filteredServers.clear();

    if (m_filterText.isEmpty()) {
        // 【安全检查】过滤空 QPointer（对象已被删除）
        for (const QPointer<Server>& serverPtr : m_allServers) {
            if (!serverPtr.isNull()) {
                m_filteredServers.append(serverPtr);
            }
        }
    } else {
        QString filter = m_filterText.toLower();
        for (const QPointer<Server>& serverPtr : m_allServers) {
            // 【安全检查】确保 QPointer 有效再访问其属性
            if (!serverPtr.isNull()) {
                Server* server = serverPtr.data();
                if (server && (server->name().toLower().contains(filter) ||
                    server->address().toLower().contains(filter) ||
                    server->location().toLower().contains(filter))) {
                    m_filteredServers.append(serverPtr);
                }
            }
        }
    }

    LOG_INFO(QString("Filter applied: %1 servers shown (total: %2)")
        .arg(m_filteredServers.count())
        .arg(m_allServers.count()));
    emit serversChanged();
}

/**
 * @brief 设置服务器的速度测试结果
 * @param serverId 服务器ID
 * @param result 测试结果 {ip, speed, asn, isp, country}
 */
void ServerListViewModel::setSpeedTestResult(const QString& serverId, const QVariantMap& result)
{
    if (serverId.isEmpty()) {
        LOG_WARNING("Empty server ID, not saving speed test result");
        return;
    }
    m_speedTestResults[serverId] = result;

    // 自动保存到本地文件
    saveSpeedTestResults();

    emit speedTestResultsChanged();
}

/**
 * @brief 获取服务器的速度测试结果
 * @param serverId 服务器ID
 * @return 测试结果，如果没有则返回空 QVariantMap
 */
QVariantMap ServerListViewModel::getSpeedTestResult(const QString& serverId) const
{
    if (m_speedTestResults.contains(serverId)) {
        return m_speedTestResults[serverId].toMap();
    }
    return QVariantMap();
}

/**
 * @brief 清除所有速度测试结果
 */
void ServerListViewModel::clearSpeedTestResults()
{
    m_speedTestResults.clear();
    saveSpeedTestResults();  // 同步清除本地存储
    emit speedTestResultsChanged();
}

/**
 * @brief 保存测速结果到本地文件
 */
void ServerListViewModel::saveSpeedTestResults()
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    QString filePath = dataDir + "/speed_test_results.json";

    QJsonObject rootObj;
    for (auto it = m_speedTestResults.constBegin(); it != m_speedTestResults.constEnd(); ++it) {
        QJsonObject resultObj;
        QVariantMap resultMap = it.value().toMap();
        resultObj["ip"] = resultMap.value("ip").toString();
        resultObj["ipInfo"] = resultMap.value("ipInfo").toString();
        resultObj["speed"] = resultMap.value("speed").toString();
        resultObj["timestamp"] = QDateTime::currentSecsSinceEpoch();
        rootObj[it.key()] = resultObj;
    }

    QJsonDocument doc(rootObj);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Compact));
        file.close();
        LOG_INFO(QString("Speed test results saved to %1 (%2 entries)")
            .arg(filePath).arg(m_speedTestResults.size()));
    } else {
        LOG_ERROR("Failed to save speed test results: " + file.errorString());
    }
}

/**
 * @brief 从本地文件加载测速结果
 */
void ServerListViewModel::loadSpeedTestResults()
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString filePath = dataDir + "/speed_test_results.json";

    QFile file(filePath);
    if (!file.exists()) {
        LOG_INFO("No saved speed test results found");
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR("Failed to open speed test results file: " + file.errorString());
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        LOG_ERROR("Failed to parse speed test results: " + parseError.errorString());
        return;
    }

    QJsonObject rootObj = doc.object();
    m_speedTestResults.clear();

    for (auto it = rootObj.constBegin(); it != rootObj.constEnd(); ++it) {
        QString serverId = it.key();
        QJsonObject resultObj = it.value().toObject();
        QVariantMap resultMap;
        resultMap["ip"] = resultObj.value("ip").toString();
        resultMap["ipInfo"] = resultObj.value("ipInfo").toString();
        resultMap["speed"] = resultObj.value("speed").toString();
        m_speedTestResults[serverId] = resultMap;
    }

    LOG_INFO(QString("Loaded %1 cached speed test results").arg(m_speedTestResults.size()));
    emit speedTestResultsChanged();
}
