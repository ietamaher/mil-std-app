// FILE: C:/Users/Maher/Documents/MIL-STD-architecture/protocols/lrfprotocolparser.h
#pragma once
#include "interfaces/ProtocolParser.h"
#include "data/DataTypes.h"
#include <vector>

class LrfProtocolParser : public ProtocolParser {
    Q_OBJECT
public:
    explicit LrfProtocolParser(QObject* parent = nullptr);
    ~LrfProtocolParser() override;

    // Use the new architecture's signature with std::vector
    std::vector<MessagePtr> parse(const QByteArray& rawData) override;
    QByteArray buildCommand(quint8 commandCode, const QByteArray& params = QByteArray()) const;

private:
    // Re-introduce correct protocol constants from the working code
    static const int PACKET_SIZE = 9;
    static const quint8 FRAME_HEADER = 0xEE;
    enum DeviceCode : quint8 { LRF = 0x07 };

    // Protocol logic functions from the working code
    quint8 calculateChecksum(const QByteArray &body) const;
    bool verifyChecksum(const QByteArray &packet) const;

    // A single handler that dispatches based on command code and returns a MessagePtr
    MessagePtr handleResponse(const QByteArray &response);

    // Individual response handlers that now create and return specific messages
    MessagePtr handleSelfCheckResponse(const QByteArray &response);
    MessagePtr handleRangingResponse(const QByteArray &response);
    MessagePtr handlePulseCountResponse(const QByteArray &response);
    MessagePtr handleProductInfoResponse(const QByteArray &response);
    MessagePtr handleTemperatureResponse(const QByteArray &response);

    QByteArray m_readBuffer;
    // m_transientData is no longer needed as a member; data is created locally in handlers.
};
