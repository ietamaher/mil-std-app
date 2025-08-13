#include "mainwindow.h"
#include "systemcontroller.h"
#include "data/DataTypes.h"
#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <memory>

// Function to load the configuration from a file
QJsonObject loadConfig(const QString& path)
{
    QFile configFile(path);
    if (!configFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open config file:" << path;
        return QJsonObject();
    }
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse config file:" << parseError.errorString();
        return QJsonObject();
    }
    return doc.object();
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Register all custom types used in cross-thread signals and slots.
    qRegisterMetaType<std::shared_ptr<const RadarDeviceData>>("std::shared_ptr<const RadarDeviceData>");
    qRegisterMetaType<std::shared_ptr<const LrfData>>("std::shared_ptr<const LrfData>");
    qRegisterMetaType<ServoDriverData>("ServoDriverData");
    qRegisterMetaType<ServoDriverData>("const ServoDriverData&");

    // 1. Load configuration from external file
    QJsonObject config = loadConfig("config/system_config.json");
    if (config.isEmpty()) {
        qCritical() << "System cannot start without a valid configuration.";
        return -1;
    }

    // 2. Create the core System Controller
    SystemController controller;

    // 3. Create the MainWindow and give it the controller
    MainWindow w(&controller);
    w.show();

    // 4. Initialize the controller AFTER the UI is shown.
    //    This allows the UI to receive all the startup log messages.
    if (!controller.initialize(config)) {
        qCritical() << "System controller failed to initialize. Shutting down.";
        // Optionally show a critical message box in the UI
        w.onLogMessage("FATAL: System controller failed to initialize.", Qt::red);
    }

    return a.exec();
}
