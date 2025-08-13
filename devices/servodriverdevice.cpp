#include "servodriverdevice.h"
#include "communication/modbustransport.h"
#include "protocols/modbusprotocolparser.h"
#include "protocols/ServoMessage.h"
#include <QJsonObject>
#include <QDebug>

ServoDriverDevice::ServoDriverDevice(QObject* parent)
    : TemplatedDevice<ServoDriverData>(parent),
    m_transport(nullptr),
    m_parser(nullptr),
    m_pollTimer(new QTimer(this))
{
    connect(m_pollTimer, &QTimer::timeout, this, &ServoDriverDevice::pollTimerTimeout);
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

    auto modbusTransport = static_cast<ModbusTransport*>(m_transport);
    connect(modbusTransport, &ModbusTransport::modbusReplyReady,
            this, &ServoDriverDevice::onModbusReplyReady);
}

ServoDriverDevice::~ServoDriverDevice() {
    m_pollTimer->stop();
}

bool ServoDriverDevice::initialize() {
    setState(DeviceState::Initializing);

    if (!m_transport || !m_parser) {
        qCritical() << objectName() << "missing dependencies!";
        setState(DeviceState::Error);
        return false;
    }

    QJsonObject config = property("config").toJsonObject();
    // REMOVED: m_slaveId = config["slaveId"].toInt(1); - now handled by transport
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
    // In a real system, you might alternate requests. Here we read both.
    sendReadRequest(ServoRegisters::POSITION_START_ADDR, ServoRegisters::POSITION_REG_COUNT);
    sendReadRequest(ServoRegisters::TEMPERATURE_START_ADDR, ServoRegisters::TEMPERATURE_REG_COUNT);
}

void ServoDriverDevice::sendReadRequest(int startAddress, int count) {
    if (state() != DeviceState::Online || !m_transport) return;

    auto transport = static_cast<ModbusTransport*>(m_transport);
    QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters, startAddress, count);

    // FIXED: Use new interface without slave ID parameter
    if (auto* reply = transport->sendReadRequest(readUnit)) {
        // The transport handles the reply connection internally
        qDebug() << objectName() << "sent read request for address" << startAddress;
    } else {
        qWarning() << objectName() << "Failed to send read request for address" << startAddress;
    }
}

void ServoDriverDevice::processMessage(const Message& message) {
    qDebug() << objectName() << "Processing message type:" << static_cast<int>(message.typeId());

    if (message.typeId() == Message::Type::ServoDataType) {
        auto const* servoMsg = static_cast<const ServoDataMessage*>(&message);

        auto currentData = data(); // Get a copy of the current data state
        auto newData = std::make_shared<ServoDriverData>(*currentData);
        newData->isConnected = true; // We got a message, so we're connected

        // Merge the new partial data into our complete data object
        const ServoDriverData& partialData = servoMsg->data();
        bool dataChanged = false;

        if (partialData.position != 0.0f && partialData.position != currentData->position) {
            newData->position = partialData.position;
            dataChanged = true;
            qDebug() << objectName() << "updated position to" << partialData.position;
        }
        if (partialData.driverTemp != 0.0f && partialData.driverTemp != currentData->driverTemp) {
            newData->driverTemp = partialData.driverTemp;
            dataChanged = true;
            qDebug() << objectName() << "updated driver temp to" << partialData.driverTemp;
        }
        if (partialData.motorTemp != 0.0f && partialData.motorTemp != currentData->motorTemp) {
            newData->motorTemp = partialData.motorTemp;
            dataChanged = true;
            qDebug() << objectName() << "updated motor temp to" << partialData.motorTemp;
        }

        // Update the master data state and emit signal
        if (dataChanged || newData->isConnected != currentData->isConnected) {
            updateData(newData);
            qDebug() << objectName() << "EMITTING servoDataChanged signal with position:" << newData->position;
            emit servoDataChanged(*newData);
        } else {
            qDebug() << objectName() << "No data changes detected, not emitting signal";
        }
    } else if (message.typeId() == Message::Type::ServoAlarmType) {
        auto const* alarmMsg = static_cast<const ServoAlarmMessage*>(&message);
        qWarning() << objectName() << "ALARM:" << alarmMsg->alarmCode() << alarmMsg->description();
        emit alarmDetected(alarmMsg->alarmCode(), alarmMsg->description());

        // Also update fault status in the data model
        auto newData = std::make_shared<ServoDriverData>(*data());
        newData->fault = true;
        updateData(newData);
        qDebug() << objectName() << "EMITTING servoDataChanged signal due to alarm";
        emit servoDataChanged(*newData);
    }
}

void ServoDriverDevice::onModbusReplyReady(QModbusReply* reply) {
    if (!m_parser || !reply) {
        qWarning() << objectName() << "onModbusReplyReady: missing parser or reply";
        return;
    }

    qDebug() << objectName() << "received Modbus reply, error:" << reply->error();

    if (reply->error() != QModbusDevice::NoError) {
        qWarning() << objectName() << "Modbus reply error:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    // Pass the reply to the parser to be converted into standard Messages
    auto messages = m_parser->parse(reply);
    reply->deleteLater(); // The device is responsible for deleting the reply

    qDebug() << objectName() << "Parser returned" << messages.size() << "messages";

    // Process the messages
    for (const auto& msg : messages) {
        if (msg) {
            qDebug() << objectName() << "processing message of type" << static_cast<int>(msg->typeId());
            processMessage(*msg);
        }
    }

    if (messages.empty()) {
        qDebug() << objectName() << "no messages parsed from Modbus reply";
    }
}

void ServoDriverDevice::writePosition(float position) {
    if (state() != DeviceState::Online || !m_transport) {
        qWarning() << objectName() << "cannot write position - device not online or transport missing";
        return;
    }

    auto transport = static_cast<ModbusTransport*>(m_transport);
    QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, ServoRegisters::POSITION_START_ADDR, 2);

    int32_t positionRaw = static_cast<int32_t>(position);
    quint16 highWord = (positionRaw >> 16) & 0xFFFF;
    quint16 lowWord = positionRaw & 0xFFFF;

    writeUnit.setValue(0, highWord);
    writeUnit.setValue(1, lowWord);

    qDebug() << objectName() << "writing position" << position << "as raw values:" << highWord << lowWord;

    // FIXED: Use new interface without slave ID parameter
    if (auto* reply = transport->sendWriteRequest(writeUnit)) {
        connect(reply, &QModbusReply::finished, this, [this, reply, position](){
            if (reply->error() != QModbusDevice::NoError) {
                qWarning() << objectName() << "Write position error:" << reply->errorString();
            } else {
                qDebug() << objectName() << "Successfully wrote position:" << position;
            }
            reply->deleteLater();
        });
    } else {
        qWarning() << objectName() << "Failed to create write position request";
    }
}
