#pragma once
#include <QObject>
#include <memory>
#include "data/DataTypes.h"

class SystemDataModel : public QObject {
    Q_OBJECT
public:
    explicit SystemDataModel(QObject* parent = nullptr);

    RadarDeviceData getRadarData() const;
    ServoDriverData getAzimuthServoData() const;
    ServoDriverData getElevationServoData() const;
    LrfData getLrfData() const;
    Plc21DeviceData getPlc21Data() const;

public slots:
    // FIX: The slot's signature MUST match the signal's signature.
    void onRadarDataUpdated(std::shared_ptr<const RadarDeviceData> radarData);
    void onAzimuthServoDataUpdated(const ServoDriverData& servoData);
    void onElevationServoDataUpdated(const ServoDriverData& servoData);
    void onLrfDataUpdated(std::shared_ptr<const LrfData> lrfData);
    void onPlc21DataUpdated(const Plc21DeviceData& plc21Data);

signals:
    void targetsUpdated();
    void azimuthServoStateUpdated();
    void elevationServoStateUpdated();
    void lrfDataChangedForUI();
    void plc21DataChangedForUI();

private:
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
};
