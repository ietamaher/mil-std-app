#pragma once
#include <QObject>
#include <memory>
#include "data/DataTypes.h"
#include <QReadWriteLock>
#include <QMap>

class SystemDataModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(ServoDriverData azServoData READ getAzimuthServoData NOTIFY azimuthServoStateUpdated)
    Q_PROPERTY(ServoDriverData elServoData READ getElevationServoData NOTIFY elevationServoStateUpdated)
    Q_PROPERTY(LrfData lrfData READ getLrfData NOTIFY lrfDataChangedForUI)
    Q_PROPERTY(Plc21DeviceData plc21Data READ getPlc21Data NOTIFY plc21DataChangedForUI)
    Q_PROPERTY(Plc42Data plc42Data READ getPlc42Data NOTIFY plc42DataChangedForUI)
    Q_PROPERTY(GyroData gyroData READ getGyroData NOTIFY gyroDataChangedForUI)

public:
    explicit SystemDataModel(QObject* parent = nullptr);

    RadarDeviceData getRadarData() const;
    ServoDriverData getAzimuthServoData() const;
    ServoDriverData getElevationServoData() const;
    LrfData getLrfData() const;
    Plc21DeviceData getPlc21Data() const;
    Plc42Data getPlc42Data() const;
    FrameData getFrameData(int camIndex) const;
    GyroData getGyroData() const;

public slots:
    void onRadarDataUpdated(std::shared_ptr<const RadarDeviceData> radarData);
    void onAzimuthServoDataUpdated(const ServoDriverData& servoData);
    void onElevationServoDataUpdated(const ServoDriverData& servoData);
    void onLrfDataUpdated(std::shared_ptr<const LrfData> lrfData);
    void onPlc21DataUpdated(const Plc21DeviceData& plc21Data);
    void onPlc42DataUpdated(const Plc42Data& plc42Data);
    void onFrameDataReady(const FrameData& data);
    void onGyroDataUpdated(const GyroData& data);

signals:
    void targetsUpdated();
    void azimuthServoStateUpdated();
    void elevationServoStateUpdated();
    void lrfDataChangedForUI();
    void plc21DataChangedForUI();
    void plc42DataChangedForUI();
    void gyroDataChangedForUI();
    void frameDataChanged(int camIndex);
    void systemStateChanged(const SystemStateData& state);

private:
    void updateSystemState(); // Helper to aggregate and emit system state

    mutable QReadWriteLock m_radarLock;
    RadarDeviceData m_radarData;

    mutable QReadWriteLock m_azServoLock;
    ServoDriverData m_azServoData;

    mutable QReadWriteLock m_elServoLock;
    ServoDriverData m_elServoData;

    mutable QReadWriteLock m_lrfLock;
    LrfData m_lrfData;

    mutable QReadWriteLock m_plc21Lock;
    Plc21DeviceData m_plc21Data;

    mutable QReadWriteLock m_plc42Lock;
    Plc42Data m_plc42Data;

    mutable QReadWriteLock m_gyroLock;
    GyroData m_gyroData;

    mutable QReadWriteLock m_frameLock;
    QMap<int, FrameData> m_frames;

    SystemStateData m_systemState;
};
