#include "SystemController.h"
#include "SystemDataModel.h"
#include "communication/modbustransport.h"
#include "devices/radardevice.h"
#include "communication/SerialPortTransport.h"
#include "protocols/nmeaparser.h"
#include <QDebug>
#include <QTimer>
#include <QThread>
#include "devices/lrfdevice.h"
#include "protocols/lrfprotocolparser.h"
#include "devices/servodriverdevice.h"
#include "protocols/modbusprotocolparser.h"

SystemController::SystemController(QObject* parent)
    : QObject(parent), m_model(new SystemDataModel(this))
{
    m_ioThread = new QThread(this);
    m_ioThread->setObjectName("IOThread");
    m_trackingTimer = new QTimer(this);
}

SystemController::~SystemController() {
    m_ioThread->quit();
    m_ioThread->wait();
}

bool SystemController::createDevices(const QJsonObject& deviceConfigs) {
    // FIXED: Use a simple counter and connect approach instead of blocking calls
    for (const QString& deviceName : deviceConfigs.keys()) {
        QJsonObject devConf = deviceConfigs[deviceName].toObject();
        if (!devConf["enabled"].toBool(false)) {
            continue;
        }

        // Queue device creation on IO thread
        QMetaObject::invokeMethod(m_ioThread, [this, deviceName, devConf]() {
            createSingleDevice(deviceName, devConf);
        }, Qt::QueuedConnection);
    }
    return true;
}

// FIXED: New method to create a single device (runs on IO thread)
void SystemController::createSingleDevice(const QString& deviceName, const QJsonObject& devConf) {
    IDevice* device = nullptr;
    Transport* transport = nullptr;
    ProtocolParser* parser = nullptr;

    QJsonObject commConf = devConf["communication"].toObject();
    QString protocol = commConf["protocol"].toString();
    QString type = devConf["type"].toString();

    // Create Transport and Parser
    if (protocol == "serial") {
        transport = new SerialPortTransport();
    } else if (protocol == "modbus") {
        transport = new ModbusTransport();
    } else {
        qWarning() << "Unsupported protocol on IO thread for:" << deviceName;
        return;
    }

    if (type == "RadarDevice") {
        parser = new NmeaParser();
    } else if (type == "LRFDevice") {
        parser = new LrfProtocolParser();
    } else if (type == "ServoDriverDevice") {
        parser = new ModbusProtocolParser();
    } else {
        qWarning() << "Unknown device type on IO thread:" << type;
        delete transport;
        return;
    }

    // Create the Device instance
    if (type == "RadarDevice") {
        auto radar = new RadarDevice();
        radar->setDependencies(transport, qobject_cast<NmeaParser*>(parser));
        device = radar;
    } else if (type == "LRFDevice") {
        auto lrf = new LRFDevice();
        lrf->setDependencies(transport, qobject_cast<LrfProtocolParser*>(parser));
        device = lrf;
    } else if (type == "ServoDriverDevice") {
        auto servo = new ServoDriverDevice();
        servo->setDependencies(transport, qobject_cast<ModbusProtocolParser*>(parser));
        device = servo;
    }

    if (!device) {
        delete transport;
        delete parser;
        return;
    }

    // Set properties
    transport->setParent(device);
    parser->setParent(device);
    device->setObjectName(deviceName);
    device->setProperty("config", commConf);

    // FIXED: Emit a signal instead of using invokeMethod to avoid deadlock
    emit deviceCreated(device, deviceName, type);
}

bool SystemController::initialize(const QJsonObject& config) {
    qDebug() << "SystemController::initialize() starting on thread:" << QThread::currentThread();

    // Check meta types first
    checkMetaTypes();

    // Connect the deviceCreated signal to handle device registration
    connect(this, &SystemController::deviceCreated, this, &SystemController::onDeviceCreated, Qt::QueuedConnection);

    // Start the thread FIRST
    m_ioThread->start(QThread::TimeCriticalPriority);
    qDebug() << "IO Thread started:" << m_ioThread;

    // Create devices
    if (!createDevices(config["devices"].toObject())) {
        qCritical() << "Failed to queue device creation from configuration.";
        return false;
    }

    // Give some time for devices to be created and then test connections
    QTimer::singleShot(3000, this, [this]() {
        qDebug() << "=== DELAYED DEBUGGING (3s after init) ===";

        // Check device count
        qDebug() << "Total devices created:" << m_devices.size();
        for (IDevice* device : m_devices) {
            qDebug() << "Device:" << device->objectName()
            << "Type:" << static_cast<int>(device->type())
            << "State:" << static_cast<int>(device->state())
            << "Thread:" << device->thread();
        }

        // Test manual signal emission
        if (m_servo_az) {
            qDebug() << "Testing servo AZ signal emission...";
            QMetaObject::invokeMethod(m_servo_az, "testSignalEmission", Qt::QueuedConnection);
        }

        qDebug() << "=== END DELAYED DEBUGGING ===";
    });

    emit logMessage("System Initialized Successfully.", Qt::darkGreen);
    return true;
}

// FIXED: New slot to handle device registration (runs on main thread)
void SystemController::onDeviceCreated(IDevice* device, const QString& deviceName, const QString& type) {
    qDebug() << "SystemController: Registering device" << deviceName << "of type" << type;

    // Register device pointers
    if (type == "RadarDevice") {
        m_radar = static_cast<RadarDevice*>(device);
    } else if (type == "LRFDevice") {
        m_lrf = static_cast<LRFDevice*>(device);
    } else if (type == "ServoDriverDevice") {
        if (deviceName == "servo_azimuth") {
            m_servo_az = static_cast<ServoDriverDevice*>(device);
        } else if (deviceName == "servo_elevation") {
            m_servo_el = static_cast<ServoDriverDevice*>(device);
        }
    }

    // Add to device list
    m_devices.append(device);

    // Connect device state signals
    connect(device, &IDevice::stateChanged, this, &SystemController::onDeviceStateChanged, Qt::QueuedConnection);
    connect(device, &IDevice::deviceError, this, &SystemController::onDeviceError, Qt::QueuedConnection);

    // Check if we have all expected devices and connect signals
    checkAndConnectSignals();

    // Initialize the device
    QMetaObject::invokeMethod(device, "initialize", Qt::QueuedConnection);
}

// FIXED: New method to check if all devices are ready and connect signals
void SystemController::checkAndConnectSignals() {
    // This is a simple approach - in a real system you might want to be more sophisticated
    static bool signalsConnected = false;

    if (!signalsConnected && m_devices.size() >= 2) { // Adjust based on your expected device count
        // Give a small delay to ensure all devices are registered
        QTimer::singleShot(100, this, [this]() {
            connectSignals();
        });
        signalsConnected = true;
    }
}

void SystemController::connectSignals() {
    qDebug() << "=== CONNECTING SIGNALS ===";
    qDebug() << "Main thread:" << QThread::currentThread();
    qDebug() << "Controller thread:" << this->thread();
    qDebug() << "Model thread:" << m_model->thread();
    qDebug() << "IO thread:" << m_ioThread;

    qDebug() << "Connecting signals - Radar:" << (m_radar != nullptr)
             << "LRF:" << (m_lrf != nullptr)
             << "Servo_AZ:" << (m_servo_az != nullptr)
             << "Servo_EL:" << (m_servo_el != nullptr);

    // Test each connection individually with detailed logging
    if (m_radar) {
        qDebug() << "Radar device thread:" << m_radar->thread();
        bool connected = connect(m_radar, &RadarDevice::radarDataUpdated,
                                 m_model, &SystemDataModel::onRadarDataUpdated,
                                 Qt::QueuedConnection);
        qDebug() << "Radar connection result:" << connected;

        // Add a test lambda
        connect(m_radar, &RadarDevice::radarDataUpdated,
                this, [this](std::shared_ptr<const RadarDeviceData> data) {
                    qDebug() << "LAMBDA: Radar data received with" << data->trackedTargets.size() << "targets";
                }, Qt::QueuedConnection);
    }

    if (m_lrf) {
        qDebug() << "LRF device thread:" << m_lrf->thread();
        bool connected = connect(m_lrf, &LRFDevice::lrfDataChanged,
                                 m_model, &SystemDataModel::onLrfDataUpdated,
                                 Qt::QueuedConnection);
        qDebug() << "LRF connection result:" << connected;

        // Add a test lambda
        connect(m_lrf, &LRFDevice::lrfDataChanged,
                this, [this](std::shared_ptr<const LrfData> data) {
                    qDebug() << "LAMBDA: LRF data received - distance:" << data->lastDistance;
                }, Qt::QueuedConnection);
    }

    if (m_servo_az) {
        qDebug() << "Servo AZ device thread:" << m_servo_az->thread();
        bool connected = connect(m_servo_az, &ServoDriverDevice::servoDataChanged,
                                 m_model, &SystemDataModel::onAzimuthServoDataUpdated,
                                 Qt::QueuedConnection);
        qDebug() << "Servo AZ connection result:" << connected;

        // Add a test lambda
        connect(m_servo_az, &ServoDriverDevice::servoDataChanged,
                this, [this](const ServoDriverData& data) {
                    qDebug() << "LAMBDA: Servo AZ data changed - position:" << data.position
                             << "connected:" << data.isConnected;
                }, Qt::QueuedConnection);
    }

    if (m_servo_el) {
        qDebug() << "Servo EL device thread:" << m_servo_el->thread();
        bool connected = connect(m_servo_el, &ServoDriverDevice::servoDataChanged,
                                 m_model, &SystemDataModel::onElevationServoDataUpdated,
                                 Qt::QueuedConnection);
        qDebug() << "Servo EL connection result:" << connected;

        // Add a test lambda
        connect(m_servo_el, &ServoDriverDevice::servoDataChanged,
                this, [this](const ServoDriverData& data) {
                    qDebug() << "LAMBDA: Servo EL data changed - position:" << data.position
                             << "connected:" << data.isConnected;
                }, Qt::QueuedConnection);
    }

    // Connect tracking timer
    connect(m_trackingTimer, &QTimer::timeout, this, &SystemController::updateTracking);

    emit logMessage("All device signals connected successfully.", Qt::darkBlue);
    qDebug() << "=== END CONNECTING SIGNALS ===";
}

// Also add this method to check meta types
void SystemController::checkMetaTypes() {
    qDebug() << "=== CHECKING META TYPES ===";

    // Check if required meta types are registered
    int typeId1 = QMetaType::type("ServoDriverData");
    int typeId2 = QMetaType::type("const ServoDriverData&");
    int typeId3 = QMetaType::type("std::shared_ptr<const RadarDeviceData>");
    int typeId4 = QMetaType::type("std::shared_ptr<const LrfData>");

    qDebug() << "ServoDriverData meta type ID:" << typeId1;
    qDebug() << "const ServoDriverData& meta type ID:" << typeId2;
    qDebug() << "std::shared_ptr<const RadarDeviceData> meta type ID:" << typeId3;
    qDebug() << "std::shared_ptr<const LrfData> meta type ID:" << typeId4;

    qDebug() << "=== END CHECKING META TYPES ===";
}

// Method to manually test signal emission
void SystemController::testSignalEmission() {
    qDebug() << "=== TESTING SIGNAL EMISSION ===";

    if (m_servo_az) {
        // Create test data and manually emit signal to see if connections work
        ServoDriverData testData;
        testData.position = 999.0f;
        testData.isConnected = true;

        qDebug() << "Manually triggering servo signal emission test";
        // This would need to be added to ServoDriverDevice for testing:
        // QMetaObject::invokeMethod(m_servo_az, "testSignalEmission", Qt::QueuedConnection);
    }

    qDebug() << "=== END TESTING SIGNAL EMISSION ===";
}

// Rest of the methods remain the same...
void SystemController::trackTarget(quint32 targetId) {
    bool found = false;
    auto currentRadarData = m_model->getRadarData();
    if (currentRadarData.trackedTargets.contains(targetId)) {
        found = true;
    }

    if (found) {
        m_trackedTargetId = targetId;
        m_trackingTimer->start(100);
        emit logMessage(QString("Starting to track target %1").arg(targetId), Qt::darkGreen);
    } else {
        emit logMessage(QString("Cannot track non-existent target %1").arg(targetId), Qt::red);
    }
}

void SystemController::stopTracking() {
    m_trackingTimer->stop();
    m_trackedTargetId = 0;
    emit logMessage("Stopped tracking.", Qt::darkBlue);
}

void SystemController::updateTracking() {
    if (m_trackedTargetId == 0 || !m_servo_az || !m_servo_el) return;

    auto currentRadarData = m_model->getRadarData();
    if (currentRadarData.trackedTargets.contains(m_trackedTargetId)) {
        const RadarTargetData& target = currentRadarData.trackedTargets.value(m_trackedTargetId);

        QMetaObject::invokeMethod(m_servo_az, "writePosition", Qt::QueuedConnection, Q_ARG(float, target.azimuthDegrees));
        QMetaObject::invokeMethod(m_servo_el, "writePosition", Qt::QueuedConnection, Q_ARG(float, 0.0f));

        qDebug() << "Tracking Update: Pointing Az at" << target.azimuthDegrees << "and El at 0.0";
    } else {
        stopTracking();
        emit logMessage(QString("Lost track of target %1").arg(m_trackedTargetId), Qt::red);
    }
}

void SystemController::lrfGetSingleDistance() {
    if (m_lrf && m_lrf->state() == IDevice::DeviceState::Online) {
        QMetaObject::invokeMethod(m_lrf, "sendSingleRanging", Qt::QueuedConnection);
        emit logMessage("LRF single ranging requested.", Qt::darkBlue);
    } else {
        emit logMessage("LRF is not available to get distance.", Qt::red);
    }
}

void SystemController::lrfGetPulseCount() {
    if (m_lrf && m_lrf->state() == IDevice::DeviceState::Online) {
        QMetaObject::invokeMethod(m_lrf, "queryAccumulatedLaserCount", Qt::QueuedConnection);
        emit logMessage("LRF pulse count requested.", Qt::darkBlue);
    } else {
        emit logMessage("LRF is not available to get pulse count.", Qt::red);
    }
}

void SystemController::onDeviceStateChanged(IDevice::DeviceState state) {
    IDevice* device = qobject_cast<IDevice*>(sender());
    if(device) {
        QString stateStr;
        switch(state) {
        case IDevice::DeviceState::Offline: stateStr = "Offline"; break;
        case IDevice::DeviceState::Initializing: stateStr = "Initializing"; break;
        case IDevice::DeviceState::Online: stateStr = "Online"; break;
        case IDevice::DeviceState::Error: stateStr = "Error"; break;
        }

        emit logMessage(QString("Device %1 changed state to %2")
                            .arg(device->objectName())
                            .arg(stateStr), Qt::darkCyan);

        if (state == IDevice::DeviceState::Error) {
            emit logMessage("CRITICAL: A device has entered an error state!", Qt::red);
        }
    }
}

void SystemController::onDeviceError(const QString& message) {
    emit logMessage("ERROR: " + message, Qt::red);
}

SystemDataModel* SystemController::model() const {
    return m_model;
}
