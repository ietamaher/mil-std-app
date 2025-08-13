#pragma once
#include "interfaces/Message.h"
#include "data/DataTypes.h"

// A message to carry PLC21 digital input data
class Plc21DigitalInputsMessage : public Message {
public:
    explicit Plc21DigitalInputsMessage(const Plc21DeviceData& data) : m_data(data) {}
    Type typeId() const override { return Type::Plc21DigitalInputsType; }
    const Plc21DeviceData& data() const { return m_data; }
private:
    Plc21DeviceData m_data;
};

// A message to carry PLC21 analog input data
class Plc21AnalogInputsMessage : public Message {
public:
    explicit Plc21AnalogInputsMessage(const Plc21DeviceData& data) : m_data(data) {}
    Type typeId() const override { return Type::Plc21AnalogInputsType; }
    const Plc21DeviceData& data() const { return m_data; }
private:
    Plc21DeviceData m_data;
};
