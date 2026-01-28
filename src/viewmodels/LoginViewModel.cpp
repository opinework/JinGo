/**
 * @file LoginViewModel.cpp
 * @brief 登录视图模型实现文件
 * @details 实现登录、注册和密码重置的业务逻辑和状态管理
 * @author JinGo VPN Team
 * @date 2025
 */

#include "LoginViewModel.h"
#include "panel/AuthManager.h"
#include "storage/SecureStorage.h"
#include "core/Logger.h"

/**
 * @brief 构造函数
 * @param parent 父对象
 *
 * @details 初始化流程：
 * 1. 初始化成员变量
 * 2. 获取AuthManager实例
 * 3. 连接登录成功/失败信号
 * 4. 连接注册成功/失败信号
 * 5. 连接密码重置成功/失败信号
 * 6. 加载已保存的凭据
 */
LoginViewModel::LoginViewModel(QObject* parent)
    : QObject(parent)
    , m_isLoggingIn(false)      // 登录流程的 loading 状态
    , m_isLoading(false)        // 通用后台操作状态（注册/重置密码）
    , m_rememberPassword(false)
    , m_authManager(&AuthManager::instance())
    , m_countdownTimer(new QTimer(this))
{
    // 倒计时定时器
    connect(m_countdownTimer, &QTimer::timeout,
            this, &LoginViewModel::onCountdownTick);
    // 登录成功：关闭登录状态通知 UI 并发射成功信号
    connect(m_authManager, &AuthManager::loginSucceeded,
            this, [this]() {
                setIsLoggingIn(false);
                emit loginSucceeded();
            });

    // 登录失败：关闭登录状态显示错误信息
    connect(m_authManager, &AuthManager::loginFailed,
            this, [this](const QString& error) {
                setIsLoggingIn(false);
                setErrorMessage(error);
                emit loginFailed(error);
                LOG_WARNING(QString("Login failed: %1").arg(error));
            });

    // 注册成功与失败回调
    connect(m_authManager, &AuthManager::registrationSucceeded,
            this, &LoginViewModel::onRegistrationSucceeded);
    connect(m_authManager, &AuthManager::registrationFailed,
            this, &LoginViewModel::onRegistrationFailed);

    // 密码重置回调
    connect(m_authManager, &AuthManager::passwordResetEmailSent,
            this, &LoginViewModel::onPasswordResetEmailSent);
    connect(m_authManager, &AuthManager::passwordResetFailed,
            this, &LoginViewModel::onPasswordResetFailed);

    // 使用验证码重置密码成功回调
    connect(m_authManager, &AuthManager::passwordResetSucceeded,
            this, &LoginViewModel::onForgetPasswordSucceeded);

    // 初始化自动填充凭据（若存在）
    loadSavedCredentials();
}

/**
 * @brief 设置邮箱
 * @param email 邮箱地址
 *
 * @details 如果邮箱发生变化，更新成员变量，发出信号并清除错误提示
 */
void LoginViewModel::setEmail(const QString& email)
{
    if (m_email != email) {
        m_email = email;
        emit emailChanged();
        clearError();              // 输入变化时清除错误提示
    }
}

/**
 * @brief 设置密码
 * @param password 密码
 *
 * @details 如果密码发生变化，更新成员变量，发出信号并清除错误提示
 */
void LoginViewModel::setPassword(const QString& password)
{
    if (m_password != password) {
        m_password = password;
        emit passwordChanged();
        clearError();
    }
}

/**
 * @brief 设置是否记住密码
 * @param remember true表示记住，false表示不记住
 *
 * @details 如果设置发生变化，更新成员变量并发出信号
 */
void LoginViewModel::setRememberPassword(bool remember)
{
    if (m_rememberPassword != remember) {
        m_rememberPassword = remember;
        emit rememberPasswordChanged();
    }
}

/**
 * @brief 设置找回密码邮箱
 * @param email 邮箱地址
 *
 * @details 如果邮箱发生变化，更新成员变量，发出信号并清除错误和成功提示
 */
void LoginViewModel::setForgotEmail(const QString& email)
{
    if (m_forgotEmail != email) {
        m_forgotEmail = email;
        emit forgotEmailChanged();
        clearError();
        setMessage(QString());
    }
}

/**
 * @brief 发送密码重置链接
 *
 * @details 发送流程：
 * 1. 验证邮箱不为空
 * 2. 验证邮箱格式
 * 3. 清除错误和成功提示
 * 4. 设置处理状态
 * 5. 调用AuthManager发送重置邮件
 */
void LoginViewModel::sendResetLink()
{
    // 验证邮箱
    if (m_forgotEmail.isEmpty()) {
        setErrorMessage(tr("Enter email"));
        return;
    }

    // 简单的邮箱格式验证
    if (!m_forgotEmail.contains("@") || !m_forgotEmail.contains(".")) {
        setErrorMessage(tr("Invalid email format"));
        return;
    }

    clearError();
    setMessage(QString());
    setIsLoading(true);

    // 调用AuthManager发送重置邮件
    m_authManager->resetPassword(m_forgotEmail);
}

/**
 * @brief 设置登录状态
 * @param logging true表示正在登录，false表示登录结束
 *
 * @details 如果状态发生变化，更新成员变量并发出isLoggingInChanged信号
 * - QML根据此信号禁用登录按钮及显示Loading文案
 */
void LoginViewModel::setIsLoggingIn(bool logging)
{
    if (m_isLoggingIn != logging) {
        m_isLoggingIn = logging;
        emit isLoggingInChanged(); // QML：禁用按钮及 UI loading 文案
    }
}

/**
 * @brief 设置加载状态
 * @param loading true表示正在加载，false表示加载结束
 *
 * @details 如果状态发生变化，更新成员变量并发出isLoadingChanged信号
 * - 用于注册/重置密码等其他流程的Loading显示
 */
void LoginViewModel::setIsLoading(bool loading)
{
    if (m_isLoading != loading) {
        m_isLoading = loading;
        emit isLoadingChanged(); // 用于注册/重置密码等其他流程
    }
}

/**
 * @brief 设置错误提示信息
 * @param message 错误信息
 *
 * @details 如果错误信息发生变化，更新成员变量并发出errorMessageChanged信号
 * - QML根据此信号更新错误提示UI
 */
void LoginViewModel::setErrorMessage(const QString& message)
{
    if (m_errorMessage != message) {
        m_errorMessage = message;
        emit errorMessageChanged(); // QML 更新错误提示
    }
}

/**
 * @brief 设置成功提示信息
 * @param message 成功信息
 *
 * @details 如果成功信息发生变化，更新成员变量并发出messageChanged信号
 * - QML根据此信号更新成功提示UI
 */
void LoginViewModel::setMessage(const QString& message)
{
    if (m_message != message) {
        m_message = message;
        emit messageChanged();
    }
}

/**
 * @brief 执行登录操作
 *
 * @details 登录流程：
 * 1. 验证用户名不为空
 * 2. 验证密码不为空
 * 3. 清除错误并设置登录状态
 * 4. 根据rememberPassword保存或删除密码到SecureStorage
 * 5. 调用AuthManager发起登录请求
 *
 * 验证失败时会设置相应的错误提示并直接返回
 */
void LoginViewModel::login()
{
    if (m_email.isEmpty()) {
        setErrorMessage(tr("Please enterUsername"));
        return;
    }

    if (m_password.isEmpty()) {
        setErrorMessage(tr("Enter password"));
        return;
    }

    clearError();
    setIsLoggingIn(true); // 开启登录流程状态，让 UI 进入 Loading 表现

    // 按需保存凭据
    if (m_rememberPassword) {
        SecureStorage::savePassword(m_email, m_password);
        SecureStorage::saveSecret("last_username", m_email);
        LOG_DEBUG("Credentials saved");
    } else {
        SecureStorage::deletePassword(m_email);
        SecureStorage::deleteSecret("last_username");
    }

    m_authManager->login(m_email, m_password);
}

/**
 * @brief 注册用户
 * @param email 邮箱地址
 * @param password 密码
 *
 * @details 清除错误，设置加载状态并调用AuthManager发起注册请求
 */
void LoginViewModel::registerUser(const QString& email, const QString& password)
{
    clearError();
    setIsLoading(true);
    m_authManager->register_(email, password);
}

/**
 * @brief 重置密码
 * @param email 邮箱地址
 *
 * @details 清除错误，设置加载状态并调用AuthManager发起密码重置请求
 */
void LoginViewModel::resetPassword(const QString& email)
{
    clearError();
    setIsLoading(true);
    m_authManager->resetPassword(email);
}

/**
 * @brief 注册成功处理
 *
 * @details 关闭加载状态，发出registrationSucceeded信号并记录日志
 */
void LoginViewModel::onRegistrationSucceeded()
{
    setIsLoading(false);
    emit registrationSucceeded();
}

/**
 * @brief 注册失败处理
 * @param error 错误信息
 *
 * @details 关闭加载状态，设置错误提示，发出registrationFailed信号并记录警告日志
 */
void LoginViewModel::onRegistrationFailed(const QString& error)
{
    setIsLoading(false);
    setErrorMessage(error);
    emit registrationFailed(error);
    LOG_WARNING(QString("Registration failed: %1").arg(error));
}

/**
 * @brief 密码重置邮件发送成功处理
 * @param email 接收邮件的邮箱地址
 *
 * @details 关闭加载状态，设置成功提示信息，发出passwordResetSucceeded信号并记录日志
 */
void LoginViewModel::onPasswordResetEmailSent(const QString& email)
{
    setIsLoading(false);
    setMessage(tr("Password reset email sent to %1").arg(email));
    emit passwordResetSucceeded();
    LOG_INFO(QString("Password reset email sent to: %1").arg(email));
}

/**
 * @brief 密码重置失败处理
 * @param error 错误信息
 *
 * @details 关闭加载状态，设置错误提示，发出passwordResetFailed信号并记录警告日志
 */
void LoginViewModel::onPasswordResetFailed(const QString& error)
{
    setIsLoading(false);
    setErrorMessage(error);
    emit passwordResetFailed(error);
    LOG_WARNING(QString("Password reset failed: %1").arg(error));
}

/**
 * @brief 清除错误提示
 *
 * @details 将错误信息设置为空字符串
 */
void LoginViewModel::clearError()
{
    setErrorMessage(QString());
}

/**
 * @brief 加载已保存的凭据
 *
 * @details 尝试从SecureStorage加载上次登录的账号和密码
 * 1. 读取上次登录的用户名
 * 2. 如果存在，尝试读取该用户名对应的密码
 * 3. 如果密码存在，自动填充并启用"记住密码"
 */
void LoginViewModel::loadSavedCredentials()
{
    // 尝试加载上次登录的用户名
    QString lastUsername = SecureStorage::loadSecret("last_username");
    if (lastUsername.isEmpty()) {
        LOG_DEBUG("No saved credentials found");
        return;
    }

    // 尝试加载该用户名对应的密码
    QString savedPassword = SecureStorage::loadPassword(lastUsername);
    if (savedPassword.isEmpty()) {
        // 只有用户名，没有密码，可能是用户取消了记住密码
        setEmail(lastUsername);
        LOG_DEBUG("Loaded saved username (no password)");
        return;
    }

    // 自动填充凭据
    setEmail(lastUsername);
    setPassword(savedPassword);
    setRememberPassword(true);

    LOG_INFO("Loaded saved credentials for: " + lastUsername);
}

/**
 * @brief 设置找回密码验证码
 * @param code 验证码
 */
void LoginViewModel::setForgotEmailCode(const QString& code)
{
    if (m_forgotEmailCode != code) {
        m_forgotEmailCode = code;
        emit forgotEmailCodeChanged();
        clearError();
    }
}

/**
 * @brief 设置找回密码新密码
 * @param password 新密码
 */
void LoginViewModel::setForgotNewPassword(const QString& password)
{
    if (m_forgotNewPassword != password) {
        m_forgotNewPassword = password;
        emit forgotNewPasswordChanged();
        clearError();
    }
}

/**
 * @brief 发送找回密码验证码
 *
 * @details 发送流程：
 * 1. 验证邮箱不为空
 * 2. 验证邮箱格式
 * 3. 检查是否在倒计时中
 * 4. 调用AuthManager发送验证码
 * 5. 启动60秒倒计时
 */
void LoginViewModel::sendForgotEmailCode()
{
    // 验证邮箱
    if (m_forgotEmail.isEmpty()) {
        setErrorMessage(tr("Enter email"));
        return;
    }

    // 简单的邮箱格式验证
    if (!m_forgotEmail.contains("@") || !m_forgotEmail.contains(".")) {
        setErrorMessage(tr("Invalid email format"));
        return;
    }

    // 检查是否在倒计时中
    if (m_forgotCodeCountdown > 0) {
        return;
    }

    clearError();
    m_isSendingCode = true;
    emit isSendingCodeChanged();

    // 发送验证码
    m_authManager->sendEmailVerificationCode(m_forgotEmail);

    // 立即启动倒计时（简化处理）
    m_isSendingCode = false;
    emit isSendingCodeChanged();

    m_forgotCodeCountdown = 60;
    emit forgotCodeCountdownChanged();
    m_countdownTimer->start(1000);
    LOG_INFO("Forgot password email verification code sending");
}

/**
 * @brief 使用验证码重置密码
 *
 * @details 重置流程：
 * 1. 验证邮箱不为空
 * 2. 验证验证码不为空
 * 3. 验证新密码不为空且至少6位
 * 4. 调用AuthManager的forgetPassword接口
 */
void LoginViewModel::resetPasswordWithCode()
{
    if (m_forgotEmail.isEmpty()) {
        setErrorMessage(tr("Enter email"));
        return;
    }

    if (m_forgotEmailCode.isEmpty()) {
        setErrorMessage(tr("Enter verification code"));
        return;
    }

    if (m_forgotNewPassword.isEmpty()) {
        setErrorMessage(tr("Enter new password"));
        return;
    }

    if (m_forgotNewPassword.length() < 6) {
        setErrorMessage(tr("Password must be at least 6 characters"));
        return;
    }

    clearError();
    setIsLoading(true);

    // 调用 xboard 的 forgetPassword 接口
    m_authManager->forgetPassword(m_forgotEmail, m_forgotEmailCode, m_forgotNewPassword);
}

/**
 * @brief 使用验证码重置密码成功处理
 */
void LoginViewModel::onForgetPasswordSucceeded()
{
    setIsLoading(false);
    setMessage(tr("Password reset successfully! Please login with new password."));
    emit passwordResetSucceeded();
    LOG_INFO("Password reset with verification code succeeded");
}

/**
 * @brief 倒计时定时器槽函数
 *
 * @details 每秒递减倒计时，为0时停止定时器
 */
void LoginViewModel::onCountdownTick()
{
    if (m_forgotCodeCountdown > 0) {
        m_forgotCodeCountdown--;
        emit forgotCodeCountdownChanged();

        if (m_forgotCodeCountdown == 0) {
            m_countdownTimer->stop();
        }
    }
}
