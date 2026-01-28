/**
 * @file LoginViewModel.h
 * @brief 登录视图模型头文件
 * @details 管理登录、注册和密码重置流程，为QML提供MVVM架构的数据绑定
 * @author JinGo VPN Team
 * @date 2025
 */

#ifndef LOGINVIEWMODEL_H
#define LOGINVIEWMODEL_H

#include <QObject>
#include <QString>
#include <QTimer>

class AuthManager;

/**
 * @class LoginViewModel
 * @brief 登录视图模型
 * @details 提供完整的认证流程管理，包括：
 * - 用户登录（输入验证、凭据保存、异步请求）
 * - 用户注册
 * - 密码重置
 * - 加载状态管理（登录/通用操作）
 * - 错误提示管理
 * - 记住密码功能
 *
 * 与AuthManager配合完成所有认证相关的网络操作
 */
class LoginViewModel : public QObject
{
    Q_OBJECT

    /// 登录邮箱（可读写）
    Q_PROPERTY(QString email READ email WRITE setEmail NOTIFY emailChanged)

    /// 登录密码（可读写）
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)

    /// 是否正在登录（只读）
    Q_PROPERTY(bool isLoggingIn READ isLoggingIn NOTIFY isLoggingInChanged)

    /// 是否正在加载（通用后台操作，只读）
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

    /// 错误提示信息（只读）
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

    /// 是否记住密码（可读写）
    Q_PROPERTY(bool rememberPassword READ rememberPassword WRITE setRememberPassword NOTIFY rememberPasswordChanged)

    /// 找回密码邮箱（可读写）
    Q_PROPERTY(QString forgotEmail READ forgotEmail WRITE setForgotEmail NOTIFY forgotEmailChanged)

    /// 找回密码验证码（可读写）
    Q_PROPERTY(QString forgotEmailCode READ forgotEmailCode WRITE setForgotEmailCode NOTIFY forgotEmailCodeChanged)

    /// 找回密码新密码（可读写）
    Q_PROPERTY(QString forgotNewPassword READ forgotNewPassword WRITE setForgotNewPassword NOTIFY forgotNewPasswordChanged)

    /// 找回密码验证码倒计时（只读）
    Q_PROPERTY(int forgotCodeCountdown READ forgotCodeCountdown NOTIFY forgotCodeCountdownChanged)

    /// 是否正在发送验证码（只读）
    Q_PROPERTY(bool isSendingCode READ isSendingCode NOTIFY isSendingCodeChanged)

    /// 成功提示信息（只读）
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)

    /// 是否正在处理找回密码请求（只读）
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     *
     * @details 初始化视图模型，连接AuthManager的各种认证结果信号，
     * 并尝试加载已保存的凭据
     */
    explicit LoginViewModel(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~LoginViewModel() override = default;

    /**
     * @brief 获取邮箱
     * @return 邮箱地址
     */
    QString email() const { return m_email; }

    /**
     * @brief 获取密码
     * @return 密码
     */
    QString password() const { return m_password; }

    /**
     * @brief 获取是否正在登录
     * @return true表示正在登录，false表示未登录
     */
    bool isLoggingIn() const { return m_isLoggingIn; }

    /**
     * @brief 获取是否正在加载
     * @return true表示正在执行后台操作，false表示空闲
     *
     * @details 用于注册、重置密码等操作的Loading显示
     */
    bool isLoading() const { return m_isLoading; }

    /**
     * @brief 获取错误提示信息
     * @return 错误信息字符串，无错误时为空
     */
    QString errorMessage() const { return m_errorMessage; }

    /**
     * @brief 获取是否记住密码
     * @return true表示记住密码，false表示不记住
     */
    bool rememberPassword() const { return m_rememberPassword; }

    /**
     * @brief 获取找回密码邮箱
     * @return 邮箱地址
     */
    QString forgotEmail() const { return m_forgotEmail; }

    /**
     * @brief 获取找回密码验证码
     * @return 验证码
     */
    QString forgotEmailCode() const { return m_forgotEmailCode; }

    /**
     * @brief 获取找回密码新密码
     * @return 新密码
     */
    QString forgotNewPassword() const { return m_forgotNewPassword; }

    /**
     * @brief 获取找回密码验证码倒计时
     * @return 倒计时秒数，0表示可以发送
     */
    int forgotCodeCountdown() const { return m_forgotCodeCountdown; }

    /**
     * @brief 获取是否正在发送验证码
     * @return true表示正在发送，false表示空闲
     */
    bool isSendingCode() const { return m_isSendingCode; }

    /**
     * @brief 获取成功提示信息
     * @return 成功信息字符串
     */
    QString message() const { return m_message; }

    /**
     * @brief 获取是否正在处理找回密码请求
     * @return true表示正在处理，false表示空闲
     */
    bool isProcessing() const { return m_isLoading; }

    /**
     * @brief 设置邮箱
     * @param email 邮箱地址
     *
     * @details 设置后会清除错误提示
     */
    Q_INVOKABLE void setEmail(const QString& email);

    /**
     * @brief 设置密码
     * @param password 密码
     *
     * @details 设置后会清除错误提示
     */
    Q_INVOKABLE void setPassword(const QString& password);

    /**
     * @brief 设置是否记住密码
     * @param remember true表示记住，false表示不记住
     */
    Q_INVOKABLE void setRememberPassword(bool remember);

    /**
     * @brief 设置找回密码邮箱
     * @param email 邮箱地址
     *
     * @details 设置后会清除错误和成功提示
     */
    Q_INVOKABLE void setForgotEmail(const QString& email);

    /**
     * @brief 设置找回密码验证码
     * @param code 验证码
     */
    Q_INVOKABLE void setForgotEmailCode(const QString& code);

    /**
     * @brief 设置找回密码新密码
     * @param password 新密码
     */
    Q_INVOKABLE void setForgotNewPassword(const QString& password);

    /**
     * @brief 发送密码重置链接（旧接口，保留兼容）
     */
    Q_INVOKABLE void sendResetLink();

    /**
     * @brief 发送找回密码验证码
     *
     * @details 发送流程：
     * 1. 验证邮箱不为空
     * 2. 验证邮箱格式
     * 3. 调用AuthManager发送验证码
     * 4. 启动60秒倒计时
     */
    Q_INVOKABLE void sendForgotEmailCode();

    /**
     * @brief 使用验证码重置密码
     *
     * @details 重置流程：
     * 1. 验证邮箱、验证码、新密码不为空
     * 2. 验证密码长度至少6位
     * 3. 调用AuthManager的forgetPassword接口
     */
    Q_INVOKABLE void resetPasswordWithCode();

public slots:
    /**
     * @brief 执行登录操作
     *
     * @details 登录流程：
     * 1. 验证用户名不为空
     * 2. 验证密码不为空
     * 3. 清除错误并设置登录状态
     * 4. 根据rememberPassword保存或删除密码
     * 5. 调用AuthManager发起登录请求
     */
    void login();

    /**
     * @brief 清除错误提示
     *
     * @details 将错误信息设置为空
     */
    void clearError();

    /**
     * @brief 加载已保存的凭据
     *
     * @todo 实现从SecureStorage加载上次登录的账号
     */
    void loadSavedCredentials();

    /**
     * @brief 注册用户
     * @param email 邮箱地址
     * @param password 密码
     *
     * @details 清除错误，设置加载状态并调用AuthManager发起注册请求
     */
    Q_INVOKABLE void registerUser(const QString& email,
                                  const QString& password);

    /**
     * @brief 重置密码
     * @param email 邮箱地址
     *
     * @details 清除错误，设置加载状态并调用AuthManager发起密码重置请求
     */
    Q_INVOKABLE void resetPassword(const QString& email);

signals:
    /**
     * @brief 邮箱变化信号
     */
    void emailChanged();

    /**
     * @brief 密码变化信号
     */
    void passwordChanged();

    /**
     * @brief 登录状态变化信号
     */
    void isLoggingInChanged();

    /**
     * @brief 加载状态变化信号
     */
    void isLoadingChanged();

    /**
     * @brief 错误提示信息变化信号
     */
    void errorMessageChanged();

    /**
     * @brief 记住密码设置变化信号
     */
    void rememberPasswordChanged();

    /**
     * @brief 找回密码邮箱变化信号
     */
    void forgotEmailChanged();

    /**
     * @brief 找回密码验证码变化信号
     */
    void forgotEmailCodeChanged();

    /**
     * @brief 找回密码新密码变化信号
     */
    void forgotNewPasswordChanged();

    /**
     * @brief 找回密码验证码倒计时变化信号
     */
    void forgotCodeCountdownChanged();

    /**
     * @brief 发送验证码状态变化信号
     */
    void isSendingCodeChanged();

    /**
     * @brief 成功提示信息变化信号
     */
    void messageChanged();

    /**
     * @brief 处理状态变化信号
     */
    void isProcessingChanged();

    /**
     * @brief 登录成功信号
     */
    void loginSucceeded();

    /**
     * @brief 登录失败信号
     * @param error 错误信息
     */
    void loginFailed(const QString& error);

    /**
     * @brief 注册成功信号
     */
    void registrationSucceeded();

    /**
     * @brief 注册失败信号
     * @param error 错误信息
     */
    void registrationFailed(const QString& error);

    /**
     * @brief 密码重置成功信号
     */
    void passwordResetSucceeded();

    /**
     * @brief 密码重置失败信号
     * @param error 错误信息
     */
    void passwordResetFailed(const QString& error);

private:
    /**
     * @brief 设置登录状态
     * @param logging true表示正在登录，false表示登录结束
     *
     * @details 用于控制登录按钮的禁用状态和Loading文案
     */
    void setIsLoggingIn(bool logging);

    /**
     * @brief 设置加载状态
     * @param loading true表示正在加载，false表示加载结束
     *
     * @details 用于注册/重置密码等其他流程的Loading显示
     */
    void setIsLoading(bool loading);

    /**
     * @brief 设置错误提示信息
     * @param message 错误信息
     */
    void setErrorMessage(const QString& message);

    /**
     * @brief 设置成功提示信息
     * @param message 成功信息
     */
    void setMessage(const QString& message);

    QString m_email;                 ///< 登录邮箱
    QString m_password;              ///< 登录密码
    bool m_isLoggingIn = false;      ///< 是否正在登录
    bool m_isLoading = false;        ///< 是否正在执行后台操作
    QString m_errorMessage;          ///< 错误提示信息
    bool m_rememberPassword = false; ///< 是否记住密码
    QString m_forgotEmail;           ///< 找回密码邮箱
    QString m_forgotEmailCode;       ///< 找回密码验证码
    QString m_forgotNewPassword;     ///< 找回密码新密码
    int m_forgotCodeCountdown = 0;   ///< 找回密码验证码倒计时
    bool m_isSendingCode = false;    ///< 是否正在发送验证码
    QString m_message;               ///< 成功提示信息

    AuthManager* m_authManager = nullptr; ///< 认证管理器指针
    QTimer* m_countdownTimer = nullptr;   ///< 倒计时定时器

private slots:
    /**
     * @brief 注册成功处理
     *
     * @details 关闭加载状态，发出registrationSucceeded信号并记录日志
     */
    void onRegistrationSucceeded();

    /**
     * @brief 注册失败处理
     * @param error 错误信息
     *
     * @details 关闭加载状态，设置错误提示，发出registrationFailed信号并记录警告日志
     */
    void onRegistrationFailed(const QString& error);

    /**
     * @brief 密码重置邮件发送成功处理
     * @param email 接收邮件的邮箱地址
     *
     * @details 关闭加载状态，设置成功提示，发出passwordResetSucceeded信号并记录日志
     */
    void onPasswordResetEmailSent(const QString& email);

    /**
     * @brief 密码重置失败处理
     * @param error 错误信息
     *
     * @details 关闭加载状态，设置错误提示，发出passwordResetFailed信号并记录警告日志
     */
    void onPasswordResetFailed(const QString& error);

    /**
     * @brief 使用验证码重置密码成功处理
     */
    void onForgetPasswordSucceeded();

    /**
     * @brief 倒计时定时器槽函数
     */
    void onCountdownTick();
};

#endif // LOGINVIEWMODEL_H
