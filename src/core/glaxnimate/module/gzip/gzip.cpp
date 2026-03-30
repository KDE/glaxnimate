/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "gzip.hpp"

#include <QFile>

using namespace glaxnimate;

bool gzip::is_compressed(QIODevice& input)
{
    return input.peek(2) == "\x1f\x8b";
}

bool gzip::is_compressed(const QByteArray& input)
{
    return input.size() >= 2 && input[0] == '\x1f' && input[1] == '\x8b';
}

