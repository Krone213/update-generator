#ifndef CRCUNIT_H
#define CRCUNIT_H

#include <QByteArray>

namespace CrcUnit {

quint8 calcCrc8(const QByteArray &data);
quint16 calcCrc16(const QByteArray &data);
quint32 calcCrc32(const QByteArray &data);
quint32 calcCrc32_intermediate(const QByteArray &data);
}

#endif // CRCUNIT_H
