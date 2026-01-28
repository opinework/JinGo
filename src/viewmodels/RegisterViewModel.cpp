/**
 * @file RegisterViewModel.cpp
 * @brief 注册视图模型实现文件
 * @details 实现用户注册的输入验证、请求处理和结果响应
 * @author JinGo VPN Team
 * @date 2025
 */

#include "RegisterViewModel.h"
#include "panel/AuthManager.h"
#include "core/Logger.h"

/**
 * @brief 构造函数
 * @param parent 父对象
 *
 * @details 初始化视图模型并连接AuthManager的注册成功/失败信号
 */
RegisterViewModel::RegisterViewModel(QObject* parent)
    : QObject(parent)
    , m_authManager(&AuthManager::instance())
    , m_countdownTimer(new QTimer(this))
{
    // 注册成功
    connect(m_authManager, &AuthManager::registrationSucceeded,
            this, &RegisterViewModel::onRegistrationSucceeded);

    // 注册失败
    connect(m_authManager, &AuthManager::registrationFailed,
            this, &RegisterViewModel::onRegistrationFailed);

    // 倒计时定时器
    connect(m_countdownTimer, &QTimer::timeout,
            this, &RegisterViewModel::onCountdownTick);
}

/**
 * @brief 设置注册邮箱
 * @param email 邮箱地址
 *
 * @details 如果邮箱发生变化，更新成员变量，发出信号并清除错误提示
 */
void RegisterViewModel::setRegisterEmail(const QString &email)
{
    if (m_registerEmail != email) {
        m_registerEmail = email;
        emit registerEmailChanged();
        clearError();
    }
}

/**
 * @brief 设置注册密码
 * @param pwd 密码
 *
 * @details 如果密码发生变化，更新成员变量，发出信号并清除错误提示
 */
void RegisterViewModel::setRegisterPassword(const QString &pwd)
{
    if (m_registerPassword != pwd) {
        m_registerPassword = pwd;
        emit registerPasswordChanged();
        clearError();
    }
}

/**
 * @brief 设置邀请码
 * @param code 邀请码
 *
 * @details 如果邀请码发生变化，更新成员变量，发出信号并清除错误提示
 */
void RegisterViewModel::setInviteCode(const QString &code)
{
    if (m_inviteCode != code) {
        m_inviteCode = code;
        emit inviteCodeChanged();
        clearError();
    }
}

/**
 * @brief 设置邮箱验证码
 * @param code 邮箱验证码
 *
 * @details 如果邮箱验证码发生变化，更新成员变量，发出信号并清除错误提示
 */
void RegisterViewModel::setEmailCode(const QString &code)
{
    if (m_emailCode != code) {
        m_emailCode = code;
        emit emailCodeChanged();
        clearError();
    }
}

/**
 * @brief 设置 reCAPTCHA 验证令牌
 * @param token reCAPTCHA 令牌
 */
void RegisterViewModel::setRecaptchaToken(const QString &token)
{
    if (m_recaptchaToken != token) {
        m_recaptchaToken = token;
        emit recaptchaTokenChanged();
    }
}

/**
 * @brief 发送邮箱验证码
 *
 * @details 发送流程：
 * 1. 验证邮箱不为空
 * 2. 验证邮箱格式
 * 3. 检查是否在倒计时中
 * 4. 调用AuthManager发送验证码
 * 5. 启动60秒倒计时
 */
void RegisterViewModel::sendEmailCode()
{
    // 验证邮箱
    if (m_registerEmail.isEmpty()) {
        setErrorMessage(tr("Enter email"));
        return;
    }

    // 简单的邮箱格式验证
    if (!m_registerEmail.contains("@") || !m_registerEmail.contains(".")) {
        setErrorMessage(tr("Invalid email format"));
        return;
    }

    // 检查是否在倒计时中
    if (m_codeCountdown > 0) {
        return;
    }

    clearError();
    m_isSendingCode = true;
    emit isSendingCodeChanged();

    // 发送验证码
    m_authManager->sendEmailVerificationCode(m_registerEmail);

    // 立即启动倒计时（简化处理）
    m_isSendingCode = false;
    emit isSendingCodeChanged();

    m_codeCountdown = 60;
    emit codeCountdownChanged();
    m_countdownTimer->start(1000);
    LOG_INFO("Email verification code sending");
}

/**
 * @brief 执行注册操作
 *
 * @details 注册流程：
 * 1. 验证邮箱不为空
 * 2. 验证密码不为空
 * 3. 验证密码长度至少6位
 * 4. 清除错误并设置处理状态
 * 5. 调用AuthManager发起注册请求
 *
 * 任何验证失败都会设置相应的错误提示并直接返回
 */
void RegisterViewModel::registerUser()
{
    if (m_registerEmail.isEmpty()) {
        setErrorMessage(tr("Enter email"));
        return;
    }
    if (m_registerPassword.isEmpty()) {
        setErrorMessage(tr("Enter password"));
        return;
    }
    if (m_registerPassword.length() < 6) {
        setErrorMessage(tr("Password must be at least 6 characters"));
        return;
    }

    clearError();
    setIsProcessing(true);

    // 发起注册（传递邀请码和邮箱验证码）
    m_authManager->register_(m_registerEmail, m_registerPassword, m_inviteCode, m_emailCode);
}

/**
 * @brief 注册成功处理
 *
 * @details 关闭处理状态，发出registrationSucceeded信号并记录日志
 */
void RegisterViewModel::onRegistrationSucceeded()
{
    setIsProcessing(false);
    emit registrationSucceeded();
}

/**
 * @brief 注册失败处理
 * @param error 错误信息
 *
 * @details 关闭处理状态，设置错误提示，发出registrationFailed信号并记录警告日志
 */
void RegisterViewModel::onRegistrationFailed(const QString &error)
{
    setIsProcessing(false);
    setErrorMessage(error);
    emit registrationFailed(error);
    LOG_WARNING(QString("Registration failed: %1").arg(error));
}

/**
 * @brief 清除错误提示
 *
 * @details 将错误信息设置为空字符串
 */
void RegisterViewModel::clearError()
{
    setErrorMessage("");
}

/**
 * @brief 设置错误提示信息
 * @param message 错误信息
 *
 * @details 如果错误信息发生变化，更新成员变量并发出errorMessageChanged信号
 */
void RegisterViewModel::setErrorMessage(const QString &message)
{
    if (m_errorMessage != message) {
        m_errorMessage = message;
        emit errorMessageChanged();
    }
}

/**
 * @brief 设置处理状态
 * @param processing true表示正在处理，false表示空闲
 *
 * @details 如果处理状态发生变化，更新成员变量并发出isProcessingChanged信号
 */
void RegisterViewModel::setIsProcessing(bool processing)
{
    if (m_isProcessing != processing) {
        m_isProcessing = processing;
        emit isProcessingChanged();
    }
}

/**
 * @brief 倒计时定时器槽函数
 *
 * @details 每秒递减倒计时，为0时停止定时器
 */
void RegisterViewModel::onCountdownTick()
{
    if (m_codeCountdown > 0) {
        m_codeCountdown--;
        emit codeCountdownChanged();

        if (m_codeCountdown == 0) {
            m_countdownTimer->stop();
        }
    }
}
