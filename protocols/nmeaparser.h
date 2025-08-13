#pragma once
#include "interfaces/ProtocolParser.h"
#include "data/DataTypes.h"
#include <vector> // <-- ADD THIS INCLUDE

class NmeaParser : public ProtocolParser {
    Q_OBJECT
public:
    explicit NmeaParser(QObject* parent = nullptr);
    ~NmeaParser() override;

    // FIX: Implement the corrected interface returning std::vector.
    std::vector<MessagePtr> parse(const QByteArray& rawData) override;
private:
    bool validateChecksum(const QByteArray& sentence);
    RadarTargetData parseRATTM(const QStringList& fields);

    QByteArray m_buffer;
};
