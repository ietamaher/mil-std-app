#pragma once
#include "interfaces/ProtocolParser.h"
#include <QModbusReply>
#include <QModbusDataUnit>
#include <QVector>

namespace Plc21Registers {
    constexpr int DIGITAL_INPUTS_START_ADDRESS = 0;
    constexpr int DIGITAL_INPUTS_COUNT = 13;
    constexpr int ANALOG_INPUTS_START_ADDRESS = 0;
    constexpr int ANALOG_INPUTS_COUNT = 6;
}

class Plc21ProtocolParser : public ProtocolParser {
    Q_OBJECT
public:
    explicit Plc21ProtocolParser(QObject* parent = nullptr);
    ~Plc21ProtocolParser() override = default;

    std::vector<MessagePtr> parse(const QByteArray& /*rawData*/) override { return {}; }
    std::vector<MessagePtr> parse(QModbusReply* reply) override;

    QModbusDataUnit createWriteOutputsRequest(const QVector<bool>& outputs);

private:
    MessagePtr parseDigitalInputs(const QModbusDataUnit& unit);
    MessagePtr parseAnalogInputs(const QModbusDataUnit& unit);
};
