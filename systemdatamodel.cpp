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

void SystemDataModel::onRadarDataUpdated(std::shared_ptr<const RadarDeviceData> radarData) {
    qDebug() << "SystemDataModel::onRadarDataUpdated called with" << radarData->trackedTargets.size() << "targets";
    if (!radarData) {
        qWarning() << "SystemDataModel: Received null radar data";
        return;
    }
    {
        QWriteLocker locker(&m_radarLock);
        m_radarData = *radarData; // Copy the data from the shared pointer
    }
    qDebug() << "SystemDataModel: Emitting targetsUpdated signal";
    emit targetsUpdated();
}

void SystemDataModel::onAzimuthServoDataUpdated(const ServoDriverData& servoData) {
    qDebug() << "SystemDataModel::onAzimuthServoDataUpdated called - position:" << servoData.position
             << "connected:" << servoData.isConnected;
    {
        QWriteLocker locker(&m_azServoLock);
        m_azServoData = servoData;
    }
    qDebug() << "SystemDataModel: Emitting azimuthServoStateUpdated signal";
    emit azimuthServoStateUpdated();
}

void SystemDataModel::onElevationServoDataUpdated(const ServoDriverData& servoData) {
    qDebug() << "SystemDataModel::onElevationServoDataUpdated called - position:" << servoData.position
             << "connected:" << servoData.isConnected;
    {
        QWriteLocker locker(&m_elServoLock);
        m_elServoData = servoData;
    }
    qDebug() << "SystemDataModel: Emitting elevationServoStateUpdated signal";
    emit elevationServoStateUpdated();
}

void SystemDataModel::onLrfDataUpdated(std::shared_ptr<const LrfData> lrfData) {
    qDebug() << "SystemDataModel::onLrfDataUpdated called - distance:" << lrfData->lastDistance
             << "connected:" << lrfData->isConnected;
    if (!lrfData) {
        qWarning() << "SystemDataModel: Received null LRF data";
        return;
    }
    {
        QWriteLocker locker(&m_lrfLock);
        m_lrfData = *lrfData;
    }
    qDebug() << "SystemDataModel: Emitting lrfDataChangedForUI signal";
    emit lrfDataChangedForUI();
}
