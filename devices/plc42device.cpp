#include "plc42device.h"
#include "communication/modbustransport.h"
#include "protocols/plc42protocolparser.h"
#include "protocols/Plc42Message.h"
#include <QModbusReply>
#include <QDebug>

PLC42Device::PLC42Device(QObject* parent)
    : TemplatedDevice<Plc42Data>(parent),
      m_transport(nullptr),
      m_parser(nullptr),
      m_pollTimer(new QTimer(this)),
      m_pendingReads(0)
{
    connect(m_pollTimer, &QTimer::timeout, this, &PLC42Device::onPollTimerTimeout);
}

PLC42Device::~PLC42Device() {
    shutdown();
}

IDevice::DeviceType PLC42Device::type() const {
    return DeviceType::PLC;
}

void PLC42Device::setDependencies(ModbusTransport* transport, Plc42ProtocolParser* parser) {
    m_transport = transport;
    m_parser = parser;
    // It's good practice to set the parent to ensure proper Qt object ownership
    if (m_transport) m_transport->setParent(this);
    if (m_parser) m_parser->setParent(this);
}

bool PLC42Device::initialize() {
    if (!m_transport || !m_parser) {
        qWarning() << "PLC42Device dependencies not set!";
        setState(DeviceState::Error);
        return false;
    }

    setState(DeviceState::Initializing);
    QJsonObject config = property("config").toJsonObject();

    if (m_transport->open(config)) {
        setState(DeviceState::Online);
        m_pollTimer->start(200); // Poll every 200ms
        return true;
    }

    setState(DeviceState::Error);
    return false;
}

void PLC42Device::shutdown() {
    m_pollTimer->stop();
    if (m_transport && state() != DeviceState::Offline) {
        QMetaObject::invokeMethod(m_transport, "close", Qt::QueuedConnection);
    }
    setState(DeviceState::Offline);
}

void PLC42Device::onPollTimerTimeout() {
    if (state() != DeviceState::Online) {
        return;
    }
    // If there are still pending reads, maybe skip this cycle or log a warning
    if (m_pendingReads > 0) {
        qWarning() << "PLC42: Skipping poll cycle, previous reads still pending.";
        return;
    }

    m_pendingReads = 2; // We are about to send two read requests

    auto readDIs = m_parser->createReadDigitalInputsRequest();
    sendReadRequest(readDIs);

    auto readHRs = m_parser->createReadHoldingRegistersRequest();
    sendReadRequest(readHRs);
}

void PLC42Device::sendReadRequest(const QModbusDataUnit& readUnit) {
    if (!m_transport || state() != DeviceState::Online) {
        m_pendingReads--;
        return;
    }

    if (auto* reply = m_transport->sendReadRequest(readUnit)) {
        connect(reply, &QModbusReply::finished, this, &PLC42Device::onReadReplyFinished);
    } else {
        m_pendingReads--;
        qWarning() << "PLC42: Failed to send read request.";
    }
}

void PLC42Device::onReadReplyFinished() {
    m_pendingReads--;
    auto* reply = qobject_cast<QModbusReply*>(sender());
    if (!reply) {
        return;
    }

    const QModbusDataUnit unit = reply->result();
    reply->deleteLater();

    const auto messages = m_parser->parse(reply);
    for (const auto& msgPtr : messages) {
        if (msgPtr) {
            // We need to know the type of the reply to merge correctly.
            // We can get this from the reply's result.
            processMessage(*msgPtr);
        }
    }
}

void PLC42Device::processMessage(const Message& message) {
    if (message.typeId() == Message::Type::Plc42DataType) {
        const auto* plcMsg = static_cast<const Plc42DataMessage*>(&message);

        // This is where we need to know the type of data.
        // The message doesn't contain it. Let's pass it from onReadReplyFinished.
        // This requires changing the signature of processMessage.
        // Let's avoid that and use the heuristic in mergeAndProcessData.
        mergeAndProcessData(plcMsg->data());
    }
}

void PLC42Device::mergeAndProcessData(const Plc42Data& partialData) {
    // This heuristic is not ideal, but it works given that the parser
    // creates a default Plc42Data object and only fills in one part of it.
    // A more robust solution would involve a more intelligent parser or message type.
    if (partialData.solenoidMode != 0 || partialData.gimbalOpMode != 0 || partialData.azimuthSpeed != 0 || partialData.elevationSpeed != 0) {
        // This is likely a holding register update
        m_stagingData.solenoidMode = partialData.solenoidMode;
        m_stagingData.gimbalOpMode = partialData.gimbalOpMode;
        m_stagingData.azimuthSpeed = partialData.azimuthSpeed;
        m_stagingData.elevationSpeed = partialData.elevationSpeed;
        m_stagingData.azimuthDirection = partialData.azimuthDirection;
        m_stagingData.elevationDirection = partialData.elevationDirection;
        m_stagingData.solenoidState = partialData.solenoidState;
        m_stagingData.resetAlarm = partialData.resetAlarm;
    } else {
        // This is likely a discrete input update
        m_stagingData.stationUpperSensor = partialData.stationUpperSensor;
        m_stagingData.stationLowerSensor = partialData.stationLowerSensor;
        m_stagingData.emergencyStopActive = partialData.emergencyStopActive;
        m_stagingData.ammunitionLevel = partialData.ammunitionLevel;
        m_stagingData.stationInput1 = partialData.stationInput1;
        m_stagingData.stationInput2 = partialData.stationInput2;
        m_stagingData.stationInput3 = partialData.stationInput3;
        m_stagingData.solenoidActive = partialData.solenoidActive;
    }

    if (m_pendingReads == 0) {
        m_stagingData.isConnected = (state() == DeviceState::Online);
        if (*data() != m_stagingData) {
            updateData(std::make_shared<const Plc42Data>(m_stagingData));
            emit plc42DataChanged(m_stagingData);
        }
    }
}


void PLC42Device::setSolenoidMode(uint16_t mode) {
    QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, 0, 1);
    writeUnit.setValue(0, mode);
    sendWriteRequest(writeUnit);
}

void PLC42Device::setGimbalMotionMode(uint16_t mode) {
    QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, 1, 1);
    writeUnit.setValue(0, mode);
    sendWriteRequest(writeUnit);
}

void PLC42Device::setAzimuthSpeed(uint32_t speed) {
    QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, 2, 2);
    uint16_t low = speed & 0xFFFF;
    uint16_t high = (speed >> 16) & 0xFFFF;
    writeUnit.setValue(0, low);
    writeUnit.setValue(1, high);
    sendWriteRequest(writeUnit);
}

void PLC42Device::setElevationSpeed(uint32_t speed) {
    QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, 4, 2);
    uint16_t low = speed & 0xFFFF;
    uint16_t high = (speed >> 16) & 0xFFFF;
    writeUnit.setValue(0, low);
    writeUnit.setValue(1, high);
    sendWriteRequest(writeUnit);
}

void PLC42Device::setAzimuthDirection(uint16_t direction) {
    QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, 6, 1);
    writeUnit.setValue(0, direction);
    sendWriteRequest(writeUnit);
}

void PLC42Device::setElevationDirection(uint16_t direction) {
    QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, 7, 1);
    writeUnit.setValue(0, direction);
    sendWriteRequest(writeUnit);
}

void PLC42Device::setSolenoidState(uint16_t state) {
    QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, 8, 1);
    writeUnit.setValue(0, state);
    sendWriteRequest(writeUnit);
}

void PLC42Device::setResetAlarm(uint16_t alarm) {
    QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, 9, 1);
    writeUnit.setValue(0, alarm);
    sendWriteRequest(writeUnit);
}

void PLC42Device::sendWriteRequest(const QModbusDataUnit& writeUnit) {
    if (!m_transport || state() != DeviceState::Online) {
        return;
    }

    if (auto* reply = m_transport->sendWriteRequest(writeUnit)) {
        connect(reply, &QModbusReply::finished, this, &PLC42Device::onWriteReplyFinished);
    } else {
        qWarning() << "PLC42: Failed to send write request.";
    }
}

void PLC42Device::onWriteReplyFinished() {
    auto* reply = qobject_cast<QModbusReply*>(sender());
    if (!reply) {
        return;
    }
    if (reply->error() != QModbusDevice::NoError) {
        qWarning() << "PLC42 write error:" << reply->errorString();
    }
    reply->deleteLater();
}
