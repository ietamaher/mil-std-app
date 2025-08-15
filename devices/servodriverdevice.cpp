#include "servodriverdevice.h"
#include "communication/modbustransport.h"
#include "protocols/modbusprotocolparser.h"
#include "protocols/ServoMessage.h"
#include <QJsonObject>
#include <QDebug>
#include <QModbusDataUnit>
#include <QModbusReply>

ServoDriverDevice::ServoDriverDevice(QObject* parent)
    : TemplatedDevice<ServoDriverData>(parent),
    m_transport(nullptr),
    m_parser(nullptr),
    m_pollTimer(new QTimer(this)),
    m_pendingReads(0)
{
    connect(m_pollTimer, &QTimer::timeout, this, &ServoDriverDevice::pollTimerTimeout);
}

ServoDriverDevice::~ServoDriverDevice() {
    shutdown();
}

IDevice::DeviceType ServoDriverDevice::type() const {
    return DeviceType::ServoDriver;
}

void ServoDriverDevice::setDependencies(Transport* transport, ProtocolParser* parser)
{
    m_transport = transport;
    m_parser = parser;
    m_transport->setParent(this);
    m_parser->setParent(this);
}

bool ServoDriverDevice::initialize() {
    setState(DeviceState::Initializing);

    if (!m_transport || !m_parser) {
        qCritical() << objectName() << "missing dependencies!";
        setState(DeviceState::Error);
        return false;
    }

    QJsonObject config = property("config").toJsonObject();
    int pollInterval = config["pollIntervalMs"].toInt(100);

    qDebug() << objectName() << "initializing with poll interval:" << pollInterval << "ms";

    if (m_transport && m_transport->open(config)) {
        setState(DeviceState::Online);
        m_pollTimer->start(pollInterval);
        qDebug() << objectName() << "initialized successfully";
        return true;
    }

    qCritical() << objectName() << "failed to initialize transport";
    setState(DeviceState::Error);
    return false;
}

void ServoDriverDevice::shutdown() {
    m_pollTimer->stop();
    if (m_transport) QMetaObject::invokeMethod(m_transport, "close", Qt::QueuedConnection);
    setState(DeviceState::Offline);
}

void ServoDriverDevice::pollTimerTimeout() {
    if (m_pendingReads > 0) {
        qWarning() << objectName() << "Skipping poll cycle, previous reads still pending.";
        return;
    }
    m_pendingReads = 2;
    sendReadRequest(ServoRegisters::POSITION_START_ADDR, ServoRegisters::POSITION_REG_COUNT);
    sendReadRequest(ServoRegisters::TEMPERATURE_START_ADDR, ServoRegisters::TEMPERATURE_REG_COUNT);
}

void ServoDriverDevice::sendReadRequest(int startAddress, int count) {
    if (state() != DeviceState::Online || !m_transport) {
        m_pendingReads--;
        return;
    }
    auto transport = static_cast<ModbusTransport*>(m_transport);
    QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters, startAddress, count);

    if (auto* reply = transport->sendReadRequest(readUnit)) {
        connect(reply, &QModbusReply::finished, this, &ServoDriverDevice::onReadReplyFinished);
    } else {
        m_pendingReads--;
        qWarning() << objectName() << "Failed to send read request for address" << startAddress;
    }
}

void ServoDriverDevice::onReadReplyFinished() {
    m_pendingReads--;
    auto* reply = qobject_cast<QModbusReply*>(sender());
    if (!reply) return;

    if (reply->error() != QModbusDevice::NoError) {
        qWarning() << objectName() << "Modbus reply error:" << reply->errorString();
    } else {
        auto messages = m_parser->parse(reply);
        for (const auto& msg : messages) {
            if (msg) processMessage(*msg);
        }
    }
    reply->deleteLater();
}

void ServoDriverDevice::processMessage(const Message& message) {
    auto currentData = data();
    auto newData = std::make_shared<ServoDriverData>(*currentData);
    newData->isConnected = true;
    bool dataChanged = false;

    if (message.typeId() == Message::Type::ServoDataType) {
        const auto* servoMsg = static_cast<const ServoDataMessage*>(&message);
        const ServoDriverData& partialData = servoMsg->data();

        // This merging logic assumes the parser sends partial data.
        // We check if the parsed value is different from the default AND different from current.
        if (partialData.position != 0.0f && partialData.position != currentData->position) {
            newData->position = partialData.position;
            dataChanged = true;
        }
        if (partialData.driverTemp != 0.0f && partialData.driverTemp != currentData->driverTemp) {
            newData->driverTemp = partialData.driverTemp;
            dataChanged = true;
        }
        if (partialData.motorTemp != 0.0f && partialData.motorTemp != currentData->motorTemp) {
            newData->motorTemp = partialData.motorTemp;
            dataChanged = true;
        }
    } else if (message.typeId() == Message::Type::ServoAlarmType) {
        const auto* alarmMsg = static_cast<const ServoAlarmMessage*>(&message);
        qWarning() << objectName() << "ALARM:" << alarmMsg->alarmCode() << alarmMsg->description();
        emit alarmDetected(alarmMsg->alarmCode(), alarmMsg->description());
        newData->fault = true;
        dataChanged = true;
    }

    if (dataChanged || newData->isConnected != currentData->isConnected) {
        updateData(newData);
        emit servoDataChanged(*newData);
    }
}

void ServoDriverDevice::writePosition(float position) {
    if (state() != DeviceState::Online || !m_transport) {
        qWarning() << objectName() << "cannot write position - device not online or transport missing";
        return;
    }
    auto* modbusParser = static_cast<ModbusProtocolParser*>(m_parser);
    QModbusDataUnit writeUnit = modbusParser->createWritePositionRequest(position);
    sendWriteRequest(writeUnit);
}

void ServoDriverDevice::sendWriteRequest(const QModbusDataUnit& writeUnit) {
    auto transport = static_cast<ModbusTransport*>(m_transport);
    if (auto* reply = transport->sendWriteRequest(writeUnit)) {
        connect(reply, &QModbusReply::finished, this, &ServoDriverDevice::onWriteReplyFinished);
    } else {
        qWarning() << objectName() << "Failed to create write position request";
    }
}

void ServoDriverDevice::onWriteReplyFinished() {
    auto* reply = qobject_cast<QModbusReply*>(sender());
    if (!reply) return;

    if (reply->error() != QModbusDevice::NoError) {
        qWarning() << objectName() << "Write position error:" << reply->errorString();
    } else {
        qDebug() << objectName() << "Successfully wrote position.";
    }
    reply->deleteLater();
}
