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

public slots:
    // FIX: The slot's signature MUST match the signal's signature.
    void onRadarDataUpdated(std::shared_ptr<const RadarDeviceData> radarData);
    void onAzimuthServoDataUpdated(const ServoDriverData& servoData);
    void onElevationServoDataUpdated(const ServoDriverData& servoData);
    void onLrfDataUpdated(std::shared_ptr<const LrfData> lrfData);

signals:
    void targetsUpdated();
    void azimuthServoStateUpdated();
    void elevationServoStateUpdated();
    void lrfDataChangedForUI();

private:
    mutable QReadWriteLock m_radarLock;
    RadarDeviceData m_radarData;

    mutable QReadWriteLock m_azServoLock;
    ServoDriverData m_azServoData;

    mutable QReadWriteLock m_elServoLock;
    ServoDriverData m_elServoData;

    mutable QReadWriteLock m_lrfLock;
    LrfData m_lrfData;

};
