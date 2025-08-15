#include "systemcontroller.h"
#include "systemdatamodel.h"
#include "communication/modbustransport.h"
#include "devices/radardevice.h"
#include "communication/serialporttransport.h"
#include "protocols/nmeaparser.h"
#include <QDebug>
#include <QTimer>
#include <QThread>
#include "devices/lrfdevice.h"
#include "protocols/lrfprotocolparser.h"
#include "devices/servodriverdevice.h"
#include "protocols/modbusprotocolparser.h"
#include "devices/plc21device.h"
#include "protocols/plc21protocolparser.h"
#include "devices/plc42device.h"
#include "protocols/plc42protocolparser.h"
#include "devices/cameravideostreamdevice.h"
#include "devices/gyrodevice.h"
#include "protocols/gyroprotocolparser.h"

SystemController::SystemController(QObject* parent)
    : QObject(parent), m_model(new SystemDataModel(this))
{
    m_ioThread = new QThread(this);
    m_ioThread->setObjectName("IOThread");
    m_trackingTimer = new QTimer(this);
}

SystemController::~SystemController() {
    if (m_dayProcessor) m_dayProcessor->stop();
    if (m_nightProcessor) m_nightProcessor->stop();
    m_ioThread->quit();
    m_ioThread->wait();
}

bool SystemController::createDevices(const QJsonObject& deviceConfigs) {
    for (const QString& deviceName : deviceConfigs.keys()) {
        QJsonObject devConf = deviceConfigs[deviceName].toObject();
        if (!devConf["enabled"].toBool(false)) {
            continue;
        }

        QMetaObject::invokeMethod(m_ioThread, [this, deviceName, devConf]() {
            createSingleDevice(deviceName, devConf);
        }, Qt::QueuedConnection);
    }
    return true;
}

void SystemController::createSingleDevice(const QString& deviceName, const QJsonObject& devConf) {
    IDevice* device = nullptr;
    Transport* transport = nullptr;
    ProtocolParser* parser = nullptr;

    QJsonObject commConf = devConf["communication"].toObject();
    QString protocol = commConf["protocol"].toString();
    QString type = devConf["type"].toString();

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
    } else if (type == "Plc21Device") {
        parser = new Plc21ProtocolParser();
    } else if (type == "PLC42Device") {
        parser = new Plc42ProtocolParser();
    } else if (type == "GyroDevice") {
        parser = new GyroProtocolParser();
    } else {
        qWarning() << "Unknown device type on IO thread:" << type;
        delete transport;
        return;
    }

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
    } else if (type == "Plc21Device") {
        auto plc = new Plc21Device();
        plc->setDependencies(transport, qobject_cast<Plc21ProtocolParser*>(parser));
        device = plc;
    } else if (type == "PLC42Device") {
        auto plc = new PLC42Device();
        plc->setDependencies(static_cast<ModbusTransport*>(transport), static_cast<Plc42ProtocolParser*>(parser));
        device = plc;
    } else if (type == "GyroDevice") {
        auto gyro = new GyroDevice();
        gyro->setDependencies(static_cast<ModbusTransport*>(transport), static_cast<GyroProtocolParser*>(parser));
        device = gyro;
    }

    if (!device) {
        delete transport;
        delete parser;
        return;
    }

    transport->setParent(device);
    parser->setParent(device);
    device->setObjectName(deviceName);
    device->setProperty("config", commConf);

    emit deviceCreated(device, deviceName, type);
}

bool SystemController::createCameraDevices(const QJsonObject& cameraConfigs) {
    if (cameraConfigs.contains("day_cam")) {
        QJsonObject conf = cameraConfigs["day_cam"].toObject();
        m_dayProcessor = new CameraVideoStreamDevice(0,
                                                     conf["device"].toString(),
                                                     conf["width"].toInt(),
                                                     conf["height"].toInt(),
                                                     static_cast<SystemStateModel*>(m_model)); // Cast needed
        connect(m_dayProcessor, &CameraVideoStreamDevice::frameDataReady, m_model, &SystemDataModel::onFrameDataReady, Qt::QueuedConnection);
        connect(m_model, &SystemDataModel::systemStateChanged, m_dayProcessor, &CameraVideoStreamDevice::onSystemStateChanged, Qt::QueuedConnection);
    }
    if (cameraConfigs.contains("night_cam")) {
        QJsonObject conf = cameraConfigs["night_cam"].toObject();
        m_nightProcessor = new CameraVideoStreamDevice(1,
                                                       conf["device"].toString(),
                                                       conf["width"].toInt(),
                                                       conf["height"].toInt(),
                                                       static_cast<SystemStateModel*>(m_model)); // Cast needed
        connect(m_nightProcessor, &CameraVideoStreamDevice::frameDataReady, m_model, &SystemDataModel::onFrameDataReady, Qt::QueuedConnection);
        connect(m_model, &SystemDataModel::systemStateChanged, m_nightProcessor, &CameraVideoStreamDevice::onSystemStateChanged, Qt::QueuedConnection);
    }
    return true;
}

bool SystemController::initialize(const QJsonObject& config) {
    qDebug() << "SystemController::initialize() starting on thread:" << QThread::currentThread();
    checkMetaTypes();
    connect(this, &SystemController::deviceCreated, this, &SystemController::onDeviceCreated, Qt::QueuedConnection);
    m_ioThread->start(QThread::TimeCriticalPriority);
    qDebug() << "IO Thread started:" << m_ioThread;
    if (!createDevices(config["devices"].toObject())) {
        qCritical() << "Failed to queue device creation from configuration.";
        return false;
    }
    if (!createCameraDevices(config["cameras"].toObject())) {
        qCritical() << "Failed to create camera devices from configuration.";
        return false;
    }

    if (m_dayProcessor) m_dayProcessor->start();
    if (m_nightProcessor) m_nightProcessor->start();

    emit logMessage("System Initialized Successfully.", Qt::darkGreen);
    return true;
}

void SystemController::onDeviceCreated(IDevice* device, const QString& deviceName, const QString& type) {
    qDebug() << "SystemController: Registering device" << deviceName << "of type" << type;

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
    } else if (type == "Plc21Device") {
        m_plc21 = static_cast<Plc21Device*>(device);
    } else if (type == "PLC42Device") {
        m_plc42 = static_cast<PLC42Device*>(device);
    } else if (type == "GyroDevice") {
        m_gyro = static_cast<GyroDevice*>(device);
    }

    m_devices.append(device);
    connect(device, &IDevice::stateChanged, this, &SystemController::onDeviceStateChanged, Qt::QueuedConnection);
    connect(device, &IDevice::deviceError, this, &SystemController::onDeviceError, Qt::QueuedConnection);
    checkAndConnectSignals();
    QMetaObject::invokeMethod(device, "initialize", Qt::QueuedConnection);
}

void SystemController::checkAndConnectSignals() {
    static bool signalsConnected = false;
    if (!signalsConnected && m_devices.size() >= 6) { // Expect 6 devices now
        QTimer::singleShot(100, this, &SystemController::connectSignals);
        signalsConnected = true;
    }
}

void SystemController::connectSignals() {
    qDebug() << "=== CONNECTING SIGNALS ===";
    if (m_radar) {
        connect(m_radar, &RadarDevice::radarDataUpdated, m_model, &SystemDataModel::onRadarDataUpdated, Qt::QueuedConnection);
    }
    if (m_lrf) {
        connect(m_lrf, &LRFDevice::lrfDataChanged, m_model, &SystemDataModel::onLrfDataUpdated, Qt::QueuedConnection);
    }
    if (m_servo_az) {
        connect(m_servo_az, &ServoDriverDevice::servoDataChanged, m_model, &SystemDataModel::onAzimuthServoDataUpdated, Qt::QueuedConnection);
    }
    if (m_servo_el) {
        connect(m_servo_el, &ServoDriverDevice::servoDataChanged, m_model, &SystemDataModel::onElevationServoDataUpdated, Qt::QueuedConnection);
    }
    if (m_plc21) {
        connect(m_plc21, &Plc21Device::panelDataChanged, m_model, &SystemDataModel::onPlc21DataUpdated, Qt::QueuedConnection);
    }
    if (m_plc42) {
        connect(m_plc42, &PLC42Device::plc42DataChanged, m_model, &SystemDataModel::onPlc42DataUpdated, Qt::QueuedConnection);
    }
    if (m_gyro) {
        connect(m_gyro, &GyroDevice::gyroDataChanged, m_model, &SystemDataModel::onGyroDataUpdated, Qt::QueuedConnection);
    }
    connect(m_trackingTimer, &QTimer::timeout, this, &SystemController::updateTracking);
    emit logMessage("All device signals connected successfully.", Qt::darkBlue);
    qDebug() << "=== END CONNECTING SIGNALS ===";
}

void SystemController::checkMetaTypes() {
    qDebug() << "=== CHECKING META TYPES ===";
    qRegisterMetaType<ServoDriverData>();
    qRegisterMetaType<const ServoDriverData&>();
    qRegisterMetaType<std::shared_ptr<const RadarDeviceData>>();
    qRegisterMetaType<std::shared_ptr<const LrfData>>();
    qRegisterMetaType<Plc21DeviceData>();
    qRegisterMetaType<const Plc21DeviceData&>();
    qRegisterMetaType<Plc42Data>();
    qRegisterMetaType<const Plc42Data&>();
    qRegisterMetaType<FrameData>();
    qRegisterMetaType<const FrameData&>();
    qRegisterMetaType<GyroData>();
    qRegisterMetaType<const GyroData&>();
}

void SystemController::setCameraTracking(int camIndex, bool enabled) {
    CameraVideoStreamDevice* cam = (camIndex == 0) ? m_dayProcessor : m_nightProcessor;
    if (cam) {
        QMetaObject::invokeMethod(cam, "setTrackingEnabled", Qt::QueuedConnection, Q_ARG(bool, enabled));
    }
}

void SystemController::setCameraDetection(int camIndex, bool enabled) {
    CameraVideoStreamDevice* cam = (camIndex == 0) ? m_dayProcessor : m_nightProcessor;
    if (cam) {
        QMetaObject::invokeMethod(cam, "setDetectionEnabled", Qt::QueuedConnection, Q_ARG(bool, enabled));
    }
}

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
        emit logMessage(QString("Device %1 changed state to %2").arg(device->objectName()).arg(stateStr), Qt::darkCyan);
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
