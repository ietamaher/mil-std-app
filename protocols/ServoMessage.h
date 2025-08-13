#pragma once
#include "interfaces/Message.h"
#include "data/DataTypes.h"
#include <QString>

// A message to carry servo telemetry data
class ServoDataMessage : public Message {
public:
    explicit ServoDataMessage(const ServoDriverData& data) : m_data(data) {}
    Type typeId() const override { return Type::ServoDataType; }
    const ServoDriverData& data() const { return m_data; }
private:
    ServoDriverData m_data;
};

// A message to carry a specific alarm code
class ServoAlarmMessage : public Message {
public:
    explicit ServoAlarmMessage(uint16_t code, const QString& desc)
        : m_alarmCode(code), m_description(desc) {}
    Type typeId() const override { return Type::ServoAlarmType; }
    uint16_t alarmCode() const { return m_alarmCode; }
    const QString& description() const { return m_description; }
private:
    uint16_t m_alarmCode;
    QString m_description;
};
