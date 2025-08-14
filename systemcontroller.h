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
class Plc21Device;
class PLC42Device;
class CameraVideoStreamDevice;
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
    void setCameraTracking(int camIndex, bool enabled);
    void setCameraDetection(int camIndex, bool enabled);

signals:
    void logMessage(const QString& message, QColor color = Qt::black);
    void deviceCreated(IDevice* device, const QString& deviceName, const QString& type);

private slots:
    void updateTracking();
    void onDeviceStateChanged(IDevice::DeviceState state);
    void onDeviceError(const QString& message);
    void onDeviceCreated(IDevice* device, const QString& deviceName, const QString& type);
    void checkMetaTypes();

private:
    bool createDevices(const QJsonObject& deviceConfigs);
    void createSingleDevice(const QString& deviceName, const QJsonObject& devConf);
    bool createCameraDevices(const QJsonObject& cameraConfigs);
    void connectSignals();
    void checkAndConnectSignals();

    SystemDataModel* m_model;
    QList<IDevice*> m_devices;
    QThread* m_ioThread;

    // Standard Devices
    RadarDevice* m_radar = nullptr;
    ServoDriverDevice* m_servo_az = nullptr;
    ServoDriverDevice* m_servo_el = nullptr;
    LRFDevice* m_lrf = nullptr;
    Plc21Device* m_plc21 = nullptr;
    PLC42Device* m_plc42 = nullptr;

    // Camera Devices (managed separately as they are QThreads)
    CameraVideoStreamDevice* m_dayProcessor = nullptr;
    CameraVideoStreamDevice* m_nightProcessor = nullptr;

    QTimer* m_trackingTimer = nullptr;
    quint32 m_trackedTargetId = 0;
};
