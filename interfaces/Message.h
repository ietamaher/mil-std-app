#pragma once
#include <memory>

class Message {
public:
    enum class Type {
        Generic,
        RadarPlotType,
        Plc21DigitalInputsType,
        Plc21AnalogInputsType,
        LrfDataType,
        LrfInfoType,
        ServoDataType,
        ServoAlarmType,
        Plc42DiscreteInputsType,
        Plc42HoldingRegistersType
        // add others as needed
    };
    virtual ~Message() = default;
    virtual Type typeId() const { return Type::Generic; }
};

using MessagePtr = std::unique_ptr<Message>;
