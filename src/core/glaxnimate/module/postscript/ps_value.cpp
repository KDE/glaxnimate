/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ps_value.hpp"

#include <QMetaEnum>

using namespace Qt::StringLiterals;

glaxnimate::ps::Value::Value(ValueArray v)
    : Value(Value::Array, QVariant::fromValue(std::move(v)), Readable|Writable)
{}

glaxnimate::ps::Value::Value(const char *str)
    : Value(Value::String, QVariant::fromValue(ps::String(str)), Readable|Writable)
{}

glaxnimate::ps::Value::Value(ps::String v)
    : Value(Value::String, QVariant::fromValue(std::move(v)), Readable|Writable)
{}

glaxnimate::ps::Value::Value(const QByteArray& v)
    : Value(Value::String, QVariant::fromValue(ps::String(v)), Readable|Writable)
{

}

glaxnimate::ps::Value::Value(ValueDict v)
    : Value(Value::Dict, QVariant::fromValue(std::move(v)), Readable|Writable)
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

    if ( type_ == String )
    {
        if ( has_attribute(Name) )
        {
            if ( has_attribute(Executable) )
                return cast<ps::String>().to_string();
            return '/' + cast<ps::String>().to_string();
        }
        return cast<ps::String>().to_string_literal();
    }

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

QByteArray glaxnimate::ps::Value::type_name(Type t)
{
    return QByteArray(QMetaEnum::fromType<Type>().valueToKey(t)).toLower() + "type"_ba;
}

QByteArray glaxnimate::ps::Value::value_type_name() const
{
    if ( type_ == String && (attributes_ & Name) )
        return "nametype";
    return type_name(type_);
}

QByteArray glaxnimate::ps::Value::hash_key() const
{
    switch ( type_ )
    {
        case glaxnimate::ps::Value::Integer:
        case glaxnimate::ps::Value::Real:
        case glaxnimate::ps::Value::Boolean:
            return "VALUE_KEY " + to_string().toLatin1();
            // case glaxnimate::ps::Value::PackedArray:
        case glaxnimate::ps::Value::Array:
            return cast<ValueArray>().hash_key();
        case glaxnimate::ps::Value::String:
            return cast<ps::String>().bytes();
        case glaxnimate::ps::Value::Dict:
            return cast<ValueDict>().hash_key();
        default:
        case glaxnimate::ps::Value::File:
        case glaxnimate::ps::Value::Mark:
        case glaxnimate::ps::Value::Null:
        case glaxnimate::ps::Value::Save:
            return "VALUE_KEY " + value_type_name();
    }
}


QString glaxnimate::ps::ValueArray::to_pretty_string(bool as_executable) const
{
    QString str = QChar(as_executable ? '{' : '[');
    for ( const auto& v : *data )
        str += v.to_pretty_string() + ' ';
    return str.trimmed() + (as_executable ? '}' : ']');
}

bool glaxnimate::ps::ValueArray::operator==(const ValueArray &oth) const
{
    if ( size() != oth.size() )
        return false;
    for ( int i = 0; i < size(); i++ )
        if ( (*data)[i] != oth[i] )
            return false;
    return true;
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

glaxnimate::ps::ValueDict::mapped_type &glaxnimate::ps::ValueDict::operator[](const key_type &index)
{
    return (*data)[index];
}

const glaxnimate::ps::ValueDict::mapped_type &glaxnimate::ps::ValueDict::operator[](const key_type &index) const
{
    return (*data)[index];
}

void glaxnimate::ps::ValueDict::put(const key_type &key, Value val)
{
    (*data)[key] = std::move(val);
}

QString glaxnimate::ps::ValueDict::to_pretty_string() const
{
    QString str = u"<< "_s;
    for ( const auto& p : *data )
        str += String(p.first).to_string_literal() + ' ' + p.second.to_pretty_string() + ' ';
    return str + u">>"_s;
}

bool glaxnimate::ps::ValueDict::operator==(const ValueDict &oth) const
{
    if ( size() != oth.size() )
        return false;
    for ( auto it1 = begin(); it1 != end(); ++it1 )
    {
        auto it2 = oth.data->find(it1->first);
        if ( it2 == oth.end() || it1->second != it2->second )
            return false;
    }
    return true;
}

bool glaxnimate::ps::ValueDict::load_into(const key_type &key, Value &out) const
{
    auto it = data->find(key);
    if ( it == data->end() )
        return false;
    out = it->second;
    return true;
}

bool glaxnimate::ps::ValueDict::store(const key_type &key, const Value &value)
{
    auto it = data->find(key);
    if ( it == data->end() )
        return false;
    it->second = value;
    return true;
}

bool glaxnimate::ps::ValueDict::shallow_equal(const ValueDict &oth) const
{
    return data == oth.data;
}
