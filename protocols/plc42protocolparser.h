#pragma once

#include "interfaces/ProtocolParser.h"
#include <QModbusDataUnit>

// Define register addresses for the PLC42
namespace Plc42Registers {
    // Discrete Inputs
    constexpr int DIGITAL_INPUTS_START_ADDRESS = 0;
    constexpr int DIGITAL_INPUTS_COUNT = 8;

    // Holding Registers
    constexpr int HOLDING_REGISTERS_START_ADDRESS = 0;
    constexpr int HOLDING_REGISTERS_COUNT = 10;
}

class Plc42ProtocolParser : public ProtocolParser {
    Q_OBJECT
public:
    explicit Plc42ProtocolParser(QObject* parent = nullptr);
    ~Plc42ProtocolParser() override = default;

    // This parser does not use raw byte streaming
    std::vector<MessagePtr> parse(const QByteArray& rawData) override;

    // This is the primary method for this parser
    std::vector<MessagePtr> parse(QModbusReply* reply) override;

    // Methods to create read requests
    QModbusDataUnit createReadDigitalInputsRequest() const;
    QModbusDataUnit createReadHoldingRegistersRequest() const;

private:
    // Helper method to create a message from a reply
    MessagePtr parseReply(const QModbusDataUnit& unit);
};
