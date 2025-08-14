#include "systemdatamodel.h"
#include <QDebug>

SystemDataModel::SystemDataModel(QObject* parent) : QObject(parent) {
    qDebug() << "SystemDataModel created";
}

RadarDeviceData SystemDataModel::getRadarData() const {
    QReadLocker locker(&m_radarLock);
    return m_radarData;
}

ServoDriverData SystemDataModel::getAzimuthServoData() const {
    QReadLocker locker(&m_azServoLock);
    return m_azServoData;
}

ServoDriverData SystemDataModel::getElevationServoData() const {
    QReadLocker locker(&m_elServoLock);
    return m_elServoData;
}

LrfData SystemDataModel::getLrfData() const {
    QReadLocker locker(&m_lrfLock);
    return m_lrfData;
}

Plc21DeviceData SystemDataModel::getPlc21Data() const {
    QReadLocker locker(&m_plc21Lock);
    return m_plc21Data;
}

Plc42Data SystemDataModel::getPlc42Data() const {
    QReadLocker locker(&m_plc42Lock);
    return m_plc42Data;
}

FrameData SystemDataModel::getFrameData(int camIndex) const {
    QReadLocker locker(&m_frameLock);
    return m_frames.value(camIndex, FrameData()); // Return default if not found
}

void SystemDataModel::onRadarDataUpdated(std::shared_ptr<const RadarDeviceData> radarData) {
    if (!radarData) {
        qWarning() << "SystemDataModel: Received null radar data";
        return;
    }
    {
        QWriteLocker locker(&m_radarLock);
        m_radarData = *radarData;
    }
    emit targetsUpdated();
    updateSystemState();
}

void SystemDataModel::onAzimuthServoDataUpdated(const ServoDriverData& servoData) {
    {
        QWriteLocker locker(&m_azServoLock);
        m_azServoData = servoData;
    }
    emit azimuthServoStateUpdated();
    updateSystemState();
}

void SystemDataModel::onElevationServoDataUpdated(const ServoDriverData& servoData) {
    {
        QWriteLocker locker(&m_elServoLock);
        m_elServoData = servoData;
    }
    emit elevationServoStateUpdated();
    updateSystemState();
}

void SystemDataModel::onLrfDataUpdated(std::shared_ptr<const LrfData> lrfData) {
    if (!lrfData) {
        qWarning() << "SystemDataModel: Received null LRF data";
        return;
    }
    {
        QWriteLocker locker(&m_lrfLock);
        m_lrfData = *lrfData;
    }
    emit lrfDataChangedForUI();
    updateSystemState();
}

void SystemDataModel::onPlc21DataUpdated(const Plc21DeviceData& plc21Data) {
    {
        QWriteLocker locker(&m_plc21Lock);
        m_plc21Data = plc21Data;
    }
    emit plc21DataChangedForUI();
    updateSystemState();
}

void SystemDataModel::onPlc42DataUpdated(const Plc42Data& plc42Data) {
    {
        QWriteLocker locker(&m_plc42Lock);
        m_plc42Data = plc42Data;
    }
    emit plc42DataChangedForUI();
    updateSystemState();
}

void SystemDataModel::onFrameDataReady(const FrameData &data) {
    {
        QWriteLocker locker(&m_frameLock);
        m_frames[data.cameraIndex] = data;
    }
    emit frameDataChanged(data.cameraIndex);
    // No need to call updateSystemState here, as this is the consumer of system state
}

void SystemDataModel::updateSystemState() {
    // This function aggregates data from various devices into a single
    // SystemStateData object and emits it. This is what the camera device listens to.
    QWriteLocker azLocker(&m_azServoLock);
    QWriteLocker elLocker(&m_elServoLock);
    QWriteLocker lrfLocker(&m_lrfLock);
    QWriteLocker plc42Locker(&m_plc42Lock);

    // Note: This is a simplified aggregation. A real implementation would be more complex.
    m_systemState.gimbalAz = m_azServoData.position;
    m_systemState.gimbalEl = m_elServoData.position;
    m_systemState.lrfDistance = m_lrfData.lastDistance;
    m_systemState.gunArmed = m_plc42Data.solenoidActive; // Example mapping
    m_systemState.ammoLoaded = m_plc42Data.ammunitionLevel; // Example mapping

    // Emit the aggregated state
    emit systemStateChanged(m_systemState);
}
