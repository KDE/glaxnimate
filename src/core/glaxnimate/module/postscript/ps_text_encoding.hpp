/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QByteArray>

namespace glaxnimate::ps {

class Base85Decoder
{
public:
    bool add_char(char ch);
    void finish();

    const QByteArray& decoded() const { return data; }

    static QByteArray decode(const QByteArray& input);

private:
    QByteArray data;
    quint32 accum = 0;
    int count = 0;
};


} // namespace glaxnimate::ps
