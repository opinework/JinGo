/**
 * @file AuthManager.h
 * @brief 身份验证管理器头文件
 * @details 提供完整的用户认证功能，包括登录、登出、注册、Token 管理和会话持久化
 * @author JinGo VPN Team
 * @date 2025
 */

#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <QObject>
#include <QString>
#include <QJsonObject>

// 前向声明
class User;
class ApiClient;

// ============================================================================
// AuthManager 类定义
// ============================================================================

/**
 * @class AuthManager
 * @brief 身份验证管理器（单例模式）
 *
 * @details
 * 核心功能：
 * - 用户认证：登录、登出、注册
 * - Token 管理：自动刷新、验证、持久化
 * - 会话管理：保存到安全存储、自动恢复
 * - 密码管理：修改密码、重置密码
 * - 用户信息：管理当前登录用户对象
 * - 信号通知：异步操作结果通过信号发送
 *
 * 架构特点：
 * - 单例设计：全局唯一实例
 * - QML 集成：支持 Q_INVOKABLE 和 Q_PROPERTY
 * - 异步操作：所有网络请求都是异步的
 * - 自动持久化：登录状态保存到加密存储
 * - Token 自动刷新：定时器每小时刷新一次
 *
 * 使用流程：
 * 1. 获取单例：AuthManager::instance()
 * 2. 连接信号：监听 loginSucceeded/loginFailed 等
 * 3. 调用方法：login(username, password)
 * 4. 处理回调：在槽函数中处理结果
 *
 * @note
 * - 线程安全：单例实例在多线程环境下是安全的
 * - 内存管理：User 对象由 AuthManager 管理
 * - 会话安全：敏感数据存储在 SecureStorage 中
 *
 * @example C++ 使用示例
 * @code
 * // 连接信号
 * connect(&AuthManager::instance(), &AuthManager::loginSucceeded,
 *         this, &MyClass::onLoginSuccess);
 * connect(&AuthManager::instance(), &AuthManager::loginFailed,
 *         this, &MyClass::onLoginFailed);
 *
 * // 执行登录
 * AuthManager::instance().login("user@example.com", "password123");
 *
 * // 槽函数处理
 * void MyClass::onLoginSuccess() {
 *     User* user = AuthManager::instance().currentUser();
 *     qDebug() << "Welcome:" << user->username();
 * }
 * @endcode
 *
 * @example QML 使用示例
 * @code
 * import JinGo 1.0
 *
 * Button {
 *     text: "登录"
 *     onClicked: {
 *         AuthManager.login(usernameField.text, passwordField.text)
 *     }
 * }
 *
 * Connections {
 *     target: AuthManager
 *     function onLoginSucceeded() {
 *         console.log("登录成功:", AuthManager.currentUser.username)
 *     }
 *     function onLoginFailed(error) {
 *         console.error("登录失败:", error)
 *     }
 * }
 *
 * // 属性绑定
 * Text {
 *     text: AuthManager.isAuthenticated ? "已登录" : "未登录"
 * }
 * @endcode
 */
class AuthManager : public QObject
{
    Q_OBJECT

    // ========================================================================
    // QML 属性
    // ========================================================================

    /**
     * @property isAuthenticated
     * @brief 当前认证状态（只读属性）
     * @details
     * - true：用户已登录
     * - false：用户未登录
     * - 可在 QML 中绑定，自动更新 UI
     * @notify authenticationChanged()
     */
    Q_PROPERTY(bool isAuthenticated READ isAuthenticated NOTIFY authenticationChanged)

    /**
     * @property currentUser
     * @brief 当前登录用户对象（只读属性）
     * @details
     * - 返回 User* 指针，未登录时为 nullptr
     * - 可在 QML 中访问用户属性（username、email、流量等）
     * - 对象生命周期由 AuthManager 管理
     * @notify currentUserChanged()
     * @see User
     */
    Q_PROPERTY(User* currentUser READ currentUser NOTIFY currentUserChanged)

    /**
     * @property subscribeInfo
     * @brief 用户订阅信息（只读属性）
     * @details
     * - 返回 QVariantMap 包含流量、过期时间等订阅信息
     * - 数据从本地数据库读取，不触发网络请求
     * - 可在 QML 中访问订阅详情
     * @notify subscribeInfoChanged()
     */
    Q_PROPERTY(QVariantMap subscribeInfo READ subscribeInfo NOTIFY subscribeInfoChanged)

    /**
     * @property plans
     * @brief 可用套餐计划列表（只读属性）
     * @details
     * - 返回 QVariantList 包含所有可用套餐
     * - 数据从本地数据库读取，不触发网络请求
     * - 可在 QML 中显示套餐列表
     * @notify plansChanged()
     */
    Q_PROPERTY(QVariantList plans READ plans NOTIFY plansChanged)

public:
    // ========================================================================
    // 单例访问
    // ========================================================================

    /**
     * @brief 获取 AuthManager 单例实例
     * @return AuthManager& 全局唯一实例的引用
     * @note 线程安全（C++11 静态局部变量保证）
     */
    static AuthManager& instance();

    // ========================================================================
    // 核心认证方法
    // ========================================================================

    /**
     * @brief 用户登录
     * @param username 用户名或邮箱地址
     * @param password 用户密码
     *
     * @details
     * 登录流程：
     * 1. 验证参数（用户名和密码不能为空）
     * 2. 如果已登录则先登出
     * 3. 发送 loginStarted() 信号
     * 4. 发起异步 POST 请求到 /auth/login
     * 5. 等待响应并触发相应信号
     *
     * 响应信号：
     * - 成功：loginSucceeded() + authenticationChanged()
     * - 失败：loginFailed(QString error)
     *
     * @note
     * - 异步操作，不会阻塞 UI
     * - 登录成功后自动保存会话到安全存储
     * - 自动设置 ApiClient 的 Token
     * - 自动启动 Token 刷新定时器
     *
     * @warning
     * - 必须在 QML 中使用 Q_INVOKABLE 标记
     * - 密码明文传输前会通过 HTTPS 加密
     *
     * @see loginStarted(), loginSucceeded(), loginFailed(), logout()
     */
    Q_INVOKABLE void login(const QString& username, const QString& password);

    Q_INVOKABLE void fetchUserProfile();

    /**
     * @brief 获取用户订阅信息
     *
     * @details
     * 获取流程：
     * 1. 检查是否已登录（未登录无法获取）
     * 2. 发送请求到 /user/getSubscribe
     * 3. 服务器返回订阅信息（订阅链接、流量等）
     * 4. 更新当前用户的订阅信息
     *
     * 响应信号：
     * - 成功：subscribeInfoLoaded(QJsonObject)
     * - 失败：subscribeInfoLoadFailed(QString error)
     *
     * @note
     * - 必须已登录才能获取订阅信息
     * - 订阅信息会自动更新到当前用户对象
     *
     * @see subscribeInfoLoaded(), subscribeInfoLoadFailed()
     */
    Q_INVOKABLE void getUserSubscribe();

    /**
     * @brief 获取所有可用订阅计划
     *
     * @details
     * 获取流程：
     * 1. 检查是否已登录（未登录无法获取）
     * 2. 发送请求到 /user/plan/fetch
     * 3. 服务器返回所有可用的订阅计划列表
     *
     * 响应信号：
     * - 成功：plansLoaded(QJsonArray)
     * - 失败：plansLoadFailed(QString error)
     *
     * @note
     * - 必须已登录才能获取订阅计划
     * - 返回的是计划列表，可能包含多个套餐
     *
     * @see plansLoaded(), plansLoadFailed()
     */
    Q_INVOKABLE void fetchPlans();

    /**
     * @brief 用户登出
     *
     * @details
     * 登出流程：
     * 1. 检查是否已认证
     * 2. 发送登出请求到服务器（可选，不等待响应）
     * 3. 清除本地会话数据（Token、用户信息等）
     * 4. 清除 ApiClient 的 Token
     * 5. 清除当前用户对象
     * 6. 发送 logoutCompleted() 和 authenticationChanged() 信号
     *
     * @note
     * - 即使服务器请求失败，本地状态也会清除
     * - 会自动停止 Token 刷新定时器
     * - 敏感数据从安全存储中删除
     *
     * @see login(), logoutCompleted(), authenticationChanged()
     */
    Q_INVOKABLE void logout();

    /**
     * @brief 检查是否已认证
     * @return bool true 表示已登录，false 表示未登录
     * @note
     * - 可用于权限检查
     * - 可在 QML 中直接访问 isAuthenticated 属性
     */
    bool isAuthenticated() const;

    /**
     * @brief 获取当前登录用户
     * @return User* 当前用户对象指针，未登录时返回 nullptr
     * @note
     * - 返回的指针由 AuthManager 管理，调用者不应删除
     * - 可用于访问用户信息（用户名、邮箱、流量、过期时间等）
     * @see User
     */
    User* currentUser() const;

    /**
     * @brief 获取当前身份验证 Token
     * @return QString 当前 Token 字符串，未登录时返回空字符串
     * @note
     * - Token 格式通常为 JWT
     * - 用于手动发起需要认证的请求
     * - 已自动设置到 ApiClient，通常无需手动获取
     */
    QString authToken() const;

    /**
     * @brief 获取用户订阅信息
     * @return QVariantMap 订阅信息，未登录或无数据时返回空 map
     * @note 数据从本地数据库读取，不触发网络请求
     */
    QVariantMap subscribeInfo() const;

    /**
     * @brief 获取可用套餐计划列表
     * @return QVariantList 套餐列表，无数据时返回空 list
     * @note 数据从本地数据库读取，不触发网络请求
     */
    QVariantList plans() const;

    // ========================================================================
    // Token 管理
    // ========================================================================

    /**
     * @brief 刷新身份验证 Token
     *
     * @details
     * 刷新流程：
     * 1. 检查是否已认证且有 Token
     * 2. 从安全存储读取 refresh_token（如果有）
     * 3. 发送刷新请求到 /auth/refresh
     * 4. 成功时更新 Token 并保存会话
     * 5. 失败时执行登出操作
     *
     * 触发场景：
     * - 定时器自动触发（每小时一次）
     * - 手动调用（主动刷新）
     * - Token 即将过期时（提前 5 分钟）
     *
     * @note
     * - 刷新失败会自动登出，需要用户重新登录
     * - 成功时发出 tokenRefreshed() 信号
     *
     * @see tokenRefreshed(), checkTokenExpiry(), setupTokenRefreshTimer()
     */
    void refreshToken();

    /**
     * @brief 从安全存储加载保存的会话
     * @return bool true 表示成功加载会话，false 表示无有效会话
     *
     * @details
     * 加载流程：
     * 1. 从 SecureStorage 读取保存的 Token
     * 2. 检查 Token 是否过期
     * 3. 读取并解析用户信息 JSON
     * 4. 创建 User 对象
     * 5. 恢复认证状态
     * 6. 设置 ApiClient 的 Token
     * 7. 发出 authenticationChanged() 信号
     * 8. 异步验证 Token 有效性
     *
     * 调用时机：
     * - 应用启动时自动调用（构造函数中）
     * - 实现"记住登录"功能
     *
     * @note
     * - Token 过期或数据无效会自动清除会话
     * - 成功加载后会验证 Token 是否被服务器撤销
     *
     * @see saveSession(), clearSession(), verifyToken()
     */
    bool loadSession();

    /**
     * @brief 清除保存的会话数据
     *
     * @details
     * 清除内容：
     * - Token（加密存储中的 auth_token）
     * - 用户信息（加密存储中的 current_user）
     * - Refresh Token（加密存储中的 refresh_token）
     * - 过期时间（加密存储中的 expires_at）
     *
     * @note
     * - 只清除存储数据，不影响内存中的状态
     * - 通常在登出或会话无效时调用
     * - 内存中的状态需要单独清理
     *
     * @see saveSession(), logout()
     */
    void clearSession();

    /**
     * @brief 检查 Token 是否即将过期
     * @return bool true 表示 Token 在缓冲时间内（5分钟）将过期
     *
     * @details
     * 检查逻辑：
     * 1. 从安全存储读取过期时间
     * 2. 计算当前时间 + 缓冲时间（5分钟）
     * 3. 比较是否超过过期时间
     *
     * @note
     * - 缓冲时间为 5 分钟（TOKEN_EXPIRY_BUFFER）
     * - 可用于提前刷新 Token，避免请求中断
     * - 如果没有过期时间信息，返回 false
     *
     * @see refreshToken()
     */
    bool checkTokenExpiry() const;

    // ========================================================================
    // 用户管理方法
    // ========================================================================

    /**
     * @brief 注册新用户
     * @param email 邮箱地址
     * @param password 密码
     * @param inviteCode 邀请码（可选）
     * @param emailCode 邮箱验证码（可选）
     *
     * @details
     * 注册流程：
     * 1. 验证参数（邮箱和密码不能为空）
     * 2. 发送 registrationStarted() 信号
     * 3. 发起异步 POST 请求到 /passport/auth/register
     * 4. 等待响应并触发相应信号
     *
     * 响应信号：
     * - 成功：registrationSucceeded()
     * - 失败：registrationFailed(QString error)
     *
     * @note
     * - XBoard 后端支持邀请码和邮箱验证码
     * - 某些后端在注册成功后直接返回登录信息，会自动登录
     * - 注册成功后可能需要邮箱验证（取决于后端配置）
     *
     * @see registrationStarted(), registrationSucceeded(), registrationFailed()
     */
    Q_INVOKABLE void register_(const QString& email,
                               const QString& password,
                               const QString& inviteCode = QString(),
                               const QString& emailCode = QString());

    /**
     * @brief 请求重置密码（传统方式 - 发送邮件）
     * @param email 用户邮箱地址
     *
     * @details
     * 重置流程：
     * 1. 验证邮箱不为空
     * 2. 保存邮箱到临时变量（用于成功回调）
     * 3. 发送请求到 /passport/auth/reset-password
     * 4. 服务器发送重置密码邮件给用户
     * 5. 用户通过邮件中的链接重置密码
     *
     * 响应信号：
     * - 成功：passwordResetEmailSent(QString email)
     * - 失败：passwordResetFailed(QString error)
     *
     * @note
     * - 此方法只发送重置请求，实际重置在网页中完成
     * - 成功只表示邮件发送成功，不表示密码已重置
     *
     * @see passwordResetEmailSent(), passwordResetFailed()
     */
    Q_INVOKABLE void resetPassword(const QString& email);

    /**
     * @brief 发送邮箱验证码
     * @param email 用户邮箱地址
     *
     * @details
     * 发送流程：
     * 1. 验证邮箱不为空
     * 2. 发送请求到 /passport/comm/sendEmailVerify
     * 3. 服务器向邮箱发送验证码
     *
     * 响应信号：
     * - 成功：emailVerificationCodeSent(QString email)
     * - 失败：emailVerificationCodeFailed(QString error)
     *
     * @note
     * - 用于注册或忘记密码时获取邮箱验证码
     * - 验证码通常有效期为几分钟
     * - 可能有发送频率限制
     *
     * @see emailVerificationCodeSent(), emailVerificationCodeFailed()
     */
    Q_INVOKABLE void sendEmailVerificationCode(const QString& email);

    /**
     * @brief 忘记密码 - 使用邮箱验证码重置（XBoard 方式）
     * @param email 用户邮箱地址
     * @param emailCode 邮箱验证码
     * @param password 新密码
     *
     * @details
     * XBoard 忘记密码流程：
     * 1. 验证所有参数不为空
     * 2. 发送请求到 /passport/auth/forget
     * 3. 服务器验证邮箱验证码
     * 4. 验证通过后直接重置密码
     *
     * 响应信号：
     * - 成功：passwordResetSucceeded()
     * - 失败：passwordResetFailed(QString error)
     *
     * @note
     * - 需要先通过 sendEmailVerificationCode() 获取 emailCode
     * - 此方法直接重置密码，不需要通过邮件链接
     * - XBoard 专用接口
     *
     * @see sendEmailVerificationCode(), passwordResetSucceeded(), passwordResetFailed()
     */
    Q_INVOKABLE void forgetPassword(const QString& email,
                                     const QString& emailCode,
                                     const QString& password);

    /**
     * @brief 修改密码
     * @param oldPassword 旧密码
     * @param newPassword 新密码
     *
     * @details
     * 修改流程：
     * 1. 检查是否已登录（未登录无法修改）
     * 2. 验证密码不为空
     * 3. 发送请求到 /auth/change-password
     * 4. 服务器验证旧密码并更新新密码
     *
     * 响应信号：
     * - 成功：passwordChangeSucceeded()
     * - 失败：passwordChangeFailed(QString error)
     *
     * @note
     * - 必须已登录才能修改密码
     * - 旧密码错误会导致修改失败
     * - 修改成功后无需重新登录（Token 仍然有效）
     *
     * @see passwordChangeSucceeded(), passwordChangeFailed()
     */
    Q_INVOKABLE void changePassword(const QString& oldPassword, const QString& newPassword);

    /**
     * @brief 重置订阅安全信息（重置订阅链接和令牌）
     *
     * @details
     * 重置流程：
     * 1. 检查是否已登录（未登录无法重置）
     * 2. 发送请求到 /user/resetSecurity
     * 3. 服务器重新生成订阅链接和令牌
     * 4. 返回新的订阅信息
     *
     * 响应信号：
     * - 成功：resetSecuritySucceeded()
     * - 失败：resetSecurityFailed(QString error)
     *
     * @note
     * - 必须已登录才能重置安全信息
     * - 重置后旧的订阅链接将失效
     * - 建议重置后重新获取订阅信息
     *
     * @see resetSecuritySucceeded(), resetSecurityFailed(), getUserSubscribe()
     */
    Q_INVOKABLE void resetSecurity();

    // ========================================================================
    // 邀请码和推荐相关方法
    // ========================================================================

    /**
     * @brief 获取邀请信息
     *
     * @details
     * 获取流程：
     * 1. 检查是否已登录
     * 2. 发送请求到 /user/invite/fetch
     * 3. 返回邀请码、邀请链接、佣金等信息
     *
     * 响应信号：
     * - 成功：inviteInfoLoaded(QJsonObject)
     * - 失败：inviteInfoFailed(QString error)
     *
     * @see inviteInfoLoaded(), inviteInfoFailed()
     */
    Q_INVOKABLE void fetchInviteInfo();

    /**
     * @brief 佣金提现
     * @param amount 提现金额
     * @param withdrawMethod 提现方式ID
     *
     * @details
     * 提现流程：
     * 1. 检查是否已登录
     * 2. 验证提现金额
     * 3. 发送请求到 /user/invite/withdraw
     * 4. 等待审核处理
     *
     * 响应信号：
     * - 成功：withdrawSucceeded()
     * - 失败：withdrawFailed(QString error)
     *
     * @see withdrawSucceeded(), withdrawFailed()
     */
    Q_INVOKABLE void withdrawCommission(double amount, int withdrawMethod);

    /**
     * @brief 生成邀请二维码
     *
     * @details
     * 生成流程：
     * 1. 检查是否已登录
     * 2. 发送请求到 /user/invite/qrcode
     * 3. 返回二维码图片URL或Base64数据
     *
     * 响应信号：
     * - 成功：qrcodeGenerated(QString qrcode)
     * - 失败：qrcodeFailed(QString error)
     *
     * @see qrcodeGenerated(), qrcodeFailed()
     */
    Q_INVOKABLE void generateInviteQRCode();

    // ========================================================================
    // 信号定义
    // ========================================================================

signals:
    /**
     * @brief 登录开始信号
     * @details 在发起登录请求时立即发出，可用于显示 Loading 状态
     */
    void loginStarted();

    /**
     * @brief 登录成功信号
     * @details 登录成功后发出，此时 currentUser 和 authToken 已设置
     */
    void loginSucceeded();

    /**
     * @brief 登录失败信号
     * @param error 错误描述信息
     * @details 登录失败时发出，包含详细的错误原因
     */
    void loginFailed(const QString& error);

    /**
     * @brief 登出完成信号
     * @details 登出操作完成后发出，此时所有状态已清除
     */
    void logoutCompleted();

    /**
     * @brief 认证状态变化信号
     * @details
     * 在以下情况发出：
     * - 登录成功
     * - 登出完成
     * - 会话加载成功
     * - Token 验证失败（自动登出）
     *
     * @note 可用于 QML 属性绑定，自动更新 UI
     */
    void authenticationChanged();

    /**
     * @brief 当前用户变化信号
     * @details
     * 在以下情况发出：
     * - 登录成功（设置新用户）
     * - 登出完成（清除用户）
     * - 用户信息更新
     *
     * @note 可用于 QML 属性绑定，自动更新 UI
     */
    void currentUserChanged();

    /**
     * @brief Token 刷新成功信号
     * @details Token 刷新成功后发出，此时 authToken 已更新
     */
    void tokenRefreshed();

    /**
     * @brief 注册开始信号
     * @details 在发起注册请求时立即发出，可用于显示 Loading 状态
     */
    void registrationStarted();

    /**
     * @brief 注册成功信号
     * @details 注册成功后发出，可能会自动登录（取决于后端）
     */
    void registrationSucceeded();

    /**
     * @brief 注册失败信号
     * @param error 错误描述信息（如：用户名已存在、邮箱格式错误等）
     */
    void registrationFailed(const QString& error);

    /**
     * @brief 密码重置邮件已发送信号
     * @param email 接收重置邮件的邮箱地址
     * @details 成功发送重置密码邮件后发出
     */
    void passwordResetEmailSent(const QString& email);

    /**
     * @brief 邮箱验证码发送成功信号
     * @param email 接收验证码的邮箱地址
     * @details 成功发送邮箱验证码后发出
     */
    void emailVerificationCodeSent(const QString& email);

    /**
     * @brief 邮箱验证码发送失败信号
     * @param error 错误描述信息
     */
    void emailVerificationCodeFailed(const QString& error);

    /**
     * @brief 密码重置成功信号
     * @details 使用验证码重置密码成功后发出（XBoard forget 接口）
     */
    void passwordResetSucceeded();

    /**
     * @brief 密码重置失败信号
     * @param error 错误描述信息（如：邮箱不存在、发送失败等）
     */
    void passwordResetFailed(const QString& error);

    /**
     * @brief 密码修改成功信号
     * @details 密码修改成功后发出，无需重新登录
     */
    void passwordChangeSucceeded();

    /**
     * @brief 密码修改失败信号
     * @param error 错误描述信息（如：旧密码错误、新密码不符合要求等）
     */
    void passwordChangeFailed(const QString& error);

    /**
     * @brief 订阅信息加载成功信号
     * @param data 订阅信息JSON数据
     * @details 成功获取订阅信息后发出，包含订阅链接、流量等信息
     */
    void subscribeInfoLoaded(const QJsonObject& data);

    /**
     * @brief 订阅信息加载失败信号
     * @param error 错误描述信息
     */
    void subscribeInfoLoadFailed(const QString& error);

    /**
     * @brief 订阅计划加载成功信号
     * @param plans 订阅计划JSON数组
     * @details 成功获取订阅计划列表后发出
     */
    void plansLoaded(const QJsonArray& plans);

    /**
     * @brief 订阅计划加载失败信号
     * @param error 错误描述信息
     */
    void plansLoadFailed(const QString& error);

    /**
     * @brief 重置安全信息成功信号
     * @details 成功重置订阅链接和令牌后发出
     */
    void resetSecuritySucceeded();

    /**
     * @brief 重置安全信息失败信号
     * @param error 错误描述信息
     */
    void resetSecurityFailed(const QString& error);

    /**
     * @brief 订阅信息变化信号
     * @details 订阅信息从数据库加载或更新时发出
     */
    void subscribeInfoChanged();

    /**
     * @brief 套餐列表变化信号
     * @details 套餐列表从数据库加载或更新时发出
     */
    void plansChanged();

    /**
     * @brief 邀请信息加载成功信号
     * @param data 邀请信息JSON数据
     * @details 包含邀请码、邀请链接、佣金等信息
     */
    void inviteInfoLoaded(const QJsonObject& data);

    /**
     * @brief 邀请信息加载失败信号
     * @param error 错误描述信息
     */
    void inviteInfoFailed(const QString& error);

    /**
     * @brief 佣金提现成功信号
     * @details 提现申请已提交，等待审核
     */
    void withdrawSucceeded();

    /**
     * @brief 佣金提现失败信号
     * @param error 错误描述信息
     */
    void withdrawFailed(const QString& error);

    /**
     * @brief 邀请二维码生成成功信号
     * @param qrcode 二维码数据（URL或Base64字符串）
     */
    void qrcodeGenerated(const QString& qrcode);

    /**
     * @brief 邀请二维码生成失败信号
     * @param error 错误描述信息
     */
    void qrcodeFailed(const QString& error);

    // ========================================================================
    // 私有部分
    // ========================================================================

private:
    /**
     * @brief 私有构造函数（单例模式）
     * @param parent 父对象指针
     * @details 初始化成员变量并尝试加载保存的会话
     */
    AuthManager(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     * @details 保存当前会话（如果已认证）并清理用户对象
     */
    ~AuthManager();

    // 禁用拷贝和赋值（单例模式）
    AuthManager(const AuthManager&) = delete;
    AuthManager& operator=(const AuthManager&) = delete;

    // ========================================================================
    // 私有辅助方法
    // ========================================================================

    /**
     * @brief 保存当前会话到安全存储
     * @details 保存 Token、用户信息和过期时间到加密存储
     */
    void saveSession();

    /**
     * @brief 设置当前用户
     * @param user 新的用户对象指针
     * @details 管理用户对象的生命周期并发出 currentUserChanged 信号
     */
    void setCurrentUser(User* user);

    /**
     * @brief 从 JSON 更新用户信息
     * @param json 包含用户信息的 JSON 对象
     * @details
     * 支持更新：
     * - 基本信息（username、email、avatarUrl、isPremium）
     * - V2Board/VmessPanel 字段（transfer_enable、u、d、expired_at）
     * - 通用字段（expiryDate、totalTraffic、usedTraffic）
     */
    void updateUserFromJson(const QJsonObject& json);

    /**
     * @brief 设置 Token 自动刷新定时器
     * @details 创建定时器，每小时自动刷新一次 Token
     */
    void setupTokenRefreshTimer();

    /**
     * @brief 从数据库加载订阅信息
     * @details 加载当前用户的订阅信息并更新缓存
     */
    void loadSubscribeInfoFromDatabase();

    /**
     * @brief 从数据库加载套餐列表
     * @details 加载所有可用套餐并更新缓存
     */
    void loadPlansFromDatabase();

    // ========================================================================
    // 私有槽函数 - 网络请求回调
    // ========================================================================

private slots:
    // 登录相关槽函数

    /**
     * @brief 登录请求成功回调
     * @param response 服务器返回的 JSON 响应
     * @details 解析 Token 和用户信息，更新认证状态
     */
    void onLoginResponse(const QJsonObject& response);

    /**
     * @brief 登录请求失败回调
     * @param error 错误描述信息
     * @details 清理状态并发出 loginFailed 信号
     */
    void onLoginError(const QString& error);

    void onUserProfileSuccess(const QJsonObject& response);

    void onUserProfileError(const QString& error);

    // Token 刷新相关槽函数

    /**
     * @brief Token 刷新成功回调
     * @param response 服务器返回的 JSON 响应
     * @details 更新 Token 并保存会话
     */
    void onTokenRefreshSuccess(const QJsonObject& response);

    /**
     * @brief Token 刷新失败回调
     * @param error 错误描述信息
     * @details Token 刷新失败，执行登出操作
     */
    void onTokenRefreshError(const QString& error);

    // Token 验证相关槽函数

    // ❌ 已禁用：xboard 不支持 token 验证接口
    // void onTokenVerifySuccess(const QJsonObject& response);
    // void onTokenVerifyError(const QString& error);

    // 注册相关槽函数

    /**
     * @brief 注册请求成功回调
     * @param response 服务器返回的 JSON 响应
     * @details 发出 registrationSucceeded 信号，可能自动登录
     */
    void onRegistrationSuccess(const QJsonObject& response);

    /**
     * @brief 注册请求失败回调
     * @param error 错误描述信息
     * @details 发出 registrationFailed 信号
     */
    void onRegistrationError(const QString& error);

    // 邮箱验证码相关槽函数

    /**
     * @brief 邮箱验证码发送成功回调
     * @param response 服务器返回的 JSON 响应
     * @details 发出 emailVerificationCodeSent 信号
     */
    void onEmailVerificationCodeSuccess(const QJsonObject& response);

    /**
     * @brief 邮箱验证码发送失败回调
     * @param error 错误描述信息
     * @details 发出 emailVerificationCodeFailed 信号
     */
    void onEmailVerificationCodeError(const QString& error);

    // 密码重置相关槽函数

    /**
     * @brief 密码重置邮件发送成功回调
     * @param response 服务器返回的 JSON 响应
     * @details 发出 passwordResetEmailSent 信号
     */
    void onPasswordResetSuccess(const QJsonObject& response);

    /**
     * @brief 密码重置请求失败回调
     * @param error 错误描述信息
     * @details 发出 passwordResetFailed 信号
     */
    void onPasswordResetError(const QString& error);

    // 忘记密码相关槽函数

    /**
     * @brief 忘记密码成功回调（XBoard）
     * @param response 服务器返回的 JSON 响应
     * @details 发出 passwordResetSucceeded 信号
     */
    void onForgetPasswordSuccess(const QJsonObject& response);

    /**
     * @brief 忘记密码失败回调（XBoard）
     * @param error 错误描述信息
     * @details 发出 passwordResetFailed 信号
     */
    void onForgetPasswordError(const QString& error);

    // 密码修改相关槽函数

    /**
     * @brief 密码修改成功回调
     * @param response 服务器返回的 JSON 响应
     * @details 发出 passwordChangeSucceeded 信号
     */
    void onPasswordChangeSuccess(const QJsonObject& response);

    /**
     * @brief 密码修改失败回调
     * @param error 错误描述信息
     * @details 发出 passwordChangeFailed 信号
     */
    void onPasswordChangeError(const QString& error);

    // 订阅信息相关槽函数

    /**
     * @brief 订阅信息获取成功回调
     * @param response 服务器返回的 JSON 响应
     * @details 更新用户订阅信息并发出 subscribeInfoLoaded 信号
     */
    void onSubscribeInfoSuccess(const QJsonObject& response);

    /**
     * @brief 订阅信息获取失败回调
     * @param error 错误描述信息
     * @details 发出 subscribeInfoLoadFailed 信号
     */
    void onSubscribeInfoError(const QString& error);

    // 订阅计划相关槽函数

    /**
     * @brief 订阅计划获取成功回调
     * @param response 服务器返回的 JSON 响应
     * @details 解析计划列表并发出 plansLoaded 信号
     */
    void onPlansSuccess(const QJsonObject& response);

    /**
     * @brief 订阅计划获取失败回调
     * @param error 错误描述信息
     * @details 发出 plansLoadFailed 信号
     */
    void onPlansError(const QString& error);

    /**
     * @brief 重置安全信息成功回调
     * @param response 服务器响应 JSON
     * @details 发出 resetSecuritySucceeded 信号
     */
    void onResetSecuritySuccess(const QJsonObject& response);

    /**
     * @brief 重置安全信息失败回调
     * @param error 错误描述信息
     * @details 发出 resetSecurityFailed 信号
     */
    void onResetSecurityError(const QString& error);

    // ========================================================================
    // 私有成员变量
    // ========================================================================

private:
    ApiClient& m_apiClient;         ///< API 客户端引用（单例）
    User* m_currentUser;            ///< 当前登录用户对象指针
    QString m_authToken;            ///< 当前身份验证 Token（JWT）
    bool m_isAuthenticated;         ///< 认证状态标志
    QString m_pendingResetEmail;    ///< 待重置密码的邮箱地址（临时存储）

    // Keychain缓存（减少macOS钥匙串授权提示）
    QString m_cachedRefreshToken;   ///< 缓存的刷新Token（避免重复读取Keychain）
    QDateTime m_cachedExpiresAt;    ///< 缓存的过期时间（避免重复读取Keychain）

    // 订阅信息和套餐缓存（从数据库加载）
    QVariantMap m_subscribeInfo;    ///< 用户订阅信息缓存
    QVariantList m_plans;            ///< 套餐计划列表缓存
};

#endif // AUTHMANAGER_H