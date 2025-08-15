#include "gyroprotocolparser.h"
#include "protocols/GyroMessage.h"
#include <QModbusReply>
#include <QDebug>
#include <QtEndian>

GyroProtocolParser::GyroProtocolParser(QObject* parent) : ProtocolParser(parent) {}

std::vector<MessagePtr> GyroProtocolParser::parse(const QByteArray& /*rawData*/) {
    return {};
}

std::vector<MessagePtr> GyroProtocolParser::parse(QModbusReply* reply) {
    std::vector<MessagePtr> messages;
    if (!reply || reply->error() != QModbusDevice::NoError) {
        if (reply) {
            qWarning() << "Gyro Read Error:" << reply->errorString();
        }
        return messages;
    }

    if (auto msg = parseReply(reply->result())) {
        messages.push_back(std::move(msg));
    }
    return messages;
}

MessagePtr GyroProtocolParser::parseReply(const QModbusDataUnit& dataUnit) {
    if (dataUnit.valueCount() != GyroRegisters::ALL_DATA_REGISTER_COUNT) {
        qWarning() << "Incorrect number of registers received. Expected"
                   << GyroRegisters::ALL_DATA_REGISTER_COUNT << "got" << dataUnit.valueCount();
        return nullptr;
    }

    GyroData newData;

    auto parseFloat = [&](int index) -> float {
        quint16 high = dataUnit.value(index);
        quint16 low = dataUnit.value(index + 1);
        quint32 combined = (static_cast<quint32>(high) << 16) | low;
        return qFromBigEndian(combined);
    };

    auto parseInt32 = [&](int index) -> qint32 {
        quint16 high = dataUnit.value(index);
        quint16 low = dataUnit.value(index + 1);
        quint32 combined = (static_cast<quint32>(high) << 16) | low;
        return qFromBigEndian(combined);
    };

    newData.imuPitchDeg = parseFloat(0);
    newData.imuRollDeg = parseFloat(2);
    newData.temperature = parseFloat(4) / 10.0;
    newData.rawAccelX = parseInt32(6);
    newData.rawAccelY = parseInt32(8);
    newData.rawAccelZ = parseInt32(10);
    newData.rawGyroX = parseInt32(12);
    newData.rawGyroY = parseInt32(14);
    newData.rawGyroZ = parseInt32(16);

    return std::make_unique<GyroDataMessage>(newData);
}

QModbusDataUnit GyroProtocolParser::createReadRequest() const {
    return QModbusDataUnit(QModbusDataUnit::InputRegisters,
                             GyroRegisters::ALL_DATA_START_ADDRESS,
                             GyroRegisters::ALL_DATA_REGISTER_COUNT);
}
