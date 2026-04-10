ROOT_PATH = $$clean_path($$PWD/..)
SRC_PATH = $$clean_path($$ROOT_PATH/src)
BUILD_PATH = $$clean_path($$ROOT_PATH/build)
BIN_PATH = $$clean_path($$ROOT_PATH/bin)
PUBLISH_PATH = $$clean_path($$ROOT_PATH/publish)
ASSETS_PATH = $$clean_path($$SRC_PATH/assets)
PLAN_PATH = $$clean_path($$ROOT_PATH/plan)

QT += core gui network qml quick quickcontrols2 widgets svg

CONFIG += c++14
CONFIG += qt warn_on

include($$SRC_PATH/version/version.pri)

win32-msvc* {
    QMAKE_CFLAGS += /utf-8
    QMAKE_CXXFLAGS += /utf-8
    APP_MANIFEST_PATH = $$clean_path($$SRC_PATH/NetworkSwitcher.exe.manifest)
    APP_MANIFEST_PATH_WIN = $$system_path($$APP_MANIFEST_PATH)
    LIBS += user32.lib iphlpapi.lib ws2_32.lib
}

TEMPLATE = app
VERSION = $${APP_VERSION}
DEFINES += APP_VERSION_STR=\\\"$${APP_VERSION}\\\"

CONFIG(debug, debug|release) {
    BUILD_CONFIG = Debug
    TARGET = NetworkSwitcherd
} else {
    BUILD_CONFIG = Release
    TARGET = NetworkSwitcher
}

DESTDIR = $$BIN_PATH
OBJECTS_DIR = $$clean_path($$BUILD_PATH/$${BUILD_CONFIG}/obj)
MOC_DIR = $$clean_path($$BUILD_PATH/$${BUILD_CONFIG}/moc)
RCC_DIR = $$clean_path($$BUILD_PATH/$${BUILD_CONFIG}/rcc)
UI_DIR = $$clean_path($$BUILD_PATH/$${BUILD_CONFIG}/ui)

SOURCES += \
    app/ApplicationController.cpp \
    core/ModeController.cpp \
    core/SettingsRepository.cpp \
    main.cpp \
    models/AdapterListModel.cpp \
    models/RouteTableModel.cpp \
    services/LoggerService.cpp \
    services/NetworkService.cpp \
    services/OpenVpnService.cpp \
    services/ProcessService.cpp \
    services/SingleInstanceService.cpp \
    services/StartupService.cpp \
    viewmodels/SettingsViewModel.cpp

HEADERS += \
    app/ApplicationController.h \
    core/AppSettings.h \
    core/ModeController.h \
    core/NetworkTypes.h \
    core/SettingsRepository.h \
    models/AdapterListModel.h \
    models/RouteTableModel.h \
    services/LoggerService.h \
    services/NetworkService.h \
    services/OpenVpnService.h \
    services/ProcessService.h \
    services/SingleInstanceService.h \
    services/StartupService.h \
    viewmodels/SettingsViewModel.h

RESOURCES += \
    resources.qrc

INCLUDEPATH += $$SRC_PATH

QML_IMPORT_PATH =
QML_DESIGNER_IMPORT_PATH =

win32 {
    RC_FILE = $$clean_path($$SRC_PATH/version/NetworkSwitcherVersion.rc)
    TARGET_EXE_PATH = $$clean_path($$DESTDIR/$${TARGET}.exe)
    RELEASE_EXE_PATH = $$clean_path($$BIN_PATH/NetworkSwitcher.exe)
    PUBLISH_PATH_WIN = $$system_path($$PUBLISH_PATH)
    PUBLISH_EXE_PATH = $$clean_path($$PUBLISH_PATH/NetworkSwitcher.exe)
    TARGET_EXE_PATH_WIN = $$system_path($$TARGET_EXE_PATH)
    RELEASE_EXE_PATH_WIN = $$system_path($$RELEASE_EXE_PATH)
    PUBLISH_EXE_PATH_WIN = $$system_path($$PUBLISH_EXE_PATH)
}

win32-msvc* {
    QMAKE_POST_LINK += mt.exe -nologo -manifest $$shell_quote($$APP_MANIFEST_PATH_WIN) $$quote(-outputresource:$${TARGET_EXE_PATH_WIN};$${LITERAL_HASH}1) $$escape_expand(\\n\\t)
}

CONFIG(release, debug|release) {
    QMAKE_POST_LINK += $(CHK_DIR_EXISTS) $$shell_quote($$PUBLISH_PATH_WIN) $(MKDIR) $$shell_quote($$PUBLISH_PATH_WIN) $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $(COPY_FILE) $$shell_quote($$RELEASE_EXE_PATH_WIN) $$shell_quote($$PUBLISH_EXE_PATH_WIN) $$escape_expand(\\n\\t)
}
