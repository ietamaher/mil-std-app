// FILE: devices/servodriverdevice.h
#pragma once
#include "devices/TemplatedDevice.h"
#include "data/DataTypes.h"
#include <QTimer>

class Transport;
class ProtocolParser; // Use forward declaration
class QModbusDataUnit;
class QModbusReply;
class Message;

class ServoDriverDevice : public TemplatedDevice<ServoDriverData> {
    Q_OBJECT
public:
    explicit ServoDriverDevice(QObject* parent = nullptr);
    ~ServoDriverDevice() override;

    // IDevice interface
    DeviceType type() const override;

    Q_INVOKABLE bool initialize() override;
    Q_INVOKABLE void setDependencies(Transport* transport, ProtocolParser* parser);

    void shutdown() override;

    // Public API for this device
    Q_INVOKABLE void writePosition(float position); // Example write command

signals:
    void servoDataChanged(const ServoDriverData& data);
    void alarmDetected(uint16_t alarmCode, const QString& description);

private slots:
    void onReadReplyFinished();
    void onWriteReplyFinished();
    void pollTimerTimeout();
    void processMessage(const Message& message);

private:
    void sendReadRequest(int startAddress, int count);
    void sendWriteRequest(const QModbusDataUnit& writeUnit);

    Transport* m_transport;
    ProtocolParser* m_parser;
    QTimer* m_pollTimer;
    int m_pendingReads = 0;
};
