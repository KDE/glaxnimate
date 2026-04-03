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
        Procedure,
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
        return Value(Tp, std::move(val));
    }
    template<Type Tp> static Value from();

    Type type() const noexcept { return type_; }

    const QVariant& value() const { return value_; }

    QString to_string() const { return value_.toString(); }

    template<Type Tp>
    bool can_convert() const
    {
        return value_.canConvert(type_for<Tp>::meta_type());
    }

    template<class T>
    T cast() const
    {
        return qvariant_cast<T>(value_);
    }

private:
    Value(Type type, QVariant value) : type_(type), value_(std::move(value)) {}

    Type type_;
    QVariant value_;
};

struct Token
{
    enum Type
    {
        Eof,
        Literal,
        Comment,
        Operator,
        Unrecoverable
    };

    Type type;
    Value value;
};

using ValueArray = std::vector<glaxnimate::ps::Value>;
using ValueDict = std::map<QString, glaxnimate::ps::Value>;
using Procedure = std::vector<Token>;

} // namespace glaxnimate::ps
Q_DECLARE_METATYPE(glaxnimate::ps::Value)
Q_DECLARE_METATYPE(glaxnimate::ps::ValueArray)
Q_DECLARE_METATYPE(glaxnimate::ps::ValueDict)
Q_DECLARE_METATYPE(glaxnimate::ps::Procedure)
namespace glaxnimate::ps {

#define VALUE_TYPE_BIN(Tp, Type, meta) \
template<> struct Value::type_for<Tp> { \
    using type = Type; \
    static constexpr int meta_type() noexcept { return meta; } \
};
#define VALUE_TYPE_FOR(Tp, Type) \
template<> struct Value::type_for<Tp> { \
        using type = Type; \
        static constexpr int meta_type() noexcept { return qMetaTypeId<Type>(); } \
};
#define VALUE_TYPE_NUL(Tp) \
template<>Value Value::from<Tp>() { return Value(Tp, {}); } \
VALUE_TYPE_BIN(Tp, void, QMetaType::UnknownType)

VALUE_TYPE_BIN(Value::Integer, int, QMetaType::Int)
VALUE_TYPE_BIN(Value::Real, float, QMetaType::Float)
VALUE_TYPE_BIN(Value::Boolean, bool, QMetaType::Bool)
VALUE_TYPE_FOR(Value::Array, ValueArray)
VALUE_TYPE_BIN(Value::String, QString, QMetaType::QString)
VALUE_TYPE_FOR(Value::Dictionary, ValueDict)
VALUE_TYPE_FOR(Value::Procedure, ps::Procedure)
VALUE_TYPE_NUL(Value::Mark)
VALUE_TYPE_NUL(Value::Null)

#undef VALUE_TYPE_FOR
#undef VALUE_TYPE_BIN
#undef VALUE_TYPE_NUL


} // namespace glaxnimate::ps
