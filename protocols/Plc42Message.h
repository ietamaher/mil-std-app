#pragma once

#include "interfaces/Message.h"
#include "data/DataTypes.h"

class Plc42DiscreteInputsMessage : public Message {
public:
    explicit Plc42DiscreteInputsMessage(const Plc42Data& data) : m_data(data) {}

    Type typeId() const override { return Type::Plc42DiscreteInputsType; }

    const Plc42Data& data() const { return m_data; }

private:
    Plc42Data m_data;
};

class Plc42HoldingRegistersMessage : public Message {
public:
    explicit Plc42HoldingRegistersMessage(const Plc42Data& data) : m_data(data) {}

    Type typeId() const override { return Type::Plc42HoldingRegistersType; }

    const Plc42Data& data() const { return m_data; }

private:
    Plc42Data m_data;
};
