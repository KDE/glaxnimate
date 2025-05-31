/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <QtGlobal>

#include "io/aep/string_decoder.hpp"

#include <QStringConverter>
QString glaxnimate::io::aep::decode_string(const QByteArray& data)
{
    auto encoding = QStringConverter::encodingForData(data);
    if ( encoding )
        return QStringDecoder(*encoding).decode(data);
    return QStringDecoder(QStringConverter::Utf8).decode(data);
}

QString glaxnimate::io::aep::decode_utf16(const QByteArray& data, bool big_endian)
{
    auto encoding = big_endian ? QStringConverter::Utf16BE : QStringConverter::Utf16LE;
    return QStringDecoder(encoding).decode(data);
}

