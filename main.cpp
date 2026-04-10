#include <QApplication>
#include <QIcon>
#include <QList>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickStyle>
#include <QTimer>

#include "app/ApplicationController.h"
#include "core/ModeController.h"
#include "core/SettingsRepository.h"
#include "models/AdapterListModel.h"
#include "models/RouteTableModel.h"
#include "services/LoggerService.h"
#include "services/NetworkService.h"
#include "services/OpenVpnService.h"
#include "services/ProcessService.h"
#include "services/SingleInstanceService.h"
#include "services/StartupService.h"
#include "viewmodels/SettingsViewModel.h"

namespace {

const char kSingleInstanceMemoryKey[] = "NetworkSwitcher.SingleInstance";
const char kSingleInstanceServerName[] = "NetworkSwitcher.ActivationServer";

DECL_LOGGER(appLogger, "APP");
DECL_LOGGER(qmlLogger, "QML");

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("NetworkSwitcher"));
    app.setApplicationDisplayName(QStringLiteral("NetworkSwitcher"));
    app.setApplicationVersion(QStringLiteral(APP_VERSION_STR));
    app.setOrganizationName(QStringLiteral("NetworkSwitcher"));
    app.setQuitOnLastWindowClosed(false);
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/app-switch.svg")));

    SingleInstanceService singleInstanceService(
        QString::fromLatin1(kSingleInstanceMemoryKey),
        QString::fromLatin1(kSingleInstanceServerName),
        &app);
    if (!singleInstanceService.initialize()) {
        singleInstanceService.notifyPrimaryInstance();
        return 0;
    }

    LoggerService::initializeSingleton(app.applicationDirPath() + QStringLiteral("/log.log"), &app);
    appLogger.info(QStringLiteral("应用启动，版本：%1。").arg(app.applicationVersion()));

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    int exitCode = -1;
    {
        QQmlApplicationEngine engine;
        QObject::connect(&engine, &QQmlApplicationEngine::warnings, &app,
        [](const QList<QQmlError> &warnings) {
            for (const QQmlError &warning : warnings) {
                qmlLogger.error(warning.toString());
            }
        });

        ProcessService processService;
        NetworkService networkService(&processService);
        SettingsRepository settingsRepository;
        OpenVpnService openVpnService;
        ModeController modeController(&settingsRepository, &networkService, &openVpnService);
        StartupService startupService(&processService);
        AdapterListModel adapterListModel;
        RouteTableModel routeTableModel;
        SettingsViewModel settingsViewModel(
            &settingsRepository,
            &networkService,
            &startupService,
            &adapterListModel,
            &routeTableModel);
        ApplicationController applicationController(&modeController);
        QObject::connect(&singleInstanceService, &SingleInstanceService::activationRequested, &applicationController,
        [&singleInstanceService, &applicationController]() {
            if (!singleInstanceService.consumePendingActivationRequest()) {
                return;
            }

            appLogger.info(QStringLiteral("收到重复启动请求，准备唤起主窗口。"));
            QTimer::singleShot(0, &applicationController, &ApplicationController::showMainWindow);
        });

        engine.rootContext()->setContextProperty(QStringLiteral("appController"), &applicationController);
        engine.rootContext()->setContextProperty(QStringLiteral("settingsViewModel"), &settingsViewModel);
        engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));

        if (engine.rootObjects().isEmpty()) {
            appLogger.error(QStringLiteral("QML 加载失败，应用启动终止。"));
        } else {
            applicationController.attachRootWindow(engine.rootObjects().constFirst());
            if (singleInstanceService.consumePendingActivationRequest()) {
                appLogger.info(QStringLiteral("检测到启动阶段的唤起请求，准备显示主窗口。"));
                QTimer::singleShot(0, &applicationController, &ApplicationController::showMainWindow);
            }

            const AppSettings appSettings = settingsRepository.load();
            if (appSettings.restoreLastMode && !appSettings.lastMode.trimmed().isEmpty()) {
                appLogger.info(QStringLiteral("恢复上次模式：%1。").arg(appSettings.lastMode));
                QTimer::singleShot(0, &modeController, [&modeController, appSettings]() {
                    modeController.switchToMode(appSettings.lastMode);
                });
            }

            const bool startMinimized = app.arguments().contains(QStringLiteral("--minimized"));
            if (!startMinimized) {
                QTimer::singleShot(0, &applicationController, &ApplicationController::showMainWindow);
            } else {
                appLogger.info(QStringLiteral("检测到 --minimized，应用启动后保持托盘驻留。"));
            }
            appLogger.info(QStringLiteral("QML 主窗口已加载。"));
            exitCode = app.exec();
        }
    }

    LoggerService::shutdownSingleton();
    return exitCode;
}
