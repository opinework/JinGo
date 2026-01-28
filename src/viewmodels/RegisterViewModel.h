/**
 * @file RegisterViewModel.h
 * @brief 注册视图模型头文件
 * @details 管理用户注册流程，包括输入验证、注册请求和结果处理
 * @author JinGo VPN Team
 * @date 2025
 */

#ifndef REGISTERVIEWMODEL_H
#define REGISTERVIEWMODEL_H

#include <QObject>
#include <QString>
#include <QTimer>

class AuthManager;

/**
 * @class RegisterViewModel
 * @brief 注册视图模型
 * @details 提供用户注册功能，包括：
 * - 注册信息输入绑定（邮箱、密码）
 * - 输入验证（邮箱格式、密码长度）
 * - 注册请求处理
 * - 错误提示和加载状态管理
 *
 * 与AuthManager配合完成异步注册操作
 */
class RegisterViewModel : public QObject
{
    Q_OBJECT

    /// 注册邮箱（可读写）
    Q_PROPERTY(QString registerEmail READ registerEmail WRITE setRegisterEmail NOTIFY registerEmailChanged)

    /// 注册密码（可读写）
    Q_PROPERTY(QString registerPassword READ registerPassword WRITE setRegisterPassword NOTIFY registerPasswordChanged)

    /// 邀请码（可读写，可选）
    Q_PROPERTY(QString inviteCode READ inviteCode WRITE setInviteCode NOTIFY inviteCodeChanged)

    /// 邮箱验证码（可读写，可选）
    Q_PROPERTY(QString emailCode READ emailCode WRITE setEmailCode NOTIFY emailCodeChanged)

    /// 错误提示信息（只读）
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

    /// 是否正在处理注册请求（只读）
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)

    /// 是否正在发送邮箱验证码（只读）
    Q_PROPERTY(bool isSendingCode READ isSendingCode NOTIFY isSendingCodeChanged)

    /// 验证码倒计时秒数（只读，0表示可以发送）
    Q_PROPERTY(int codeCountdown READ codeCountdown NOTIFY codeCountdownChanged)

    /// reCAPTCHA 验证令牌（可读写）
    Q_PROPERTY(QString recaptchaToken READ recaptchaToken WRITE setRecaptchaToken NOTIFY recaptchaTokenChanged)

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     *
     * @details 初始化视图模型并连接AuthManager的注册结果信号
     */
    explicit RegisterViewModel(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~RegisterViewModel() override = default;

    /**
     * @brief 获取注册邮箱
     * @return 邮箱地址
     */
    QString registerEmail() const { return m_registerEmail; }

    /**
     * @brief 获取注册密码
     * @return 密码
     */
    QString registerPassword() const { return m_registerPassword; }

    /**
     * @brief 获取邀请码
     * @return 邀请码
     */
    QString inviteCode() const { return m_inviteCode; }

    /**
     * @brief 获取邮箱验证码
     * @return 邮箱验证码
     */
    QString emailCode() const { return m_emailCode; }

    /**
     * @brief 获取错误提示信息
     * @return 错误信息字符串，无错误时为空
     */
    QString errorMessage() const { return m_errorMessage; }

    /**
     * @brief 获取是否正在处理注册请求
     * @return true表示正在处理，false表示空闲
     */
    bool isProcessing() const { return m_isProcessing; }

    /**
     * @brief 获取是否正在发送邮箱验证码
     * @return true表示正在发送，false表示空闲
     */
    bool isSendingCode() const { return m_isSendingCode; }

    /**
     * @brief 获取验证码倒计时秒数
     * @return 倒计时秒数，0表示可以发送
     */
    int codeCountdown() const { return m_codeCountdown; }

    /**
     * @brief 获取 reCAPTCHA 验证令牌
     * @return reCAPTCHA 令牌
     */
    QString recaptchaToken() const { return m_recaptchaToken; }

    /**
     * @brief 设置 reCAPTCHA 验证令牌
     * @param token reCAPTCHA 令牌
     */
    Q_INVOKABLE void setRecaptchaToken(const QString& token);

    /**
     * @brief 设置注册邮箱
     * @param email 邮箱地址
     *
     * @details 设置后会清除错误提示
     */
    Q_INVOKABLE void setRegisterEmail(const QString& email);

    /**
     * @brief 设置注册密码
     * @param pwd 密码
     *
     * @details 设置后会清除错误提示
     */
    Q_INVOKABLE void setRegisterPassword(const QString& pwd);

    /**
     * @brief 设置邀请码
     * @param code 邀请码
     *
     * @details 设置后会清除错误提示
     */
    Q_INVOKABLE void setInviteCode(const QString& code);

    /**
     * @brief 设置邮箱验证码
     * @param code 邮箱验证码
     *
     * @details 设置后会清除错误提示
     */
    Q_INVOKABLE void setEmailCode(const QString& code);

    /**
     * @brief 发送邮箱验证码
     *
     * @details 发送流程：
     * 1. 验证邮箱不为空且格式正确
     * 2. 检查是否在倒计时中
     * 3. 调用AuthManager发送验证码
     * 4. 启动60秒倒计时
     */
    Q_INVOKABLE void sendEmailCode();

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
     * 验证失败时会设置相应的错误提示
     */
    Q_INVOKABLE void registerUser();

signals:
    /**
     * @brief 注册邮箱变化信号
     */
    void registerEmailChanged();

    /**
     * @brief 注册密码变化信号
     */
    void registerPasswordChanged();

    /**
     * @brief 邀请码变化信号
     */
    void inviteCodeChanged();

    /**
     * @brief 邮箱验证码变化信号
     */
    void emailCodeChanged();

    /**
     * @brief 错误提示信息变化信号
     */
    void errorMessageChanged();

    /**
     * @brief 处理状态变化信号
     */
    void isProcessingChanged();

    /**
     * @brief 发送验证码状态变化信号
     */
    void isSendingCodeChanged();

    /**
     * @brief 验证码倒计时变化信号
     */
    void codeCountdownChanged();

    /**
     * @brief reCAPTCHA 令牌变化信号
     */
    void recaptchaTokenChanged();

    /**
     * @brief 注册成功信号
     */
    void registrationSucceeded();

    /**
     * @brief 注册失败信号
     * @param error 错误信息
     */
    void registrationFailed(const QString &error);

private:
    /**
     * @brief 设置处理状态
     * @param processing true表示正在处理，false表示空闲
     */
    void setIsProcessing(bool processing);

    /**
     * @brief 设置错误提示信息
     * @param message 错误信息
     */
    void setErrorMessage(const QString& message);

    /**
     * @brief 清除错误提示
     */
    void clearError();

    QString m_registerEmail;    ///< 注册邮箱
    QString m_registerPassword; ///< 注册密码
    QString m_inviteCode;       ///< 邀请码
    QString m_emailCode;        ///< 邮箱验证码
    QString m_recaptchaToken;   ///< reCAPTCHA 验证令牌
    QString m_errorMessage;     ///< 错误提示信息
    bool m_isProcessing = false; ///< 是否正在处理注册请求
    bool m_isSendingCode = false; ///< 是否正在发送验证码
    int m_codeCountdown = 0;    ///< 验证码倒计时秒数

    AuthManager* m_authManager = nullptr; ///< 认证管理器指针
    QTimer* m_countdownTimer = nullptr;   ///< 倒计时定时器

private slots:
    /**
     * @brief 注册成功处理
     *
     * @details 关闭处理状态并发出registrationSucceeded信号
     */
    void onRegistrationSucceeded();

    /**
     * @brief 注册失败处理
     * @param error 错误信息
     *
     * @details 关闭处理状态，设置错误提示并发出registrationFailed信号
     */
    void onRegistrationFailed(const QString& error);

    /**
     * @brief 倒计时定时器槽函数
     *
     * @details 每秒递减倒计时，为0时停止定时器
     */
    void onCountdownTick();
};

#endif // REGISTERVIEWMODEL_H
