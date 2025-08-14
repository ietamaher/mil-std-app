#include "plc42protocolparser.h"
#include "protocols/Plc42Message.h"
#include <QModbusReply>
#include <QDebug>

Plc42ProtocolParser::Plc42ProtocolParser(QObject* parent) : ProtocolParser(parent) {}

std::vector<MessagePtr> Plc42ProtocolParser::parse(const QByteArray& /*rawData*/) {
    // This parser works with QModbusReply, not raw byte arrays.
    return {};
}

std::vector<MessagePtr> Plc42ProtocolParser::parse(QModbusReply* reply) {
    std::vector<MessagePtr> messages;
    if (!reply) {
        return messages;
    }

    if (reply->error() != QModbusDevice::NoError) {
        qWarning() << "PLC42 reply error:" << reply->errorString();
        // Optionally create an error message
        return messages;
    }

    const QModbusDataUnit unit = reply->result();
    if (auto msg = parseReply(unit)) {
        messages.push_back(std::move(msg));
    }

    return messages;
}

MessagePtr Plc42ProtocolParser::parseReply(const QModbusDataUnit& unit) {
    Plc42Data data;

    if (unit.registerType() == QModbusDataUnit::DiscreteInputs) {
        if (unit.valueCount() >= Plc42Registers::DIGITAL_INPUTS_COUNT) {
            data.stationUpperSensor  = unit.value(0);
            data.stationLowerSensor  = unit.value(1);
            data.emergencyStopActive = unit.value(2);
            data.ammunitionLevel     = unit.value(3);
            data.stationInput1       = unit.value(4);
            data.stationInput2       = unit.value(5);
            data.stationInput3       = unit.value(6);
            data.solenoidActive      = unit.value(7);
        } else {
            qWarning() << "PLC42: Not enough digital input values in reply.";
            return nullptr;
        }
    } else if (unit.registerType() == QModbusDataUnit::HoldingRegisters) {
        if (unit.valueCount() >= Plc42Registers::HOLDING_REGISTERS_COUNT) {
            data.solenoidMode       = unit.value(0);
            data.gimbalOpMode       = unit.value(1);

            // Combine two 16-bit registers into a 32-bit value for azimuth speed.
            uint16_t azLow  = unit.value(2);
            uint16_t azHigh = unit.value(3);
            data.azimuthSpeed = (static_cast<uint32_t>(azHigh) << 16) | azLow;

            // Combine two 16-bit registers into a 32-bit value for elevation speed.
            uint16_t elLow  = unit.value(4);
            uint16_t elHigh = unit.value(5);
            data.elevationSpeed = (static_cast<uint32_t>(elHigh) << 16) | elLow;

            data.azimuthDirection   = unit.value(6);
            data.elevationDirection = unit.value(7);
            data.solenoidState      = unit.value(8);
            data.resetAlarm         = unit.value(9);
        } else {
            qWarning() << "PLC42: Not enough holding register values in reply.";
            return nullptr;
        }
    }

    return std::make_unique<Plc42DataMessage>(data);
}

QModbusDataUnit Plc42ProtocolParser::createReadDigitalInputsRequest() const {
    return QModbusDataUnit(QModbusDataUnit::DiscreteInputs,
                           Plc42Registers::DIGITAL_INPUTS_START_ADDRESS,
                           Plc42Registers::DIGITAL_INPUTS_COUNT);
}

QModbusDataUnit Plc42ProtocolParser::createReadHoldingRegistersRequest() const {
    return QModbusDataUnit(QModbusDataUnit::HoldingRegisters,
                           Plc42Registers::HOLDING_REGISTERS_START_ADDRESS,
                           Plc42Registers::HOLDING_REGISTERS_COUNT);
}
