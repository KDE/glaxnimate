/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ps_value.hpp"


QString glaxnimate::ps::format_string(const QString &value)
{
    QString result = QChar('(');
    for ( auto qch : value )
    {
        quint32 ch = qch.unicode();
        if ( ch > 255 )
        {
            // TODO?
            result += qch;
            continue;
        }

        switch ( ch )
        {
            case '\\':
            case '(':
            case ')':
                result += '\\';
                result += qch;
                continue;
            case '\n':
                result += u"\\n"_s;
                continue;
            case '\t':
                result += u"\\t"_s;
                continue;
            case '\r':
                result += u"\\r"_s;
                continue;
            case '\b':
                result += u"\\b"_s;
                continue;
            case '\v':
                result += u"\\v"_s;
                continue;
            case '\f':
                result += u"\\f"_s;
                continue;
        }

        if ( ch < ' ' || ch > 126 )
        {
            result += '\\' + QString::number(ch, 8);
            continue;
        }

        result += qch;
    }

    return result + ')';
}

QString glaxnimate::ps::Value::to_pretty_string() const
{
    if ( type_ == Mark )
        return u"mark"_s;
    else if ( type_ == Null )
        return u"null"_s;
    else if ( type_ == Dictionary )
        return u"-dict-"_s;

    if ( type_ == Array )
    {
        QString str = QChar('[');
        for ( const auto& v : cast<std::vector<Value>>() )
            str += v.to_pretty_string() + ' ';
        return str.trimmed() + ']';
    }

    if ( type_ == String )
    {
        return format_string(to_string());
    }

    return to_string();
}
