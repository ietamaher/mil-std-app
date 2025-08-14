#pragma once

#include "osd/osdrenderer.h"
#include <QString>
#include <QColor>

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

// Placeholder for the model class itself
class SystemStateModel : public QObject {
    Q_OBJECT
public:
    void updateTrackingResult(int, bool, float, float, float, float, float, float, int) {}
};
