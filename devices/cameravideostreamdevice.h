#pragma once

// --- Standard Library Includes ---
#include <atomic>
#include <string>
#include <vector> // For FrameData::detections

// --- Qt Includes ---
#include <QThread>
#include <QImage>
#include <QMutex>
#include <QRect>
#include <QColor>
#include <QObject>
#include <QString>
#include <QElapsedTimer>

// --- GStreamer Includes ---
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

// --- VPI Includes ---
#include <vpi/Types.h>          // VPIImage, VPIStream, VPIPayload, etc.
#include <vpi/Array.h>
#include <vpi/Image.h>
#include <vpi/Stream.h>
#include <vpi/algo/ConvertImageFormat.h>
#include <vpi/algo/CropScaler.h>
#include <vpi/algo/DCFTracker.h> // VPITrackingState, VPIDCFTrackedBoundingBox
#include <vpi/OpenCVInterop.hpp>

// --- OpenCV Includes ---
namespace cv {
    class Mat;
}

// --- Project Includes ---
#include "data/DataTypes.h"
#include "models/systemstatemodel.h" // For SystemStateData used in onSystemStateChanged slot

// --- Class Definition ---

/**
 * @brief Processes video frames from a GStreamer pipeline using VPI for tracking/detection.
 *
 * This class runs in a separate thread to avoid blocking the main GUI thread.
 * It receives video frames, performs VPI operations (like tracking), gathers system state,
 * and emits the combined data in a FrameData struct.
 */
class CameraVideoStreamDevice : public QThread
{
    Q_OBJECT

public:
    // --- Constructor & Destructor ---
    explicit CameraVideoStreamDevice(int cameraIndex,
                            const QString &deviceName,
                            int sourceWidth, // Output width expected after processing (e.g., crop/scale)
                            int sourceHeight, // Output height expected
                            SystemStateModel* stateModel,
                            QObject *parent = nullptr);
    ~CameraVideoStreamDevice() override;

    // --- Public Methods ---
    /**
     * @brief Signals the processing thread to stop gracefully.
     */
    void stop();

public slots:
    // --- Public Slots ---
    /**
     * @brief Enables or disables the VPI tracker.
     * @param enabled True to enable tracking, false to disable.
     */
    void setTrackingEnabled(bool enabled);

    /**
     * @brief Enables or disables object detection.
     * @param enabled True to enable detection, false to disable.
     */
    void setDetectionEnabled(bool enabled);

    /**
     * @brief Updates the internal state based on system-wide changes.
     * @param newState The latest system state data.
     */
    void onSystemStateChanged(const SystemStateData &newState);

signals:
    // --- Signals ---
    /**
     * @brief Emitted when a new frame has been processed and its data is ready.
     * @param data The structure containing the image and associated metadata.
     */
    void frameDataReady(const FrameData &data);

    /**
     * @brief Emitted when a processing error occurs.
     * @param cameraIndex The index of the camera where the error occurred.
     * @param errorMessage A description of the error.
     */
    void processingError(int cameraIndex, const QString &errorMessage);

    /**
     * @brief Emitted to provide status updates from the processing thread.
     * @param cameraIndex The index of the camera providing the update.
     * @param statusMessage The status message.
     */
    void statusUpdate(int cameraIndex, const QString &statusMessage);

protected:
    // --- QThread Reimplementation ---
    /**
     * @brief The main entry point for the thread's execution.
     * Contains the GStreamer main loop and frame processing logic.
     */
    void run() override;

private slots:
               // --- Private Slots ---
               // (None currently defined, add internal slots here if needed)

private:
    // --- Private Helper Methods ---

    // GStreamer Management
    bool initializeGStreamer();
    void cleanupGStreamer();
    static GstFlowReturn on_new_sample_from_sink(GstAppSink *sink, gpointer user_data);
    GstFlowReturn handleNewSample(GstAppSink *sink);

    // VPI Management & Processing
    bool initializeVPI();
    void cleanupVPI();
    bool processFrame(GstBuffer *buffer);
    bool initializeFirstTarget(VPIImage vpiFrameInput, float boxX, float boxY, float boxW, float boxH);
    bool runTrackingCycle(VPIImage vpiFrameInput);

    // Utility Methods
    QImage cvMatToQImage(const cv::Mat &inMat);

    // --- Member Variables ---

    // Thread Control

    // Configuration & Identification
    int m_cameraIndex;          // Identifier for this processor instance
    QString m_deviceName;       // e.g., /dev/video0
    int m_sourceWidth;          // Width from the GStreamer source (e.g., v4l2src)
    int m_sourceHeight;         // Height from the GStreamer source
    int m_outputWidth;          // Target width after VPI processing (e.g., crop/scale)
    int m_outputHeight;         // Target height after VPI processing
    SystemStateModel* m_stateModel;
    const int m_maxTrackedTargets; // Max targets for VPI arrays
    std::atomic<bool> m_abortRequest; // Flag to signal thread termination


    bool m_stabEnabled;
    float m_currentAzimuth;
    float m_currentElevation;
    float m_cameraFOV;
    float m_lrfDistance;


    bool m_sysCharged;
    bool m_sysArmed;
    bool m_sysReady;
    float m_speed;
    std::atomic<bool> m_trackingEnabled; // Control flag for enabling/disabling tracking
    bool m_trackerInitialized;  // Flag indicating if the tracker has an initial target
    std::atomic<bool> m_detectionEnabled; // Control flag for enabling/disabling detection
    // GStreamer Components
    GstElement *m_pipeline;     // The GStreamer pipeline
    GstElement *m_appSink;      // Sink element to grab frames from
    GMainLoop *m_gstLoop;       // GStreamer main loop for event handling

    // VPI Components & State
    VPIBackend m_vpiBackend;    // VPI backend (e.g., VPI_BACKEND_CUDA)
    VPIStream m_vpiStream;      // VPI processing stream
    VPIPayload m_dcfPayload;    // Payload for DCF tracker algorithm
    VPIPayload m_cropScalePayload; // Payload for Crop/Scaler algorithm
    VPIImage m_vpiFrameNV12;    // VPI Image buffer for NV12 format
    VPIImage m_vpiTgtPatches;   // VPI Image for tracker target patches
    VPIArray m_vpiInTargets;    // VPI Array for input bounding boxes
    VPIArray m_vpiOutTargets;   // VPI Array for output tracked bounding boxes
    VPIArray m_vpiConfidenceScores; // VPI Array for confidence scores
    int m_vpiTgtPatchSize;      // Size of the target patches
    VPIDCFTrackedBoundingBox m_currentTarget; // Internal tracker state (uses VPIRectI)
    QElapsedTimer m_velocityTimer; // To measure time between frames
    float m_lastTargetCenterX_px;
    float m_lastTargetCenterY_px;


    // OpenCV Buffers (if needed for intermediate steps)
    cv::Mat m_yuy2_host_buffer; // Example buffer for format conversion

    // State Variables (synchronized via mutex or atomic where needed)
    QMutex m_stateMutex;        // Mutex to protect access to shared state variables below
    OperationalMode m_currentMode;
    MotionMode m_motionMode;
    bool m_currentZeroingModeActive;
    bool m_currentZeroingApplied;
    float m_currentZeroingAzOffset;
    float m_currentZeroingElOffset;
    //TrackingPhase m_currentTrackingState; // Current tracking state (e.g., VPI_TRACKING_STATE_LOST)

    bool m_currentWindageModeActive;
    bool m_currentWindageApplied;
    float m_currentWindageSpeed;
    bool m_currentIsReticleInNoFireZone;
    bool m_currentGimbalStoppedAtNTZLimit;
    int m_currentReticleAimpointImageX_px;
    int m_currentReticleAimpointImageY_px;
    QString m_currentLeadStatusText;
    QString m_currentScanName; // Name of the current scan (if applicable)
    TrackingPhase m_currentTrackingPhase; // Current tracking phase (e.g., acquisition, tracking)
    bool m_trackerHasValidTarget;
    int m_currentAcquisitionBoxX_px; // X position of the acquisition box in pixels
    int m_currentAcquisitionBoxY_px; // Y position of the acquisition box in pixels
    int m_currentAcquisitionBoxW_px; // Width of the acquisition box in pixels
    int m_currentAcquisitionBoxH_px; // Height of the acquisition box in pixels
    bool m_currentActiveCameraIsDay; // Flag indicating if the active camera is a day camera
    FireMode m_fireMode;
    ReticleType m_reticleType;
    QColor m_colorStyle;
    bool m_isLacActiveForReticle; // Flag for LAC reticle mode

    // YoloInference Engine
    YoloInference m_inference;      // Object detection inference handler


    int m_frameCount = 0;

    int m_cropTop;
    int m_cropBottom;
};
