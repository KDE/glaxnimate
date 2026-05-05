/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ps_text_encoding.hpp"

using namespace glaxnimate::ps;

bool Base85Decoder::add_char(char ch)
{
    if ( ch == 'z' )
    {
        if ( count == 0 )
        {
            data.append(4, 0);
            return true;
        }
        return false;
    }

    if ( ch < '!' || ch > 'u' )
        return false;

    accum *= 85;
    accum += ch - '!';
    count++;
    if ( count == 5 )
    {
        data.push_back((accum >> 24) & 0xff);
        data.push_back((accum >> 16) & 0xff);
        data.push_back((accum >> 8) & 0xff);
        data.push_back(accum & 0xff);
        count = 0;
        accum = 0;
    }

    return true;
}

void Base85Decoder::finish()
{
    if ( count )
    {
        int to_flush = count - 1;
        for( ; count % 5; count++ )
        {
            accum *= 85;
            accum += 84;
        }

        if ( to_flush > 0 )
            data.push_back((accum >> 24) & 0xff);
        if ( to_flush > 1 )
            data.push_back((accum >> 16) & 0xff);
        if ( to_flush > 2 )
            data.push_back((accum >> 8) & 0xff);
        if ( to_flush > 3 )
            data.push_back(accum & 0xff);
        count = 0;
        accum = 0;
    }
}

QByteArray Base85Decoder::decode(const QByteArray &input)
{
    Base85Decoder decoder;
    decoder.data.reserve(input.size() * 4 / 5);
    for ( char c : input )
        decoder.add_char(c);
    decoder.finish();
    return decoder.decoded();
}

