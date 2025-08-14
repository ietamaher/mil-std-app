#pragma once

#include <QRectF>
#include <QString>

// Placeholder struct based on usage in CameraVideoStreamDevice
struct YoloDetection {
    QRectF bbox;
    float score;
    int class_idx;
    QString class_name;
};

// Placeholder class for YoloInference
class YoloInference {
public:
    YoloInference(const std::string& onnx_path, const cv::Size& input_size, const std::string& classes_path, bool use_cuda) {}
    std::vector<YoloDetection> runInference(const cv::Mat& frame) {
        return {};
    }
};
