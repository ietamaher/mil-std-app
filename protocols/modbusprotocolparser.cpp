#include "modbusprotocolparser.h"
#include "protocols/ServoMessage.h"
#include "data/DataTypes.h"
#include <QModbusDataUnit>

ModbusProtocolParser::ModbusProtocolParser(QObject* parent) : ProtocolParser(parent) {
    initializeAlarmMap();
}

std::vector<MessagePtr> ModbusProtocolParser::parse(QModbusReply* reply) {
    std::vector<MessagePtr> messages;
    if (!reply || reply->error() != QModbusDevice::NoError) {
        // Optionally create an error message here
        return messages;
    }

    const QModbusDataUnit unit = reply->result();

    // Route the reply to the correct parser based on the start address
    switch (unit.startAddress()) {
    case ServoRegisters::POSITION_START_ADDR:
        if (auto msg = parsePositionReply(unit)) {
            messages.push_back(std::move(msg));
        }
        break;
    case ServoRegisters::TEMPERATURE_START_ADDR:
        if (auto msg = parseTemperatureReply(unit)) {
            messages.push_back(std::move(msg));
        }
        break;
    case ServoRegisters::ALARM_STATUS_ADDR:
        if (auto msg = parseAlarmReply(unit)) {
            messages.push_back(std::move(msg));
        }
        break;
        // Add other cases here...
    }

    return messages;
}

MessagePtr ModbusProtocolParser::parsePositionReply(const QModbusDataUnit& unit) {
    if (unit.valueCount() < ServoRegisters::POSITION_REG_COUNT) return nullptr;

    ServoDriverData data;
    int32_t positionRaw = (static_cast<int32_t>(unit.value(0)) << 16) | unit.value(1);
    data.position = static_cast<float>(positionRaw);

    return std::make_unique<ServoDataMessage>(data);
}

MessagePtr ModbusProtocolParser::parseTemperatureReply(const QModbusDataUnit& unit) {
    if (unit.valueCount() < ServoRegisters::TEMPERATURE_REG_COUNT) return nullptr;

    ServoDriverData data;
    int32_t driverTempRaw = (static_cast<int32_t>(unit.value(0)) << 16) | unit.value(1);
    data.driverTemp = static_cast<float>(driverTempRaw) * 0.1f;

    int32_t motorTempRaw = (static_cast<int32_t>(unit.value(2)) << 16) | unit.value(3);
    data.motorTemp = static_cast<float>(motorTempRaw) * 0.1f;

    return std::make_unique<ServoDataMessage>(data);
}

MessagePtr ModbusProtocolParser::parseAlarmReply(const QModbusDataUnit& unit) {
    if (unit.valueCount() < 2) return nullptr;

    uint16_t alarmCode = (unit.value(0) << 16) | unit.value(1);
    if (alarmCode != 0) {
        QString desc = getAlarmDescription(alarmCode);
        return std::make_unique<ServoAlarmMessage>(alarmCode, desc);
    }
    return nullptr;
}

QString ModbusProtocolParser::getAlarmDescription(uint16_t alarmCode) {
    return m_alarmMap.value(alarmCode, QString("Unknown Alarm: 0x%1").arg(alarmCode, 4, 16, QChar('0')));
}

void ModbusProtocolParser::initializeAlarmMap() {
    m_alarmMap[0x0001] = "Overcurrent Alarm";
    m_alarmMap[0x0002] = "Overvoltage Alarm";
    m_alarmMap[0x0003] = "Undervoltage Alarm";
    //... add all others
}
