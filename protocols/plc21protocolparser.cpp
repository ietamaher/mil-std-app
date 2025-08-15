#include "plc21protocolparser.h"
#include "protocols/Plc21Message.h"
#include "data/DataTypes.h"
#include <QModbusDataUnit>
#include <QDebug>

Plc21ProtocolParser::Plc21ProtocolParser(QObject* parent) : ProtocolParser(parent) {}

QModbusDataUnit Plc21ProtocolParser::createWriteOutputsRequest(const QVector<bool>& outputs) {
    QModbusDataUnit writeUnit(QModbusDataUnit::Coils, 0, outputs.size());
    for(int i = 0; i < outputs.size(); ++i) {
        writeUnit.setValue(i, outputs.at(i));
    }
    return writeUnit;
}

std::vector<MessagePtr> Plc21ProtocolParser::parse(QModbusReply* reply) {
    std::vector<MessagePtr> messages;
    if (!reply || reply->error() != QModbusDevice::NoError) {
        qWarning() << "Plc21ProtocolParser: Received reply with error:" << (reply ? reply->errorString() : "null reply");
        return messages;
    }

    const QModbusDataUnit unit = reply->result();

    MessagePtr msg;
    if (unit.registerType() == QModbusDataUnit::DiscreteInputs) {
        msg = parseDigitalInputs(unit);
    } else if (unit.registerType() == QModbusDataUnit::HoldingRegisters) {
        msg = parseAnalogInputs(unit);
    } else {
        qWarning() << "Plc21ProtocolParser: Received unexpected register type:" << unit.registerType();
    }

    if (msg) {
        messages.push_back(std::move(msg));
    }

    return messages;
}

MessagePtr Plc21ProtocolParser::parseDigitalInputs(const QModbusDataUnit& unit) {
    if (unit.valueCount() < Plc21Registers::DIGITAL_INPUTS_COUNT) {
        qWarning() << "Plc21ProtocolParser: Digital inputs reply has insufficient data:" << unit.valueCount();
        return nullptr;
    }

    Plc21DeviceData data;
    data.authorizeSw = (unit.value(0) != 0);
    data.menuValSw = (unit.value(1) != 0);
    data.menuDownSW = (unit.value(2) != 0);
    data.menuUpSW = (unit.value(3) != 0);
    data.switchCameraSW = (unit.value(4) != 0);
    data.enableStabilizationSW = (unit.value(5) != 0);
    data.homePositionSW = (unit.value(6) != 0);
    data.loadAmmunitionSW = (unit.value(8) != 0);
    data.armGunSW = (unit.value(9) != 0);
    data.enableStationSW = (unit.value(10) != 0);

    return std::make_unique<Plc21DigitalInputsMessage>(data);
}

MessagePtr Plc21ProtocolParser::parseAnalogInputs(const QModbusDataUnit& unit) {
    if (unit.valueCount() < Plc21Registers::ANALOG_INPUTS_COUNT) {
        qWarning() << "Plc21ProtocolParser: Analog inputs reply has insufficient data:" << unit.valueCount();
        return nullptr;
    }

    Plc21DeviceData data;
    data.fireMode = unit.value(0);
    data.speedSW = unit.value(1);
    data.panelTemperature = unit.value(2);

    return std::make_unique<Plc21AnalogInputsMessage>(data);
}
