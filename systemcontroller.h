#pragma once
#include <QObject>
#include <QJsonObject>
#include "interfaces/IDevice.h" // For DeviceState
#include <QTimer>
#include <QColor>

class SystemDataModel;
class RadarDevice;
class ServoDriverDevice;
class LRFDevice;
class QThread;

class SystemController : public QObject {
    Q_OBJECT
public:
    explicit SystemController(QObject* parent = nullptr);
    ~SystemController();

    bool initialize(const QJsonObject& config);
    SystemDataModel* model() const;

public slots:
    void trackTarget(quint32 targetId);
    void stopTracking();
    void lrfGetSingleDistance();
    void lrfGetPulseCount();

signals:
    void logMessage(const QString& message, QColor color = Qt::black);
    // ADDED: New signal for device creation completion
    void deviceCreated(IDevice* device, const QString& deviceName, const QString& type);

private slots:
    void updateTracking();
    void onDeviceStateChanged(IDevice::DeviceState state);
    void onDeviceError(const QString& message);
    // ADDED: New slot to handle device registration
    void onDeviceCreated(IDevice* device, const QString& deviceName, const QString& type);

    void checkMetaTypes();
    void testSignalEmission();
private:
    bool createDevices(const QJsonObject& deviceConfigs);
    // ADDED: New method to create a single device on IO thread
    void createSingleDevice(const QString& deviceName, const QJsonObject& devConf);
    void connectSignals();
    // ADDED: New method to check if all devices are ready
    void checkAndConnectSignals();

    SystemDataModel* m_model;
    QList<IDevice*> m_devices;
    QThread* m_ioThread;

    RadarDevice* m_radar = nullptr;
    ServoDriverDevice* m_servo_az = nullptr;
    ServoDriverDevice* m_servo_el = nullptr;
    LRFDevice* m_lrf = nullptr;

    QTimer* m_trackingTimer = nullptr;
    quint32 m_trackedTargetId = 0;
    void debugDeviceConnections();
};
