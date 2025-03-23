#include <QGuiApplication>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QFile>
#include <QtQml>
#include "MediaPlayer.h"
#include "VoxManager.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    VoxManager *voxManager = new VoxManager();
    QQmlApplicationEngine engine;
    QQmlContext *context = engine.rootContext();

    context->setContextProperty("vox_manager", voxManager);

    qmlRegisterSingletonType(QUrl("file:Theme.qml"), "Themes", 1, 0, "Theme");

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("vox_editor", "Main");

    return app.exec();
}

