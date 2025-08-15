#pragma once

#include "interfaces/ProtocolParser.h"
#include <QModbusDataUnit>

namespace GyroRegisters {
    constexpr int ALL_DATA_START_ADDRESS = 0x03E8;
    constexpr int ALL_DATA_REGISTER_COUNT = 18;
}

class GyroProtocolParser : public ProtocolParser {
    Q_OBJECT
public:
    explicit GyroProtocolParser(QObject* parent = nullptr);
    ~GyroProtocolParser() override = default;

    std::vector<MessagePtr> parse(const QByteArray& rawData) override;
    std::vector<MessagePtr> parse(QModbusReply* reply) override;

    QModbusDataUnit createReadRequest() const;

private:
    MessagePtr parseReply(const QModbusDataUnit& unit);
};
