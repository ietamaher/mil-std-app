#pragma once

#include "devices/TemplatedDevice.h"
#include "data/DataTypes.h"
#include <QTimer>

class ModbusTransport;
class GyroProtocolParser;
class QModbusReply;
class Message;

class GyroDevice : public TemplatedDevice<GyroData> {
    Q_OBJECT
public:
    explicit GyroDevice(QObject* parent = nullptr);
    ~GyroDevice() override;

    DeviceType type() const override;
    Q_INVOKABLE bool initialize() override;
    Q_INVOKABLE void setDependencies(ModbusTransport* transport, GyroProtocolParser* parser);
    void shutdown() override;

signals:
    void gyroDataChanged(const GyroData& data);

private slots:
    void onReadReplyFinished();
    void onPollTimerTimeout();
    void processMessage(const Message& message);

private:
    void sendReadRequest();

    ModbusTransport* m_transport;
    GyroProtocolParser* m_parser;
    QTimer* m_pollTimer;
};
