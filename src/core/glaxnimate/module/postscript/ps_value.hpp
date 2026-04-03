/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <vector>
#include <map>

#include <QVariant>

namespace glaxnimate::ps {

using namespace Qt::StringLiterals;

/**
 * @brief formats a string into a postscript string
 */
QString format_string(const QString& value);

class Value
{
public:
    enum Type
    {
        Integer,
        Real,
        Boolean,
        Array,
        PackedArray = Array,
        String,
        Dictionary,
        File, // TODO
        Mark,
        Null,
        Save,
    };

    enum Attribute
    {
        None = 0,
        Execute = 1,
        Write = 2,
        Read = 4
    };

    template<Type Tp> struct type_for;

    Value() : Value(Null, {}) {}

    Value(const QByteArray& str) : Value(String, str) {}
    Value(const QString& str) : Value(String, str) {}
    Value(int num) : Value(Integer, num) {}
    Value(float num) : Value(Real, num) {}
    Value(bool num) : Value(Boolean, num) {}

    template<Type Tp>
    static Value from(typename type_for<Tp>::type val)
    {
        return Value(Tp, QVariant::fromValue(std::move(val)));
    }
    template<Type Tp> static Value from();

    Type type() const noexcept { return type_; }

    const QVariant& value() const { return value_; }

    QString to_string() const
    {
        return value_.toString();
    }

    QString to_pretty_string() const;

    template<Type Tp>
    bool can_convert() const
    {
        return value_.canConvert(type_for<Tp>::meta_type());
    }

    bool can_convert(Type tp) const
    {
        switch ( tp )
        {
#define CASE(x) case x: return can_convert<x>();
            CASE(Integer)
            CASE(Real)
            CASE(Boolean)
            CASE(Array)
            // CASE(PackedArray)
            CASE(String)
            CASE(Dictionary)
            CASE(File)
            CASE(Mark)
            CASE(Null)
            CASE(Save)
#undef CASE
        }
        return false;
    }

    template<class T>
    T cast() const
    {
        return qvariant_cast<T>(value_);
    }

    int attributes() const
    {
        return attributes_;
    }

    bool has_attribute(int attr) const
    {
        return (attributes_ & attr) == attr;
    }

    void set_attribute(int attr, bool on)
    {
        attributes_ = on ? attributes_ | attr : attributes_ & ~attr;
    }

private:
    Value(Type type, QVariant value) : type_(type), value_(std::move(value)) {}

    Type type_;
    QVariant value_;
    int attributes_ = None;
};

using ValueArray = std::vector<glaxnimate::ps::Value>;
using ValueDict = std::map<QString, glaxnimate::ps::Value>;

} // namespace glaxnimate::ps
Q_DECLARE_METATYPE(glaxnimate::ps::Value)
Q_DECLARE_METATYPE(glaxnimate::ps::ValueArray)
Q_DECLARE_METATYPE(glaxnimate::ps::ValueDict)

namespace glaxnimate::ps {

#define VALUE_TYPE_BIN(Tp, Type) \
template<> struct Value::type_for<Tp> { \
    using type = Type; \
    static constexpr QMetaType meta_type() noexcept { return QMetaType::fromType<Type>(); } \
};
#define VALUE_TYPE_FOR(Tp, Type) \
template<> struct Value::type_for<Tp> { \
        using type = Type; \
        static constexpr QMetaType meta_type() noexcept { return QMetaType::fromType<Type>(); } \
};
#define VALUE_TYPE_NUL(Tp) \
template<>inline Value Value::from<Tp>() { return Value(Tp, {}); } \
VALUE_TYPE_BIN(Tp, void)

VALUE_TYPE_BIN(Value::Integer, int)
VALUE_TYPE_BIN(Value::Real, float)
VALUE_TYPE_BIN(Value::Boolean, bool)
VALUE_TYPE_FOR(Value::Array, ValueArray)
VALUE_TYPE_BIN(Value::String, QString)
VALUE_TYPE_FOR(Value::Dictionary, ValueDict)
VALUE_TYPE_NUL(Value::Mark)
VALUE_TYPE_NUL(Value::Null)
VALUE_TYPE_NUL(Value::File)
VALUE_TYPE_NUL(Value::Save)

#undef VALUE_TYPE_FOR
#undef VALUE_TYPE_BIN
#undef VALUE_TYPE_NUL


} // namespace glaxnimate::ps
