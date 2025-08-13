#pragma once
#include "devices/TemplatedDevice.h"
#include "data/DataTypes.h"
#include <memory>
#include <QTimer>

class Transport;
class LrfProtocolParser;
class Message;

class LRFDevice : public TemplatedDevice<LrfData> {
    Q_OBJECT
public:
    explicit LRFDevice(QObject* parent = nullptr);

    DeviceType type() const override;
    Q_INVOKABLE bool initialize() override;
    Q_INVOKABLE void setDependencies(Transport* transport, LrfProtocolParser* parser);

    void shutdown() override;

    // Public API
    Q_INVOKABLE void sendSelfCheck();
    Q_INVOKABLE void sendSingleRanging();
    Q_INVOKABLE void sendContinuousRanging1Hz();
    Q_INVOKABLE void sendContinuousRanging5Hz();
    Q_INVOKABLE void sendContinuousRanging10Hz();
    Q_INVOKABLE void stopRanging();
    Q_INVOKABLE void queryAccumulatedLaserCount();
    Q_INVOKABLE void queryProductInfo();
    Q_INVOKABLE void queryTemperature();

signals:
    void lrfDataChanged(std::shared_ptr<const LrfData> newData);
    void productInfoReceived(quint8 productId, const QString& softwareVersion);
    void responseTimeout();

private slots:
    void processFrame(const QByteArray& frame);
    void processMessage(const Message& message);
    void handleCommandResponseTimeout();

private:
    void sendCommand(quint8 commandCode);

    Transport* m_transport;
    LrfProtocolParser* m_parser;
    QTimer* m_commandResponseTimer;
};
