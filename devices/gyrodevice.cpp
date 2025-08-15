#include "gyrodevice.h"
#include "communication/modbustransport.h"
#include "protocols/gyroprotocolparser.h"
#include "protocols/GyroMessage.h"
#include <QJsonObject>
#include <QDebug>
#include <QModbusDataUnit>
#include <QModbusReply>

GyroDevice::GyroDevice(QObject* parent)
    : TemplatedDevice<GyroData>(parent),
    m_transport(nullptr),
    m_parser(nullptr),
    m_pollTimer(new QTimer(this))
{
    connect(m_pollTimer, &QTimer::timeout, this, &GyroDevice::onPollTimerTimeout);
}

GyroDevice::~GyroDevice() {
    shutdown();
}

IDevice::DeviceType GyroDevice::type() const {
    return DeviceType::Inclinometer; // Or a new Gyro type if added
}

void GyroDevice::setDependencies(ModbusTransport* transport, GyroProtocolParser* parser) {
    m_transport = transport;
    m_parser = parser;
    m_transport->setParent(this);
    m_parser->setParent(this);
}

bool GyroDevice::initialize() {
    setState(DeviceState::Initializing);

    if (!m_transport || !m_parser) {
        qCritical() << objectName() << "missing dependencies!";
        setState(DeviceState::Error);
        return false;
    }

    QJsonObject config = property("config").toJsonObject();
    int pollInterval = config["pollIntervalMs"].toInt(100);

    if (m_transport && m_transport->open(config)) {
        setState(DeviceState::Online);
        m_pollTimer->start(pollInterval);
        return true;
    }

    setState(DeviceState::Error);
    return false;
}

void GyroDevice::shutdown() {
    m_pollTimer->stop();
    if (m_transport) {
        QMetaObject::invokeMethod(m_transport, "close", Qt::QueuedConnection);
    }
    setState(DeviceState::Offline);
}

void GyroDevice::onPollTimerTimeout() {
    sendReadRequest();
}

void GyroDevice::sendReadRequest() {
    if (state() != DeviceState::Online) return;

    QModbusDataUnit readUnit = m_parser->createReadRequest();
    if (auto* reply = m_transport->sendReadRequest(readUnit)) {
        connect(reply, &QModbusReply::finished, this, &GyroDevice::onReadReplyFinished);
    } else {
        qWarning() << objectName() << "Failed to send read request.";
    }
}

void GyroDevice::onReadReplyFinished() {
    auto* reply = qobject_cast<QModbusReply*>(sender());
    if (!reply) return;

    if (reply->error() == QModbusDevice::NoError) {
        auto messages = m_parser->parse(reply);
        for (const auto& msg : messages) {
            if (msg) processMessage(*msg);
        }
    }
    reply->deleteLater();
}

void GyroDevice::processMessage(const Message& message) {
    if (message.typeId() == Message::Type::GyroDataType) {
        const auto* gyroMsg = static_cast<const GyroDataMessage*>(&message);
        auto newData = std::make_shared<GyroData>(gyroMsg->data());
        newData->isConnected = true;

        if (*data() != *newData) {
            updateData(newData);
            emit gyroDataChanged(*newData);
        }
    }
}
