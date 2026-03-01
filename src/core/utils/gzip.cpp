/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "gzip.hpp"

#include <array>
#include <cstring>

#include <QFile>
#include <QBuffer>
#include "utils/i18n.hpp"


using namespace glaxnimate;

#ifdef GLAXNIMATE_CORE_KDE
bool utils::gzip::decompress(QIODevice& input, QByteArray& output, const utils::gzip::ErrorFunc& on_error)
{
    KCompressionDevice compressed(&input, false, KCompressionDevice::GZip);
    compressed.open(QIODevice::ReadOnly);
    output = compressed.readAll();

    if ( compressed.error() )
    {
        on_error(i18n("Could not decompress data"));
        return false;
    }

    return true;
}


bool utils::gzip::decompress(const QByteArray& input, QByteArray& output, const utils::gzip::ErrorFunc& on_error)
{
    QBuffer buf(const_cast<QByteArray*>(&input));
    return decompress(buf, output, on_error);
}
#endif

bool utils::gzip::is_compressed(QIODevice& input)
{
    return input.peek(2) == "\x1f\x8b";
}

bool utils::gzip::is_compressed(const QByteArray& input)
{
    return input.size() >= 2 && input[0] == '\x1f' && input[1] == '\x8b';
}

