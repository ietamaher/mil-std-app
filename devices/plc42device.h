#pragma once

#include "devices/TemplatedDevice.h"
#include "data/DataTypes.h"
#include <QTimer>

class ModbusTransport;
class Plc42ProtocolParser;
class Message;
class QModbusReply;

class PLC42Device : public TemplatedDevice<Plc42Data> {
    Q_OBJECT
public:
    explicit PLC42Device(QObject* parent = nullptr);
    ~PLC42Device() override;

    DeviceType type() const override;
    Q_INVOKABLE bool initialize() override;
    Q_INVOKABLE void setDependencies(ModbusTransport* transport, Plc42ProtocolParser* parser);
    void shutdown() override;

    // Public API for controlling the PLC
    Q_INVOKABLE void setSolenoidMode(uint16_t mode);
    Q_INVOKABLE void setGimbalMotionMode(uint16_t mode);
    Q_INVOKABLE void setAzimuthSpeed(uint32_t speed);
    Q_INVOKABLE void setElevationSpeed(uint32_t speed);
    Q_INVOKABLE void setAzimuthDirection(uint16_t direction);
    Q_INVOKABLE void setElevationDirection(uint16_t direction);
    Q_INVOKABLE void setSolenoidState(uint16_t state);
    Q_INVOKABLE void setResetAlarm(uint16_t alarm);

signals:
    void plc42DataChanged(const Plc42Data& data);
    void responseTimeout();

private slots:
    void onPollTimerTimeout();
    void onReadReplyFinished();
    void onWriteReplyFinished();

private:
    void sendReadRequest(const QModbusDataUnit& readUnit);
    void sendWriteRequest(const QModbusDataUnit& writeUnit);
    void processMessage(const Message& message);
    void mergeAndProcessData(const Plc42Data& partialData);


    ModbusTransport* m_transport;
    Plc42ProtocolParser* m_parser;
    QTimer* m_pollTimer;
    Plc42Data m_stagingData;

    // To keep track of pending read requests
    int m_pendingReads;
};
