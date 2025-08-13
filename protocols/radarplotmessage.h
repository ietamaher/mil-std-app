#pragma once
#include "interfaces/message.h"
#include "data/DataTypes.h"

class RadarPlotMessage : public Message {
public:
    explicit RadarPlotMessage(const RadarTargetData& p) : m_plot(p) {}
    // FIX: Corrected to use the proper enum member.
    Type typeId() const override { return Type::RadarPlotType; }
    const RadarTargetData& plot() const { return m_plot; }
private:
    RadarTargetData m_plot;
};
