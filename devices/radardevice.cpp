#include "radardevice.h"
#include "interfaces/Transport.h"
#include "interfaces/ProtocolParser.h"
#include "protocols/radarplotmessage.h"
#include <QDebug>
#include <QJsonObject>

RadarDevice::RadarDevice(QObject* parent)
    : TemplatedDevice<RadarDeviceData>(parent),
    m_transport(nullptr),
    m_parser(nullptr)
{
    // Constructor is now empty
}

IDevice::DeviceType RadarDevice::type() const {
    return DeviceType::Radar;
}

void RadarDevice::setDependencies(Transport* transport, ProtocolParser* parser)
{
    // This is called on the IO thread
    m_transport = transport;
    m_parser = parser;

    // Parent them to the device for lifetime management
    m_transport->setParent(this);
    m_parser->setParent(this);

    // Now it's safe to connect signals
    connect(m_transport, &Transport::frameReceived, this, &RadarDevice::processFrame);
}

bool RadarDevice::initialize() {
    setState(IDevice::DeviceState::Initializing);
    QJsonObject config = property("config").toJsonObject();

    // Safety check
    if (!m_transport) {
        setState(DeviceState::Error);
        return false;
    }

    if (m_transport->open(config)) {
        setState(DeviceState::Online);
        return true;
    }
    setState(DeviceState::Error);
    return false;
}

// THIS IS THE CORRECT IMPLEMENTATION
void RadarDevice::shutdown()
{
    // 1. Perform device-specific shutdown actions
    if (m_transport) {
        // Since this method might be called from another thread,
        // use invokeMethod to ensure close() runs on the transport's thread.
        QMetaObject::invokeMethod(m_transport, "close", Qt::QueuedConnection);
    }
    // 2. Update the device's state
    setState(DeviceState::Offline);
}

void RadarDevice::processFrame(const QByteArray& frame) {
    // FIX: Declare 'messages' as const to prevent copy-on-write when iterating.
    //      This forces the use of a const_iterator, which is safe for move-only types.
    const auto messages = m_parser->parse(frame);
    for (const auto& msg : messages) {
        if(msg) {
            processMessage(*msg);
        }
    }
}

void RadarDevice::processMessage(const Message& message) {
    // FIX: Changed enum to correct 'Message::Type::RadarPlotType'.
    if (message.typeId() == Message::Type::RadarPlotType) {
        // FIX: Replaced non-existent 'as<>()' method with a safe 'static_cast'
        //      since the type has already been verified.
        const auto* plotMsg = static_cast<const RadarPlotMessage*>(&message);
        if (!plotMsg) return;

        auto currentData = data();
        auto newData = std::make_shared<RadarDeviceData>(*currentData);

        RadarTargetData plot = plotMsg->plot();
        plot.lastSeenTimestamp = QDateTime::currentMSecsSinceEpoch();
        newData->trackedTargets.insert(plot.id, plot);
        newData->isConnected = true;
        newData->lastMessageTimestamp = QDateTime::currentMSecsSinceEpoch();

        updateData(newData);
        emit radarDataUpdated(newData);
    }
}
