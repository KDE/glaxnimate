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

glaxnimate::ps::Value::Value(ValueDict v)
    : Value(Value::Dict, QVariant::fromValue(std::move(v)))
{

}

QString glaxnimate::ps::Value::to_pretty_string() const
{
    if ( type_ == Mark )
        return u"mark"_s;
    else if ( type_ == Null )
        return u"null"_s;
    else if ( type_ == Dict )
        return cast<ValueDict>().to_pretty_string();
        //return u"-dict-"_s;
    else if ( type_ == Array )
        return cast<ValueArray>().to_pretty_string(has_attribute(Executable));
    else if ( type_ == String )
        return cast<ps::String>().to_string_literal();

    return to_string();
}

bool glaxnimate::ps::Value::can_convert(Type t) const
{
    if ( t == type_ )
        return true;

    return (t == Real && type_ == Integer) || (type_ == Real && t == Integer);
}

bool glaxnimate::ps::Value::shallow_equal(const Value &oth) const
{
    if ( !can_convert(oth.type_) )
        return false;

    switch ( type_ )
    {
        case Integer:
        case Real:
        case Boolean:
        case String:
            return *this == oth;
        case Array:
            return cast<ValueArray>().shallow_equal(oth.cast<ValueArray>());
        case Dict:
            return cast<ValueDict>().shallow_equal(oth.cast<ValueDict>());
        case Mark:
        case Null:
            return true;
        // TODO
        case File:
        case Save:
        default:
            return false;
    }
}

QString glaxnimate::ps::Value::type_name(Type t)
{
    return QString::fromLatin1(QMetaEnum::fromType<Type>().valueToKey(t)).toLower() + "type"_L1;
}

std::size_t glaxnimate::ps::Value::hash() const
{
    switch ( type_ )
    {
        case glaxnimate::ps::Value::Integer:
            return std::hash<int>()(cast<int>());
        case glaxnimate::ps::Value::Real:
            return std::hash<float>()(cast<float>());
        case glaxnimate::ps::Value::Boolean:
            return std::hash<bool>()(cast<bool>());
            // case glaxnimate::ps::Value::PackedArray:
        case glaxnimate::ps::Value::Array:
            return cast<ValueArray>().hash();
        case glaxnimate::ps::Value::String:
            return cast<ps::String>().hash();
        case glaxnimate::ps::Value::Dict:
            return cast<ValueDict>().hash();
        default:
        case glaxnimate::ps::Value::File:
        case glaxnimate::ps::Value::Mark:
        case glaxnimate::ps::Value::Null:
        case glaxnimate::ps::Value::Save:
            return 0;
    }
}

QString glaxnimate::ps::ValueArray::to_pretty_string(bool as_executable) const
{
    QString str = QChar(as_executable ? '{' : '[');
    for ( const auto& v : *data )
        str += v.to_pretty_string() + ' ';
    return str.trimmed() + (as_executable ? '}' : ']');
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

QString glaxnimate::ps::ValueDict::to_pretty_string() const
{
    QString str = u"<< "_s;
    for ( const auto& p : *data )
        str += p.first.to_pretty_string() + ' ' + p.second.to_pretty_string() + ' ';
    return str + u">>"_s;
}
