#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QPointer>
#include <QScopedPointer>
#include <QSystemTrayIcon>

class QIcon;
class QWindow;
class ModeController;

class ApplicationController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentMode READ currentMode NOTIFY currentModeChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(bool logExpanded READ logExpanded NOTIFY logExpandedChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QAbstractItemModel *logsModel READ logsModel CONSTANT)

public:
    explicit ApplicationController(ModeController *modeController, QObject *parent = nullptr);
    ~ApplicationController() override;

    QString currentMode() const;
    QString statusText() const;
    bool logExpanded() const;
    bool busy() const;
    QAbstractItemModel *logsModel();

    void attachRootWindow(QObject *rootObject);

    Q_INVOKABLE void attachTrayMenuWindow(QObject *trayMenuWindowObject);
    Q_INVOKABLE void showMainWindow();
    Q_INVOKABLE void hideMainWindow();
    Q_INVOKABLE void closeApplication();
    Q_INVOKABLE void toggleLogPanel();
    Q_INVOKABLE void openSettings();
    Q_INVOKABLE void requestModeChange(const QString &mode);
    Q_INVOKABLE void startWindowDrag();

signals:
    void currentModeChanged();
    void statusTextChanged();
    void logExpandedChanged();
    void busyChanged();
    void settingsRequested();

private slots:
    void handleTrayActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void createTrayIcon();
    QIcon buildModeIcon(const QString &mode) const;
    void showTrayMenuWindow();
    void hideTrayMenuWindow();
    void updateTrayVisualState();
    void setCurrentMode(const QString &mode);
    void setStatusText(const QString &text);

    QString m_currentMode;
    QString m_statusText;
    bool m_logExpanded;
    bool m_busy;
    bool m_showMainWindowPending;
    QPointer<QWindow> m_rootWindow;
    QPointer<QWindow> m_trayMenuWindow;
    ModeController *m_modeController;
    QScopedPointer<QSystemTrayIcon> m_trayIcon;
};
