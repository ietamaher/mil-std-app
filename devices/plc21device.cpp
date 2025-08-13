#include "plc21device.h"
#include "communication/modbustransport.h"
#include "protocols/plc21protocolparser.h"
#include "protocols/Plc21Message.h"
#include <QJsonObject>
#include <QDebug>
#include <QModbusDataUnit>

Plc21Device::Plc21Device(QObject* parent)
    : TemplatedDevice<Plc21DeviceData>(parent),
    m_transport(nullptr),
    m_parser(nullptr),
    m_pollTimer(new QTimer(this))
{
    connect(m_pollTimer, &QTimer::timeout, this, &Plc21Device::pollTimerTimeout);
}

Plc21Device::~Plc21Device() {
    m_pollTimer->stop();
}

IDevice::DeviceType Plc21Device::type() const {
    return DeviceType::PLC;
}

void Plc21Device::setDependencies(Transport* transport, ProtocolParser* parser) {
    m_transport = transport;
    m_parser = parser;
    m_transport->setParent(this);
    m_parser->setParent(this);

    auto modbusTransport = static_cast<ModbusTransport*>(m_transport);
    connect(modbusTransport, &ModbusTransport::modbusReplyReady,
            this, &Plc21Device::onModbusReplyReady);
}

bool Plc21Device::initialize() {
    setState(DeviceState::Initializing);

    if (!m_transport || !m_parser) {
        qCritical() << objectName() << "missing dependencies!";
        setState(DeviceState::Error);
        return false;
    }

    QJsonObject config = property("config").toJsonObject();
    int pollInterval = config["pollIntervalMs"].toInt(50);

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

void Plc21Device::shutdown() {
    m_pollTimer->stop();
    if (m_transport) {
        QMetaObject::invokeMethod(m_transport, "close", Qt::QueuedConnection);
    }
    setState(DeviceState::Offline);
}

void Plc21Device::pollTimerTimeout() {
    sendReadRequest(QModbusDataUnit::DiscreteInputs,
                    Plc21Registers::DIGITAL_INPUTS_START_ADDRESS,
                    Plc21Registers::DIGITAL_INPUTS_COUNT);

    sendReadRequest(QModbusDataUnit::HoldingRegisters,
                    Plc21Registers::ANALOG_INPUTS_START_ADDRESS,
                    Plc21Registers::ANALOG_INPUTS_COUNT);
}

void Plc21Device::sendReadRequest(int registerType, int startAddress, int count) {
    if (state() != DeviceState::Online || !m_transport) return;

    auto transport = static_cast<ModbusTransport*>(m_transport);
    QModbusDataUnit readUnit(static_cast<QModbusDataUnit::RegisterType>(registerType), startAddress, count);

    if (auto* reply = transport->sendReadRequest(readUnit)) {
        qDebug() << objectName() << "sent read request for reg type" << registerType << "addr" << startAddress;
    } else {
        qWarning() << objectName() << "failed to send read request for reg type" << registerType << "addr" << startAddress;
    }
}

void Plc21Device::onModbusReplyReady(QModbusReply* reply) {
    if (!m_parser || !reply) {
        qWarning() << objectName() << "onModbusReplyReady: missing parser or reply";
        if(reply) reply->deleteLater();
        return;
    }

    if (reply->error() != QModbusDevice::NoError) {
        reply->deleteLater();
        return;
    }

    auto messages = m_parser->parse(reply);
    reply->deleteLater();

    for (const auto& msg : messages) {
        if (msg) {
            processMessage(*msg);
        }
    }
}

void Plc21Device::processMessage(const Message& message) {
    auto currentData = data();
    auto newData = std::make_shared<Plc21DeviceData>(*currentData);
    newData->isConnected = true;
    bool dataChanged = false;

    switch (message.typeId()) {
        case Message::Type::Plc21DigitalInputsType: {
            auto const* msg = static_cast<const Plc21DigitalInputsMessage*>(&message);
            const Plc21DeviceData& partial = msg->data();

            if (newData->armGunSW != partial.armGunSW) { newData->armGunSW = partial.armGunSW; dataChanged = true; }
            if (newData->loadAmmunitionSW != partial.loadAmmunitionSW) { newData->loadAmmunitionSW = partial.loadAmmunitionSW; dataChanged = true; }
            if (newData->enableStationSW != partial.enableStationSW) { newData->enableStationSW = partial.enableStationSW; dataChanged = true; }
            if (newData->homePositionSW != partial.homePositionSW) { newData->homePositionSW = partial.homePositionSW; dataChanged = true; }
            if (newData->enableStabilizationSW != partial.enableStabilizationSW) { newData->enableStabilizationSW = partial.enableStabilizationSW; dataChanged = true; }
            if (newData->authorizeSw != partial.authorizeSw) { newData->authorizeSw = partial.authorizeSw; dataChanged = true; }
            if (newData->switchCameraSW != partial.switchCameraSW) { newData->switchCameraSW = partial.switchCameraSW; dataChanged = true; }
            if (newData->menuUpSW != partial.menuUpSW) { newData->menuUpSW = partial.menuUpSW; dataChanged = true; }
            if (newData->menuDownSW != partial.menuDownSW) { newData->menuDownSW = partial.menuDownSW; dataChanged = true; }
            if (newData->menuValSw != partial.menuValSw) { newData->menuValSw = partial.menuValSw; dataChanged = true; }
            break;
        }
        case Message::Type::Plc21AnalogInputsType: {
            auto const* msg = static_cast<const Plc21AnalogInputsMessage*>(&message);
            const Plc21DeviceData& partial = msg->data();

            if (newData->speedSW != partial.speedSW) { newData->speedSW = partial.speedSW; dataChanged = true; }
            if (newData->fireMode != partial.fireMode) { newData->fireMode = partial.fireMode; dataChanged = true; }
            if (newData->panelTemperature != partial.panelTemperature) { newData->panelTemperature = partial.panelTemperature; dataChanged = true; }
            break;
        }
        default:
            return; // Not a message for us
    }

    if (dataChanged || newData->isConnected != currentData->isConnected) {
        updateData(newData);
        emit panelDataChanged(*newData);
    }
}

void Plc21Device::writeOutputs(const QVector<bool>& outputs) {
    if (state() != DeviceState::Online || !m_transport) {
        qWarning() << objectName() << "cannot write outputs - device not online or transport missing";
        return;
    }

    auto transport = static_cast<ModbusTransport*>(m_transport);

    QModbusDataUnit writeUnit(QModbusDataUnit::Coils, 0, outputs.size());
    for(int i = 0; i < outputs.size(); ++i) {
        writeUnit.setValue(i, outputs.at(i));
    }

    if (auto* reply = transport->sendWriteRequest(writeUnit)) {
        connect(reply, &QModbusReply::finished, this, [this, reply](){
            if (reply->error() != QModbusDevice::NoError) {
                qWarning() << objectName() << "Write outputs error:" << reply->errorString();
            } else {
                qDebug() << objectName() << "Successfully wrote outputs.";
            }
            reply->deleteLater();
        });
    } else {
        qWarning() << objectName() << "Failed to create write outputs request";
    }
}
