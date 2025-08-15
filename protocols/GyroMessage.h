#pragma once

#include "interfaces/Message.h"
#include "data/DataTypes.h"

class GyroDataMessage : public Message {
public:
    explicit GyroDataMessage(const GyroData& data) : m_data(data) {}

    Type typeId() const override { return Type::GyroDataType; }

    const GyroData& data() const { return m_data; }

private:
    GyroData m_data;
};
