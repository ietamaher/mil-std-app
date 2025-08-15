// FILE: protocols/modbusprotocolparser.h
#pragma once
#include "interfaces/ProtocolParser.h"
#include <QModbusReply>
#include <QMap>
#include <QModbusDataUnit>

// Define register addresses here for clarity
namespace ServoRegisters {
constexpr int POSITION_START_ADDR = 204;
constexpr int POSITION_REG_COUNT = 2;
constexpr int TEMPERATURE_START_ADDR = 248;
constexpr int TEMPERATURE_REG_COUNT = 4;
constexpr int ALARM_STATUS_ADDR = 172;
// Add other addresses as needed...
}

class ModbusProtocolParser : public ProtocolParser {
    Q_OBJECT
public:
    explicit ModbusProtocolParser(QObject* parent = nullptr);
    ~ModbusProtocolParser() override = default;

    // This parser does not use raw byte streaming
    std::vector<MessagePtr> parse(const QByteArray& /*rawData*/) override { return {}; }

    // This is the primary method for this parser
    std::vector<MessagePtr> parse(QModbusReply* reply) override;

    // Method to create write requests
    QModbusDataUnit createWritePositionRequest(float position);

private:
    // Helper methods to create specific messages from a reply
    MessagePtr parsePositionReply(const QModbusDataUnit& unit);
    MessagePtr parseTemperatureReply(const QModbusDataUnit& unit);
    MessagePtr parseAlarmReply(const QModbusDataUnit& unit);

    // Alarm description lookup
    void initializeAlarmMap();
    QString getAlarmDescription(uint16_t alarmCode);
    QMap<uint16_t, QString> m_alarmMap;
};
