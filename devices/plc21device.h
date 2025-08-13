#pragma once
#include "devices/TemplatedDevice.h"
#include "data/DataTypes.h"
#include <QTimer>
#include <QVector>

class Transport;
class ProtocolParser;
class QModbusReply;
class Message;

class Plc21Device : public TemplatedDevice<Plc21DeviceData> {
    Q_OBJECT
public:
    explicit Plc21Device(QObject* parent = nullptr);
    ~Plc21Device() override;

    // IDevice interface
    DeviceType type() const override;
    Q_INVOKABLE bool initialize() override;
    Q_INVOKABLE void setDependencies(Transport* transport, ProtocolParser* parser);
    void shutdown() override;

    // Public API for this device
    Q_INVOKABLE void writeOutputs(const QVector<bool>& outputs);

signals:
    void panelDataChanged(const Plc21DeviceData& data);

private slots:
    void onModbusReplyReady(QModbusReply* reply);
    void pollTimerTimeout();
    void processMessage(const Message& message);

private:
    void sendReadRequest(int registerType, int startAddress, int count);

    Transport* m_transport;
    ProtocolParser* m_parser;
    QTimer* m_pollTimer;
};
