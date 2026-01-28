// src/ui/SystemTrayManager.h
#ifndef SYSTEMTRAYMANAGER_H
#define SYSTEMTRAYMANAGER_H

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>

/**
 * @brief 系统托盘管理器
 * 
 * 提供系统托盘图标和菜单功能，可以从 QML 中调用
 */
class SystemTrayManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(bool connected READ isConnected WRITE setConnected NOTIFY connectedChanged)
    
public:
    explicit SystemTrayManager(QObject* parent = nullptr);
    ~SystemTrayManager() override;
    
    // 属性访问器
    bool isVisible() const;
    void setVisible(bool visible);
    
    bool isConnected() const;
    void setConnected(bool connected);
    
    /**
     * @brief 显示托盘通知消息
     */
    Q_INVOKABLE void showMessage(const QString& title, 
                                  const QString& message,
                                  int duration = 3000);
    
    /**
     * @brief 设置工具提示文本
     */
    Q_INVOKABLE void setToolTip(const QString& text);
    
signals:
    void visibleChanged();
    void connectedChanged();
    
    // 用户操作信号
    void activated();
    void showWindowRequested();
    void quickConnectRequested();
    void disconnectRequested();
    void settingsRequested();
    void quitRequested();
    
private slots:
    void onActivated(QSystemTrayIcon::ActivationReason reason);
    void onShowWindow();
    void onConnectAction();
    void onSettings();
    void onQuit();
    
private:
    void createTrayIcon();
    void createMenu();
    void updateMenu();
    
private:
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_menu;
    
    // 菜单项
    QAction* m_showAction;
    QAction* m_connectAction;
    QAction* m_settingsAction;
    QAction* m_quitAction;
    
    bool m_isConnected;
};

#endif // SYSTEMTRAYMANAGER_H