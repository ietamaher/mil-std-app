#pragma once

// Placeholder enums based on usage in CameraVideoStreamDevice
enum class OperationalMode { Idle, Surveillance };
enum class MotionMode { Manual };
enum class FireMode { SingleShot };
enum class ReticleType { BoxCrosshair };
enum class TrackingPhase { Off, Acquisition, Tracking_LockPending, Tracking_ActiveLock, Tracking_Coast, Tracking_Firing };
