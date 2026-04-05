/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ps_value.hpp"

#include <QMetaEnum>

using namespace Qt::StringLiterals;

glaxnimate::ps::Value::Value(ValueArray v)
    : Value(Value::Array, QVariant::fromValue(std::move(v)))
{}

glaxnimate::ps::Value::Value(const char *str)
    : Value(Value::String, QVariant::fromValue(ps::String(str)))
{}

glaxnimate::ps::Value::Value(ps::String v)
    : Value(Value::String, QVariant::fromValue(std::move(v)))
{}

glaxnimate::ps::Value::Value(const QByteArray& v)
    : Value(Value::String, QVariant::fromValue(ps::String(v)))
{

}

QString glaxnimate::ps::Value::to_pretty_string() const
{
    if ( type_ == Mark )
        return u"mark"_s;
    else if ( type_ == Null )
        return u"null"_s;
    else if ( type_ == Dict )
        return u"-dict-"_s;
    else if ( type_ == Array )
        return cast<ValueArray>().to_pretty_string();
    else if ( type_ == String )
        return cast<ps::String>().to_string_literal();

    return to_string();
}

QString glaxnimate::ps::Value::type_name(Type t)
{
    return QString::fromLatin1(QMetaEnum::fromType<Type>().valueToKey(t)).toLower() + "type"_L1;
}

QString glaxnimate::ps::ValueArray::to_pretty_string() const
{
    QString str = QChar('[');
    for ( const auto& v : *data )
        str += v.to_pretty_string() + ' ';
    return str.trimmed() + ']';
}

QString glaxnimate::ps::String::to_string() const
{
    return QString::fromUtf8(*data);
}

QByteArray glaxnimate::ps::String::to_string_literal() const
{
    QByteArray result(1, '(');
    for ( auto ch : *data )
    {
        switch ( ch )
        {
            case '\\':
            case '(':
            case ')':
                result += '\\';
                result += ch;
                continue;
            case '\n':
                result += "\\n";
                continue;
            case '\t':
                result += "\\t";
                continue;
            case '\r':
                result += "\\r";
                continue;
            case '\b':
                result += "\\b";
                continue;
            case '\v':
                result += "\\v";
                continue;
            case '\f':
                result += "\\f";
                continue;
        }

        if ( ch < ' ' || ch > 126 )
        {
            result += '\\' + QByteArray::number(ch, 8);
            continue;
        }

        result += ch;
    }

    return result + ')';
}
