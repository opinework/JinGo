# 面板扩展开发指南

本文档介绍如何为 JinGo 添加新的面板系统支持。

## 目录

1. [架构概述](#架构概述)
2. [接口定义](#接口定义)
3. [开发步骤](#开发步骤)
4. [代码示例](#代码示例)
5. [API 端点映射](#api-端点映射)
6. [数据格式转换](#数据格式转换)
7. [注册和使用](#注册和使用)
8. [测试建议](#测试建议)

---

## 架构概述

JinGo 采用可扩展的面板架构：

```
┌─────────────────────────────────────────────────────┐
│                    JinGo 应用层                      │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │
│  │ LoginView   │  │ OrderView   │  │ TicketView  │  │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  │
│         │                │                │         │
│         └────────────────┼────────────────┘         │
│                          ▼                          │
│                 ┌─────────────────┐                 │
│                 │  PanelManager   │                 │
│                 │   (单例管理器)   │                 │
│                 └────────┬────────┘                 │
│                          │                          │
│         ┌────────────────┼────────────────┐         │
│         ▼                ▼                ▼         │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │
│  │ XBoard      │  │ V2Board     │  │ 自定义面板   │  │
│  │ (JinDo内置) │  │ (JinGo扩展) │  │ (用户扩展)  │  │
│  └─────────────┘  └─────────────┘  └─────────────┘  │
│         │                │                │         │
│         └────────────────┼────────────────┘         │
│                          ▼                          │
│                 ┌─────────────────┐                 │
│                 │ IPanelProvider  │                 │
│                 │   (统一接口)    │                 │
│                 └─────────────────┘                 │
└─────────────────────────────────────────────────────┘
```

**核心组件：**

| 组件 | 位置 | 说明 |
|------|------|------|
| `IPanelProvider` | JinDo | 面板接口定义 |
| `PanelManager` | JinDo | 面板管理器（注册、切换、配置） |
| `XBoardProvider` | JinDo | XBoard 内置实现 |
| `V2BoardProvider` | JinGo | V2Board 扩展实现（参考） |

---

## 接口定义

所有面板提供者必须实现 `IPanelProvider` 接口：

```cpp
class IPanelProvider : public QObject
{
    Q_OBJECT

public:
    // 回调类型
    using SuccessCallback = std::function<void(const QJsonObject&)>;
    using ErrorCallback = std::function<void(const QString&)>;

    // 面板类型枚举
    enum PanelType { XBoard, V2Board, SSPanel, Custom };

    // ========== 必须实现的方法 ==========

    // 面板信息
    virtual PanelType panelType() const = 0;
    virtual QString panelName() const = 0;
    virtual QString panelVersion() const = 0;

    // 配置
    virtual void setBaseUrl(const QString& url) = 0;
    virtual QString baseUrl() const = 0;
    virtual void setAuthToken(const QString& token) = 0;
    virtual QString authToken() const = 0;

    // 用户认证
    virtual void login(const QString& email, const QString& password,
                       SuccessCallback onSuccess, ErrorCallback onError) = 0;
    virtual void register_(const QString& email, const QString& password,
                           const QString& inviteCode, const QString& emailCode,
                           SuccessCallback onSuccess, ErrorCallback onError) = 0;
    virtual void logout(SuccessCallback onSuccess, ErrorCallback onError) = 0;

    // 用户信息
    virtual void getUserInfo(SuccessCallback onSuccess, ErrorCallback onError) = 0;
    virtual void getSubscribeInfo(SuccessCallback onSuccess, ErrorCallback onError) = 0;

    // 套餐
    virtual void fetchPlans(SuccessCallback onSuccess, ErrorCallback onError) = 0;

    // 订单
    virtual void createOrder(int planId, const QString& period, const QString& couponCode,
                             SuccessCallback onSuccess, ErrorCallback onError) = 0;
    virtual void fetchOrders(int page, int pageSize,
                             SuccessCallback onSuccess, ErrorCallback onError) = 0;

    // 支付
    virtual void fetchPaymentMethods(SuccessCallback onSuccess, ErrorCallback onError) = 0;
    virtual void getPaymentUrl(const QString& tradeNo, const QString& paymentMethod,
                               SuccessCallback onSuccess, ErrorCallback onError) = 0;

    // 工单
    virtual void fetchTickets(int page, int pageSize,
                              SuccessCallback onSuccess, ErrorCallback onError) = 0;
    virtual void createTicket(const QString& subject, int level, const QString& message,
                              SuccessCallback onSuccess, ErrorCallback onError) = 0;

    // 系统配置
    virtual void getSystemConfig(SuccessCallback onSuccess, ErrorCallback onError) = 0;

signals:
    void authenticationChanged(bool authenticated);
    void tokenUpdated(const QString& token);
    void errorOccurred(const QString& error);
};
```

---

## 开发步骤

### 步骤 1: 创建头文件

在 `JinGo/src/panel/` 目录下创建 `MyPanelProvider.h`：

```cpp
#ifndef MYPANELPROVIDER_H
#define MYPANELPROVIDER_H

#include <panel/IPanelProvider.h>
#include <QNetworkAccessManager>

class MyPanelProvider : public IPanelProvider
{
    Q_OBJECT

public:
    explicit MyPanelProvider(QObject* parent = nullptr);
    ~MyPanelProvider() override;

    // 面板信息
    PanelType panelType() const override { return PanelType::Custom; }
    QString panelName() const override { return "MyPanel"; }
    QString panelVersion() const override { return "1.0"; }

    // 配置
    void setBaseUrl(const QString& url) override;
    QString baseUrl() const override;
    void setAuthToken(const QString& token) override;
    QString authToken() const override;

    // 实现所有纯虚函数...
    void login(const QString& email, const QString& password,
               SuccessCallback onSuccess, ErrorCallback onError) override;
    // ... 其他方法

private:
    QString m_baseUrl;
    QString m_authToken;
    QNetworkAccessManager* m_networkManager;
};

#endif // MYPANELPROVIDER_H
```

### 步骤 2: 实现 API 调用

创建 `MyPanelProvider.cpp`：

```cpp
#include "MyPanelProvider.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>

// 定义 API 端点
namespace MyPanelEndpoints {
    const QString Login = "/api/auth/login";
    const QString UserInfo = "/api/user/info";
    // ... 其他端点
}

MyPanelProvider::MyPanelProvider(QObject* parent)
    : IPanelProvider(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

void MyPanelProvider::login(const QString& email,
                            const QString& password,
                            SuccessCallback onSuccess,
                            ErrorCallback onError)
{
    QString url = m_baseUrl + MyPanelEndpoints::Login;

    // 构建请求
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 构建请求体
    QJsonObject data;
    data["email"] = email;
    data["password"] = password;

    // 发送请求
    QNetworkReply* reply = m_networkManager->post(
        request, QJsonDocument(data).toJson());

    // 处理响应
    connect(reply, &QNetworkReply::finished, [=]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            if (onError) onError(reply->errorString());
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject response = doc.object();

        // 提取 Token（根据你的面板响应格式调整）
        QString token = response["data"].toObject()["token"].toString();
        if (!token.isEmpty()) {
            setAuthToken(token);
            emit authenticationChanged(true);
            emit tokenUpdated(token);
        }

        if (onSuccess) onSuccess(response);
    });
}

// 实现其他方法...
```

### 步骤 3: 更新 CMakeLists.txt

在 `JinGo/CMakeLists.txt` 中添加源文件：

```cmake
set(NETWORK_SOURCES
    src/network/V2BoardProvider.cpp
    src/network/MyPanelProvider.cpp    # 添加新文件
)
```

### 步骤 4: 注册面板

在 `main.cpp` 中注册：

```cpp
#include <panel/PanelManager.h>
#include "network/MyPanelProvider.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 注册自定义面板
    PanelManager::instance().registerProvider(
        "mypanel",
        new MyPanelProvider()
    );

    // 设置面板 URL（可从配置读取）
    PanelManager::instance().setPanelUrl(
        "mypanel",
        "https://panel.example.com"
    );

    // ... 其他初始化
}
```

---

## API 端点映射

不同面板系统的 API 端点可能不同，下表列出常见映射：

### 认证相关

| 功能 | XBoard | V2Board | SSPanel |
|------|--------|---------|---------|
| 登录 | `/passport/auth/login` | `/api/v1/passport/auth/login` | `/auth/login` |
| 注册 | `/passport/auth/register` | `/api/v1/passport/auth/register` | `/auth/register` |
| 登出 | `/passport/auth/logout` | `/api/v1/passport/auth/logout` | `/auth/logout` |
| 发送验证码 | `/passport/comm/sendEmailVerify` | `/api/v1/passport/comm/sendEmailVerify` | `/auth/send` |

### 用户相关

| 功能 | XBoard | V2Board | SSPanel |
|------|--------|---------|---------|
| 用户信息 | `/user/info` | `/api/v1/user/info` | `/user` |
| 订阅信息 | `/user/getSubscribe` | `/api/v1/user/getSubscribe` | `/user/subscribe` |
| 重置安全 | `/user/resetSecurity` | `/api/v1/user/resetSecurity` | `/user/reset` |

### 订单相关

| 功能 | XBoard | V2Board | SSPanel |
|------|--------|---------|---------|
| 套餐列表 | `/user/plan/fetch` | `/api/v1/user/plan/fetch` | `/user/shop` |
| 创建订单 | `/user/order/save` | `/api/v1/user/order/save` | `/user/order` |
| 订单列表 | `/user/order/fetch` | `/api/v1/user/order/fetch` | `/user/order` |
| 支付方式 | `/user/order/getPaymentMethod` | `/api/v1/user/order/getPaymentMethod` | `/user/payment` |

### 工单相关

| 功能 | XBoard | V2Board | SSPanel |
|------|--------|---------|---------|
| 工单列表 | `/user/ticket/fetch` | `/api/v1/user/ticket/fetch` | `/user/ticket` |
| 创建工单 | `/user/ticket/save` | `/api/v1/user/ticket/save` | `/user/ticket` |
| 回复工单 | `/user/ticket/reply` | `/api/v1/user/ticket/reply` | `/user/ticket/{id}` |

---

## 数据格式转换

不同面板返回的数据格式可能不同，需要转换为统一格式。

### 登录响应

**XBoard 格式：**
```json
{
  "data": {
    "auth_data": "eyJhbGciOiJIUzI1NiIs..."
  }
}
```

**V2Board 格式：**
```json
{
  "data": {
    "token": "eyJhbGciOiJIUzI1NiIs...",
    "auth_data": "eyJhbGciOiJIUzI1NiIs..."
  }
}
```

**统一处理：**
```cpp
QString token = response["data"].toObject()["auth_data"].toString();
if (token.isEmpty()) {
    token = response["data"].toObject()["token"].toString();
}
```

### 用户信息

**XBoard/V2Board 格式：**
```json
{
  "data": {
    "email": "user@example.com",
    "transfer_enable": 107374182400,  // 字节
    "u": 1073741824,                  // 上传（字节）
    "d": 5368709120,                  // 下载（字节）
    "expired_at": 1735689600          // 时间戳
  }
}
```

**统一格式转换：**
```cpp
QJsonObject normalizeUserInfo(const QJsonObject& data)
{
    QJsonObject result;
    result["email"] = data["email"];

    // 流量单位统一为字节
    qint64 total = data["transfer_enable"].toVariant().toLongLong();
    qint64 up = data["u"].toVariant().toLongLong();
    qint64 down = data["d"].toVariant().toLongLong();

    result["total_traffic"] = total;
    result["used_upload"] = up;
    result["used_download"] = down;
    result["used_traffic"] = up + down;
    result["remaining_traffic"] = total - up - down;

    // 过期时间统一为时间戳
    result["expire_time"] = data["expired_at"];

    return result;
}
```

### 套餐列表

**统一格式：**
```json
{
  "id": 1,
  "name": "基础套餐",
  "price": 9.99,
  "traffic": 107374182400,
  "period": "month",
  "features": ["不限速", "多设备"]
}
```

---

## 注册和使用

### 在应用启动时注册

```cpp
// main.cpp
#include <panel/PanelManager.h>
#include "network/V2BoardProvider.h"
#include "network/MyPanelProvider.h"

void initPanelProviders()
{
    auto& pm = PanelManager::instance();

    // 注册 V2Board
    pm.registerProvider("v2board", new V2BoardProvider());
    pm.setPanelUrl("v2board", "https://v2board.example.com");

    // 注册自定义面板
    pm.registerProvider("mypanel", new MyPanelProvider());
    pm.setPanelUrl("mypanel", "https://mypanel.example.com");

    // 从配置加载用户选择的面板
    QString selectedPanel = Settings::value("panel/selected", "xboard");
    pm.setCurrentProvider(selectedPanel);
}
```

### 在 QML 中切换面板

```qml
ComboBox {
    id: panelSelector
    model: PanelManager.availableProviders

    onCurrentTextChanged: {
        PanelManager.setCurrentProvider(currentText)
    }
}

TextField {
    id: panelUrlField
    placeholderText: "面板 API 地址"

    onEditingFinished: {
        PanelManager.setPanelUrl(
            panelSelector.currentText,
            text
        )
    }
}
```

### 使用面板 API

```cpp
// 通过 PanelManager 调用
PanelManager::instance().login(email, password);

// 或直接获取提供者
auto provider = PanelManager::instance().currentProvider();
provider->fetchPlans(
    [](const QJsonObject& response) {
        // 处理套餐列表
    },
    [](const QString& error) {
        // 处理错误
    }
);
```

---

## 测试建议

### 1. 单元测试

```cpp
void TestMyPanelProvider::testLogin()
{
    MyPanelProvider provider;
    provider.setBaseUrl("https://test.example.com");

    QSignalSpy spy(&provider, &IPanelProvider::authenticationChanged);

    provider.login("test@example.com", "password",
        [](const QJsonObject& response) {
            QVERIFY(response.contains("data"));
        },
        [](const QString& error) {
            QFAIL(error.toUtf8());
        });

    // 等待异步完成
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toBool(), true);
}
```

### 2. 集成测试清单

- [ ] 登录/登出流程
- [ ] Token 刷新
- [ ] 用户信息获取
- [ ] 订阅信息获取
- [ ] 套餐列表获取
- [ ] 订单创建和查询
- [ ] 支付流程
- [ ] 工单创建和回复
- [ ] 错误处理（网络错误、认证失败等）

### 3. 调试技巧

```cpp
// 启用网络请求日志
void MyPanelProvider::request(const QString& url, const QJsonObject& data)
{
    qDebug() << "[MyPanel] Request:" << url;
    qDebug() << "[MyPanel] Data:" << QJsonDocument(data).toJson();

    // ... 发送请求

    // 在响应处理中
    qDebug() << "[MyPanel] Response:" << reply->readAll();
}
```

---

## 参考实现

完整的参考实现请查看：

- **XBoardProvider**: `JinDo/src/panel/XBoardProvider.cpp`
- **V2BoardProvider**: `JinGo/src/panel/V2BoardProvider.cpp`

---

## 常见问题

### Q: 如何处理不同的认证方式？

A: 在 `setAuthToken` 中根据面板类型设置不同的请求头：

```cpp
void MyPanelProvider::setAuthToken(const QString& token)
{
    m_authToken = token;
    // 有些面板使用 "Bearer token"
    // 有些面板使用 "token"
    // 根据你的面板调整
}

QNetworkRequest createRequest(const QString& url)
{
    QNetworkRequest request(url);
    if (!m_authToken.isEmpty()) {
        // XBoard/V2Board 格式
        request.setRawHeader("Authorization", m_authToken.toUtf8());
        // 或 Bearer 格式
        // request.setRawHeader("Authorization",
        //     ("Bearer " + m_authToken).toUtf8());
    }
    return request;
}
```

### Q: 如何处理分页？

A: 统一使用 `page` 和 `pageSize` 参数：

```cpp
void MyPanelProvider::fetchOrders(int page, int pageSize,
                                  SuccessCallback onSuccess,
                                  ErrorCallback onError)
{
    // 有些面板使用 page/pageSize
    QString url = QString("%1/orders?page=%2&pageSize=%3")
        .arg(m_baseUrl).arg(page).arg(pageSize);

    // 有些面板使用 offset/limit
    // int offset = (page - 1) * pageSize;
    // QString url = QString("%1/orders?offset=%2&limit=%3")
    //     .arg(m_baseUrl).arg(offset).arg(pageSize);

    // ...
}
```

### Q: 如何支持 WebSocket 实时通知？

A: 在提供者中添加 WebSocket 支持：

```cpp
class MyPanelProvider : public IPanelProvider
{
    // ...
private:
    QWebSocket* m_webSocket;

public:
    void connectNotifications() {
        m_webSocket = new QWebSocket();
        connect(m_webSocket, &QWebSocket::textMessageReceived,
                this, &MyPanelProvider::onNotification);
        m_webSocket->open(QUrl(m_baseUrl + "/ws"));
    }

private slots:
    void onNotification(const QString& message) {
        // 处理实时通知
    }
};
```
