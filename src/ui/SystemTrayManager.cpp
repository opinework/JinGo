// src/ui/SystemTrayManager.cpp
#include "SystemTrayManager.h"
#include <QApplication>
#include <QDebug>

SystemTrayManager::SystemTrayManager(QObject* parent)
    : QObject(parent)
    , m_trayIcon(nullptr)
    , m_menu(nullptr)
    , m_showAction(nullptr)
    , m_connectAction(nullptr)
    , m_settingsAction(nullptr)
    , m_quitAction(nullptr)
    , m_isConnected(false)
{
    createTrayIcon();
    createMenu();
}

SystemTrayManager::~SystemTrayManager()
{
    if (m_trayIcon) {
        m_trayIcon->hide();
        delete m_trayIcon;
    }
    
    if (m_menu) {
        delete m_menu;
    }
}

// ============================================================================
// 属性访问器
// ============================================================================

bool SystemTrayManager::isVisible() const
{
    return m_trayIcon && m_trayIcon->isVisible();
}

void SystemTrayManager::setVisible(bool visible)
{
    if (m_trayIcon && m_trayIcon->isVisible() != visible) {
        m_trayIcon->setVisible(visible);
        emit visibleChanged();
    }
}

bool SystemTrayManager::isConnected() const
{
    return m_isConnected;
}

void SystemTrayManager::setConnected(bool connected)
{
    if (m_isConnected != connected) {
        m_isConnected = connected;
        updateMenu();
        emit connectedChanged();
    }
}

// ============================================================================
// 公共方法
// ============================================================================

void SystemTrayManager::showMessage(const QString& title, 
                                    const QString& message,
                                    int duration)
{
    if (m_trayIcon && m_trayIcon->isVisible()) {
        m_trayIcon->showMessage(title, message, 
                               QSystemTrayIcon::Information, 
                               duration);
    }
}

void SystemTrayManager::setToolTip(const QString& text)
{
    if (m_trayIcon) {
        m_trayIcon->setToolTip(text);
    }
}

// ============================================================================
// 私有方法
// ============================================================================

void SystemTrayManager::createTrayIcon()
{
    // 检查系统托盘是否可用
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qWarning("System tray is not available on this system");
        return;
    }
    
    m_trayIcon = new QSystemTrayIcon(this);
    
    // 设置图标
    QIcon icon(":/icons/app.png");
    if (icon.isNull()) {
        qWarning("Failed to load tray icon from :/icons/app.png");
        // 创建一个简单的默认图标
        QPixmap pixmap(32, 32);
        pixmap.fill(Qt::blue);
        icon = QIcon(pixmap);
    }
    m_trayIcon->setIcon(icon);
    
    // 设置工具提示
    m_trayIcon->setToolTip(tr("JinGo VPN"));
    
    // 连接激活信号
    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, &SystemTrayManager::onActivated);
    
    // 显示托盘图标
    m_trayIcon->show();
}

void SystemTrayManager::createMenu()
{
    if (!m_trayIcon) {
        return;
    }
    
    m_menu = new QMenu();
    
    // 显示主窗口
    m_showAction = m_menu->addAction(tr("Show Main Window"));
    m_showAction->setIcon(QIcon::fromTheme("window"));
    connect(m_showAction, &QAction::triggered, 
            this, &SystemTrayManager::onShowWindow);
    
    m_menu->addSeparator();
    
    // 快速连接/断开
    m_connectAction = m_menu->addAction(tr("Quick Connect"));
    m_connectAction->setIcon(QIcon::fromTheme("network-connect"));
    connect(m_connectAction, &QAction::triggered,
            this, &SystemTrayManager::onConnectAction);
    
    m_menu->addSeparator();
    
    // 设置
    m_settingsAction = m_menu->addAction(tr("Settings"));
    m_settingsAction->setIcon(QIcon::fromTheme("preferences-system"));
    connect(m_settingsAction, &QAction::triggered,
            this, &SystemTrayManager::onSettings);
    
    m_menu->addSeparator();
    
    // 退出
    m_quitAction = m_menu->addAction(tr("Quit"));
    m_quitAction->setIcon(QIcon::fromTheme("application-exit"));
    connect(m_quitAction, &QAction::triggered,
            this, &SystemTrayManager::onQuit);
    
    // 设置上下文菜单
    m_trayIcon->setContextMenu(m_menu);
}

void SystemTrayManager::updateMenu()
{
    if (!m_connectAction) {
        return;
    }
    
    if (m_isConnected) {
        m_connectAction->setText(tr("DisconnectConnect"));
        m_connectAction->setIcon(QIcon::fromTheme("network-disconnect"));
    } else {
        m_connectAction->setText(tr("Quick Connect"));
        m_connectAction->setIcon(QIcon::fromTheme("network-connect"));
    }
}

// ============================================================================
// 槽函数
// ============================================================================

void SystemTrayManager::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
        case QSystemTrayIcon::Trigger:
            // 单击 (Windows/Linux 左键点击)
            emit activated();
            break;
            
        case QSystemTrayIcon::DoubleClick:
            // 双击 - 显示主窗口
            emit showWindowRequested();
            break;

        case QSystemTrayIcon::MiddleClick:
            // 中键点击
            break;
            
        default:
            break;
    }
}

void SystemTrayManager::onShowWindow()
{
    emit showWindowRequested();
}

void SystemTrayManager::onConnectAction()
{
    if (m_isConnected) {
        emit disconnectRequested();
    } else {
        emit quickConnectRequested();
    }
}

void SystemTrayManager::onSettings()
{
    emit settingsRequested();
}

void SystemTrayManager::onQuit()
{
    emit quitRequested();
}