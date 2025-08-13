#include "nmeaparser.h"
#include "protocols/radarplotmessage.h"
#include <QStringList>
#include <QDebug>

NmeaParser::NmeaParser(QObject* parent)
    : ProtocolParser(parent)
{
}

// FIX: Add the missing destructor implementation.
NmeaParser::~NmeaParser() = default;

std::vector<MessagePtr> NmeaParser::parse(const QByteArray& rawData) {
    std::vector<MessagePtr> out;
    m_buffer.append(rawData);

    while (true) {
        int idx = m_buffer.indexOf("\r\n");
        if (idx == -1) break;
        QByteArray sentence = m_buffer.left(idx);
        m_buffer.remove(0, idx + 2);

        if (!sentence.startsWith('$')) continue;
        if (!validateChecksum(sentence)) continue;

        QStringList parts = QString::fromLatin1(sentence).split('*').first().split(',');
        if (parts.isEmpty()) continue;

        if (parts.first() == "$RATTM") {
            RadarTargetData plot = parseRATTM(parts);
            if (plot.id != 0) {
                out.push_back(std::make_unique<RadarPlotMessage>(plot));
            }
        }
    }
    return out;
}

bool NmeaParser::validateChecksum(const QByteArray& sentence) {
    int asterisk = sentence.indexOf('*');
    if (asterisk == -1) return false;
    quint8 ch = 0;
    for (int i = 1; i < asterisk; ++i) ch ^= static_cast<quint8>(sentence.at(i));
    bool ok = false;
    quint8 recv = sentence.mid(asterisk + 1).toUShort(&ok, 16);
    return ok && (recv == ch);
}

RadarTargetData NmeaParser::parseRATTM(const QStringList &fields) {
    RadarTargetData r;
    if (fields.size() < 7) return r;
    r.id = fields.at(1).toUInt();
    r.rangeMeters = fields.at(2).toFloat() * 1852.0f;
    r.azimuthDegrees = fields.at(3).toFloat();
    r.relativeCourseDegrees = fields.at(5).toFloat();
    r.relativeSpeedMPS = fields.at(6).toFloat() * 0.514444f;
    r.lastSeenTimestamp = QDateTime::currentMSecsSinceEpoch();
    return r;
}

