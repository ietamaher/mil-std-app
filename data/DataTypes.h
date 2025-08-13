#ifndef DATATYPES_H
#define DATATYPES_H


#include <QtCore>
// Data from your original RadarDevice example
struct RadarTargetData {
    quint32 id = 0;
    float azimuthDegrees = 0.0f;
    float rangeMeters = 0.0f;
    float relativeCourseDegrees = 0.0f;
    float relativeSpeedMPS = 0.0f;
    qint64 lastSeenTimestamp = 0;
};

// Represents the data held by the RadarDevice
struct RadarDeviceData {
    bool isConnected = false;
    QHash<quint32, RadarTargetData> trackedTargets;
    qint64 lastMessageTimestamp = 0;
};

// Represents the data for a Servo Driver
struct ServoDriverData {
    bool isConnected   = false;   ///< True if device is connected
    float position     = 0.0f;    ///< Current servo position
    float rpm          = 0.0f;    ///< Servo RPM
    float torque       = 0.0f;    ///< Current torque
    float motorTemp    = 0.0f;    ///< Motor temperature
    float driverTemp   = 0.0f;    ///< Driver temperature
    bool fault         = false;   ///< Fault status

    bool operator==(const ServoDriverData &other) const {
        return (
            isConnected   == other.isConnected &&
            position     == other.position    &&
            rpm          == other.rpm         &&
            torque       == other.torque      &&
            motorTemp    == other.motorTemp   &&
            driverTemp   == other.driverTemp  &&
            fault        == other.fault
            );
    }
    bool operator!=(const ServoDriverData &other) const {
        return !(*this == other);
    }
};

// LRF DATA STRUCTURE
struct LrfData {
    bool isConnected = false;
    quint16 lastDistance = 0;
    bool isLastRangingValid = false;
    quint8 pulseCount = 0;
    quint8 rawStatusByte = 0;
    bool isFault = false;
    bool noEcho = false;
    bool laserNotOut = false;
    bool isOverTemperature = false;
    bool isTempValid = false;
    qint8 temperature = 0;
    quint32 laserCount = 0;

    bool operator!=(const LrfData &other) const {
        return (isConnected != other.isConnected ||
                lastDistance != other.lastDistance ||
                isLastRangingValid != other.isLastRangingValid ||
                pulseCount != other.pulseCount ||
                rawStatusByte != other.rawStatusByte ||
                isFault != other.isFault ||
                noEcho != other.noEcho ||
                laserNotOut != other.laserNotOut ||
                isOverTemperature != other.isOverTemperature ||
                isTempValid != other.isTempValid ||
                temperature != other.temperature ||
                laserCount != other.laserCount);
    }
};



#endif // DATATYPES_H


