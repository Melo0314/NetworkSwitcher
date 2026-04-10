#include "ApplicationController.h"

#include "../core/ModeController.h"
#include "../services/LoggerService.h"

#include <QCursor>
#include <QGuiApplication>
#include <QIcon>
#include <QImage>
#include <QPainter>
#include <QScreen>
#include <QSvgRenderer>
#include <QWindow>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace {

DECL_LOGGER(logger, "APP");

}

ApplicationController::ApplicationController(ModeController *modeController, QObject *parent)
    : QObject(parent)
    , m_currentMode(modeController->currentMode())
    , m_statusText(modeController->statusText())
    , m_logExpanded(false)
    , m_busy(modeController->busy())
    , m_showMainWindowPending(false)
    , m_modeController(modeController)
{
    createTrayIcon();
    logger.info(QStringLiteral("ApplicationController 已初始化。"));

    connect(m_modeController, &ModeController::currentModeChanged, this, [this]() {
        setCurrentMode(m_modeController->currentMode());
    });
    connect(m_modeController, &ModeController::statusTextChanged, this, [this]() {
        setStatusText(m_modeController->statusText());
    });
    connect(m_modeController, &ModeController::busyChanged, this, [this]() {
        const bool currentBusy = m_modeController->busy();
        if (m_busy == currentBusy) {
            return;
        }

        m_busy = currentBusy;
        emit busyChanged();
    });
    updateTrayVisualState();
}

ApplicationController::~ApplicationController() = default;

QString ApplicationController::currentMode() const
{
    return m_currentMode;
}

QString ApplicationController::statusText() const
{
    return m_statusText;
}

bool ApplicationController::logExpanded() const
{
    return m_logExpanded;
}

bool ApplicationController::busy() const
{
    return m_busy;
}

QAbstractItemModel *ApplicationController::logsModel()
{
    return LoggerService::instance().logsModel();
}

void ApplicationController::attachRootWindow(QObject *rootObject)
{
    m_rootWindow = qobject_cast<QWindow *>(rootObject);
    if (m_rootWindow && m_showMainWindowPending) {
        showMainWindow();
    }
}

void ApplicationController::attachTrayMenuWindow(QObject *trayMenuWindowObject)
{
    m_trayMenuWindow = qobject_cast<QWindow *>(trayMenuWindowObject);
}

void ApplicationController::showMainWindow()
{
    if (!m_rootWindow) {
        m_showMainWindowPending = true;
        return;
    }

    hideTrayMenuWindow();
    m_showMainWindowPending = false;
    if (m_rootWindow->visibility() == QWindow::Minimized) {
        m_rootWindow->setVisibility(QWindow::Windowed);
    }

    m_rootWindow->show();
    m_rootWindow->raise();
    m_rootWindow->requestActivate();

#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(m_rootWindow->winId());
    if (hwnd) {
        ShowWindow(hwnd, IsIconic(hwnd) ? SW_RESTORE : SW_SHOW);
        SetForegroundWindow(hwnd);
    }
#endif
}

void ApplicationController::hideMainWindow()
{
    if (!m_rootWindow) {
        return;
    }

    m_rootWindow->hide();
    logger.info(QStringLiteral("主窗口已隐藏到托盘。"));
}

void ApplicationController::closeApplication()
{
    hideTrayMenuWindow();
    logger.info(QStringLiteral("应用即将退出。"));
    QGuiApplication::quit();
}

void ApplicationController::toggleLogPanel()
{
    m_logExpanded = !m_logExpanded;
    emit logExpandedChanged();
    logger.info(m_logExpanded
                ? QStringLiteral("日志面板已展开。")
                : QStringLiteral("日志面板已折叠。"));
}

void ApplicationController::openSettings()
{
    logger.info(QStringLiteral("设置页已打开。"));
    emit settingsRequested();
}

void ApplicationController::requestModeChange(const QString &mode)
{
    hideTrayMenuWindow();
    if (mode == m_currentMode) {
        logger.info(QStringLiteral("模式未变化，保持为 %1。").arg(mode));
        return;
    }

    logger.info(QStringLiteral("用户请求切换到 %1。").arg(mode));
    if (!m_modeController->switchToMode(mode)) {
        updateTrayVisualState();
    }
}

void ApplicationController::startWindowDrag()
{
    if (!m_rootWindow) {
        return;
    }

#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(m_rootWindow->winId());
    if (!hwnd) {
        return;
    }

    ReleaseCapture();
    SendMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
#endif
}

void ApplicationController::handleTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Context) {
        showTrayMenuWindow();
    } else if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
        showMainWindow();
    }
}

void ApplicationController::createTrayIcon()
{
    m_trayIcon.reset(new QSystemTrayIcon(this));
    m_trayIcon->setIcon(buildModeIcon(m_currentMode));
    m_trayIcon->setToolTip(QStringLiteral("NetworkSwitcher"));
    connect(m_trayIcon.data(), &QSystemTrayIcon::activated, this, &ApplicationController::handleTrayActivated);

    m_trayIcon->show();
    logger.info(QStringLiteral("系统托盘已创建。"));
}

QIcon ApplicationController::buildModeIcon(const QString &mode) const
{
    QColor color(QStringLiteral("#F5A97F"));
    if (mode == QStringLiteral("本地局域网")) {
        color = QColor(QStringLiteral("#5BC0EB"));
    } else if (mode == QStringLiteral("远程局域网")) {
        color = QColor(QStringLiteral("#7BD88F"));
    }

    QIcon icon;
    const QList<int> sizes { 16, 20, 24, 32, 48, 64 };
    for (int size : sizes) {
        QSvgRenderer renderer(QStringLiteral(":/icons/app-switch.svg"));
        QImage image(size, size, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);

        QPainter painter(&image);
        renderer.render(&painter);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(image.rect(), color);
        painter.end();

        icon.addPixmap(QPixmap::fromImage(image));
    }

    return icon;
}

void ApplicationController::showTrayMenuWindow()
{
    if (!m_trayMenuWindow) {
        return;
    }

    const QPoint cursorPos = QCursor::pos();
    QScreen *screen = QGuiApplication::screenAt(cursorPos);
    if (!screen && m_rootWindow) {
        screen = m_rootWindow->screen();
    }
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen) {
        return;
    }

    const QRect availableGeometry = screen->availableGeometry();
    const int menuWidth = m_trayMenuWindow->width();
    const int menuHeight = m_trayMenuWindow->height();

    int x = cursorPos.x() - menuWidth / 2;
    int y = cursorPos.y();

    x = qBound(availableGeometry.left(), x, availableGeometry.right() - menuWidth);
    y = qBound(availableGeometry.top(), y, availableGeometry.bottom() - menuHeight);

    m_trayMenuWindow->setPosition(x, y);
    m_trayMenuWindow->show();
    m_trayMenuWindow->raise();
    m_trayMenuWindow->requestActivate();
}

void ApplicationController::hideTrayMenuWindow()
{
    if (m_trayMenuWindow && m_trayMenuWindow->isVisible()) {
        m_trayMenuWindow->hide();
    }
}

void ApplicationController::updateTrayVisualState()
{
    if (!m_trayIcon) {
        return;
    }

    const QIcon icon = buildModeIcon(m_currentMode);
    m_trayIcon->setIcon(icon);
    m_trayIcon->setToolTip(QStringLiteral("NetworkSwitcher - %1").arg(m_currentMode));
}

void ApplicationController::setCurrentMode(const QString &mode)
{
    if (m_currentMode == mode) {
        return;
    }

    m_currentMode = mode;
    emit currentModeChanged();
    updateTrayVisualState();
}

void ApplicationController::setStatusText(const QString &text)
{
    if (m_statusText == text) {
        return;
    }

    m_statusText = text;
    emit statusTextChanged();
}
