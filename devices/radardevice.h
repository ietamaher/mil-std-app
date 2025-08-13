#pragma once
#include "devices/TemplatedDevice.h"
#include "data/DataTypes.h"
#include <memory>

class Transport;
class ProtocolParser;
class Message;

class RadarDevice : public TemplatedDevice<RadarDeviceData> {
    Q_OBJECT
public:
    explicit RadarDevice(QObject* parent = nullptr);
    // Implement ALL pure virtual functions from IDevice
    DeviceType type() const override;
    Q_INVOKABLE bool initialize() override;
    Q_INVOKABLE void setDependencies(Transport* transport, ProtocolParser* parser);

    void shutdown() override; // This declaration is necessary

signals:
    void radarDataUpdated(std::shared_ptr<const RadarDeviceData> newData);

private slots:
    void processFrame(const QByteArray& frame);
    void processMessage(const Message& message);

private:
    Transport* m_transport;
    ProtocolParser* m_parser;
};
