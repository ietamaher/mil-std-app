#ifndef DATATYPES_H
#define DATATYPES_H


#include <QtCore>
#include <QImage>
#include <QRect>
#include <QColor>
#include <vector>
#include "osd/osdrenderer.h"
#include "utils/inference.h"
#include <vpi/Types.h> // For VPITrackingState

// --- Data Structure Definitions ---

/**
 * @brief Holds the processed frame data and associated metadata to be emitted.
 */
struct FrameData {
    int cameraIndex = -1;
    QImage baseImage;
    bool trackingEnabled = false;
    bool trackerInitialized = false;
    VPITrackingState trackingState = VPI_TRACKING_STATE_LOST;
    QRect trackingBbox = QRect(0, 0, 0, 0); // Use QRect for Qt integration
    OperationalMode currentOpMode = OperationalMode::Idle;
    MotionMode motionMode = MotionMode::Manual;
    bool stabEnabled = false;
    float azimuth = 0.0f;
    float elevation = 0.0f;
    float cameraFOV = 0.0f;
    float speed = 0.0f;
    float lrfDistance = 0.0f;
    bool sysCharged = false;
    bool sysArmed = false;
    bool sysReady = false;
    FireMode fireMode = FireMode::SingleShot;
    ReticleType reticleType = ReticleType::BoxCrosshair;
    QColor colorStyle = QColor(70, 226, 165);
    std::vector<YoloDetection> detections;
    bool detectionEnabled = false;
    bool zeroingModeActive = false;
    float zeroingAzimuthOffset = 0.0f;
    float zeroingElevationOffset = 0.0f;
    bool zeroingAppliedToBallistics = false;
    bool windageModeActive = false;
    float windageSpeedKnots = 0.0f;
    bool windageAppliedToBallistics = false;
    bool isReticleInNoFireZone = false;
    bool gimbalStoppedAtNTZLimit = false;
    bool leadAngleActive = false;
    int reticleAimpointImageX_px;
    int reticleAimpointImageY_px;
    QString leadStatusText;
    QString currentScanName = "";
    TrackingPhase currentTrackingPhase = TrackingPhase::Off;
    bool trackerHasValidTarget = false;
    float acquisitionBoxX_px = 0.0f;
    float acquisitionBoxY_px = 0.0f;
    float acquisitionBoxW_px = 0.0f;
    float acquisitionBoxH_px = 0.0f;
};

// Placeholder struct based on usage in CameraVideoStreamDevice::onSystemStateChanged
struct SystemStateData {
    OperationalMode opMode;
    MotionMode motionMode;
    bool enableStabilization;
    float gimbalAz;
    float gimbalEl;
    float lrfDistance;
    bool ammoLoaded;
    bool gunArmed;
    bool isReady() const { return ammoLoaded && gunArmed; }
    bool activeCameraIsDay;
    float dayCurrentHFOV;
    float nightCurrentHFOV;
    float gimbalSpeed;
    FireMode fireMode;
    ReticleType reticleType;
    QColor colorStyle;
    bool zeroingModeActive;
    bool zeroingAppliedToBallistics;
    float zeroingAzimuthOffset;
    float zeroingElevationOffset;
    bool windageModeActive;
    bool windageAppliedToBallistics;
    float windageSpeedKnots;
    bool isReticleInNoFireZone;
    bool isReticleInNoTraverseZone;
    bool leadAngleCompensationActive;
    int reticleAimpointImageX_px;
    int reticleAimpointImageY_px;
    QString leadStatusText;
    QString currentScanName;
    TrackingPhase currentTrackingPhase;
    int acquisitionBoxX_px;
    int acquisitionBoxY_px;
    int acquisitionBoxW_px;
    int acquisitionBoxH_px;
};

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

// PLC21 DATA STRUCTURE
struct Plc21DeviceData {
    bool isConnected       = false; ///< Device connection status.

    // Digital Inputs
    bool armGunSW          = false; ///< State of the gun arming switch.
    bool loadAmmunitionSW  = false; ///< State of the ammunition loading switch.
    bool enableStationSW   = false; ///< State of the station enable switch.
    bool homePositionSW    = false; ///< State of the home position switch.
    bool enableStabilizationSW = false; ///< State of the stabilization enable switch.
    bool authorizeSw       = false; ///< State of the authorization switch.
    bool switchCameraSW    = false; ///< State of the camera switch.
    bool menuUpSW          = false; ///< State of the 'Menu Up' button.
    bool menuDownSW        = false; ///< State of the 'Menu Down' button.
    bool menuValSw         = false; ///< State of the 'Menu Validate' button.

    // Analog Inputs (Holding Registers)
    int  speedSW           = 2;     ///< Value of the speed switch.
    int  fireMode          = 0;     ///< Current fire mode.
    int  panelTemperature  = 0;     ///< Panel temperature.

    bool operator!=(const Plc21DeviceData &other) const {
        return (
            isConnected       != other.isConnected ||
            armGunSW          != other.armGunSW ||
            loadAmmunitionSW  != other.loadAmmunitionSW ||
            enableStationSW   != other.enableStationSW ||
            homePositionSW    != other.homePositionSW ||
            enableStabilizationSW != other.enableStabilizationSW ||
            authorizeSw       != other.authorizeSw ||
            switchCameraSW    != other.switchCameraSW ||
            menuUpSW          != other.menuUpSW ||
            menuDownSW        != other.menuDownSW ||
            menuValSw         != other.menuValSw ||
            speedSW           != other.speedSW ||
            fireMode          != other.fireMode ||
            panelTemperature  != other.panelTemperature
        );
    }
};

// PLC42 DATA STRUCTURE
struct Plc42Data {
    bool isConnected             = false; ///< Device connection status.

    // Discrete inputs
    bool stationUpperSensor      = false; ///< State of the station upper sensor.
    bool stationLowerSensor      = false; ///< State of the station lower sensor.
    bool emergencyStopActive     = false; ///< State of the emergency stop.
    bool ammunitionLevel         = false; ///< State of the ammunition level.
    bool stationInput1           = false; ///< State of station input 1.
    bool stationInput2           = false; ///< State of station input 2.
    bool stationInput3           = false; ///< State of station input 3.
    bool solenoidActive          = false; ///< State of solenoid activation.

    // Holding registers
    uint16_t solenoidMode        = 0;     ///< Solenoid mode.
    uint16_t gimbalOpMode        = 0;     ///< Gimbal operating mode.
    uint32_t azimuthSpeed        = 0;     ///< Azimuth speed (32-bit value).
    uint32_t elevationSpeed      = 0;     ///< Elevation speed (32-bit value).
    uint16_t azimuthDirection    = 0;     ///< Azimuth direction.
    uint16_t elevationDirection  = 0;     ///< Elevation direction.
    uint16_t solenoidState       = 0;     ///< Solenoid state.
    uint16_t resetAlarm          = 0;     ///< Alarm reset command.

    /**
     * @brief Equality comparison operator for Plc42Data.
     * @param other The other Plc42Data object to compare.
     * @return True if all members are equal, false otherwise.
     */
    bool operator==(const Plc42Data &other) const {
        return (
            isConnected             == other.isConnected &&
            stationUpperSensor      == other.stationUpperSensor &&
            stationLowerSensor      == other.stationLowerSensor &&
            emergencyStopActive     == other.emergencyStopActive &&
            ammunitionLevel         == other.ammunitionLevel &&
            stationInput1           == other.stationInput1 &&
            stationInput2           == other.stationInput2 &&
            stationInput3           == other.stationInput3 &&
            solenoidActive          == other.solenoidActive &&
            solenoidMode            == other.solenoidMode &&
            gimbalOpMode            == other.gimbalOpMode &&
            azimuthSpeed            == other.azimuthSpeed &&
            elevationSpeed          == other.elevationSpeed &&
            azimuthDirection        == other.azimuthDirection &&
            elevationDirection      == other.elevationDirection &&
            solenoidState           == other.solenoidState &&
            resetAlarm              == other.resetAlarm
            );
    }

    /**
     * @brief Inequality comparison operator for Plc42Data.
     * @param other The other Plc42Data object to compare.
     * @return True if at least one member is different, false otherwise.
     */
    bool operator!=(const Plc42Data &other) const {
        return !(*this == other);
    }
};

Q_DECLARE_METATYPE(Plc42Data)

#endif // DATATYPES_H
