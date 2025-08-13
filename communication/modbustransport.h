#pragma once
#include "interfaces/transport.h"
#include <QModbusRtuSerialClient>
#include <QJsonObject>

class ModbusTransport : public Transport {
    Q_OBJECT
public:
    explicit ModbusTransport(QObject* parent = nullptr);
    ~ModbusTransport() override;

    bool open(const QJsonObject& config) override;
    void close() override;
    void sendFrame(const QByteArray& /*frame*/) override { /* no-op */ }

    // FIXED: Remove slaveId parameter - it should come from config
    QModbusReply* sendReadRequest(const QModbusDataUnit &unit);
    QModbusReply* sendWriteRequest(const QModbusDataUnit &unit);

    // FIXED: Add method to get current slave ID
    int slaveId() const { return m_slaveId; }

signals:
    void modbusReplyReady(QModbusReply* reply);

private slots:
    void onStateChanged(QModbusDevice::State state);
    void onModbusError(QModbusDevice::Error err);

private:
    QModbusRtuSerialClient* m_client;
    QJsonObject m_config;
    int m_slaveId; // FIXED: Store slave ID from config
};
