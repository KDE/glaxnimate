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

class ValueArray;


class String
{
public:
    using container = QByteArray;
    using value_type = container::value_type;
    using iterator = container::iterator;
    using const_iterator = container::const_iterator;
    using reference = container::reference;
    using const_reference = container::const_reference;
    using size_type = int;

    String(const char* data)
        : data(std::make_shared<container>(data))
    {}
    String(const QByteArray& data)
        : data(std::make_shared<container>(data))
    {}

    String() : data(std::make_shared<container>()) {}
    String(const String&) = default;
    String(String&&) = default;
    String& operator=(const String&) = default;
    String& operator=(String&&) = default;

    size_type size() const { return data->size(); }
    bool empty() const { return data->isEmpty(); }

    reference operator[](size_type index) { return (*data)[index]; }
    const_reference operator[](size_type index) const { return (*data)[index]; }

    iterator begin() { return data->begin(); }
    iterator end() { return data->end(); }
    iterator begin() const { return data->begin(); }
    iterator end() const { return data->end(); }

    QString to_string() const;
    /**
     * @brief formats a string into a postscript string
     */
    QByteArray to_string_literal() const;
    friend QDebug operator<<(QDebug dbg, const String& arr) { return dbg << *arr.data; }


    bool operator==(const String& oth) const
    {
        return *data == *oth.data;
    }

    bool operator!=(const String& oth) const
    {
        return !(*this == oth);
    }

    const QByteArray& bytes() const { return *data; }

private:
    std::shared_ptr<container> data;
};

class Value
{
    Q_GADGET

public:
    enum Type
    {
        Integer,
        Real,
        Boolean,
        Array,
        PackedArray = Array,
        String,
        Dict,
        File, // TODO
        Mark,
        Null,
        Save,
    };

    Q_ENUM(Type)

    enum Attribute
    {
        None = 0,
        Execute = 1,
        Write = 2,
        Read = 4
    };

    template<Type Tp> struct type_for;

    Value() : Value(Null, {}) {}

    Value(int num) : Value(Integer, num) {}
    Value(float num) : Value(Real, num) {}
    Value(double num) : Value(Real, num) {}
    Value(bool num) : Value(Boolean, num) {}
    Value(ValueArray v);
    Value(const char* str);
    Value(class String v);
    Value(const QByteArray &v);
    Value(const Value&) = default;
    Value(Value&&) = default;
    Value& operator=(const Value&) = default;
    Value& operator=(Value&&) = default;

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
        if ( type_ == String )
            return cast<ps::String>().to_string();

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
            CASE(Dict)
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

    bool operator==(const Value& oth) const
    {
        return value_ == oth.value_;
    }

    bool operator!=(const Value& oth) const
    {
        return value_ != oth.value_;
    }

    friend QDebug& operator<<(QDebug& dbg, const Value& v) { return dbg << v.to_pretty_string(); }

    static QString type_name(Type t);

private:
    Value(Type type, QVariant value) : type_(type), value_(std::move(value)) {}

    Type type_;
    QVariant value_;
    int attributes_ = None;
};

/**
 * @brief Arrays share data hence this wrapper around a shared pointer to vector
 */
class ValueArray
{
public:
    using container = std::vector<Value>;
    using value_type = container::value_type;
    using iterator = container::iterator;
    using const_iterator = container::const_iterator;
    using reference = container::reference;
    using const_reference = container::const_reference;
    using size_type = int;

    /*template<class... Args>
    ValueArray(Args&&... args)
        : data(std::make_shared<container>(std::forward<Args>(args)...))
    {}*/

    ValueArray(std::initializer_list<Value> init)
        : data(std::make_shared<container>(std::move(init)))
    {}

    ValueArray() : data(std::make_shared<container>()) {}
    ValueArray(const ValueArray&) = default;
    ValueArray(ValueArray&&) = default;
    ValueArray& operator=(const ValueArray&) = default;
    ValueArray& operator=(ValueArray&&) = default;

    void resize(size_type sz) { data->resize(sz); }
    void reserve(size_type sz) { data->reserve(sz); }
    size_type size() const { return data->size(); }
    bool empty() const { return data->empty(); }

    reference operator[](size_type index) { return (*data)[index]; }
    const_reference operator[](size_type index) const { return (*data)[index]; }
    template<class... Args>
    void emplace_back(Args&&... args)
    {
        data->emplace_back(std::forward<Args>(args)...);
    }

    iterator begin() { return data->begin(); }
    iterator end() { return data->end(); }
    iterator begin() const { return data->begin(); }
    iterator end() const { return data->end(); }

    QString to_pretty_string() const;
    friend QDebug operator<<(QDebug dbg, const ValueArray& arr) { return dbg << arr.to_pretty_string(); }


    bool operator==(const ValueArray& oth) const
    {
        if ( size() != oth.size() )
            return false;
        for ( int i = 0; i < size(); i++ )
            if ( (*data)[i] != oth[i] )
                return false;
        return true;
    }

    bool operator!=(const ValueArray& oth) const
    {
        return !(*this == oth);
    }

private:
    std::shared_ptr<container> data;
};

using ValueDict = std::map<QString, glaxnimate::ps::Value>;
using ValueType = Value;

} // namespace glaxnimate::ps
Q_DECLARE_METATYPE(glaxnimate::ps::Value)
Q_DECLARE_METATYPE(glaxnimate::ps::ValueArray)
Q_DECLARE_METATYPE(glaxnimate::ps::String)
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
VALUE_TYPE_BIN(Value::String, ps::String)
VALUE_TYPE_FOR(Value::Dict, ValueDict)
VALUE_TYPE_NUL(Value::Mark)
VALUE_TYPE_NUL(Value::Null)
VALUE_TYPE_NUL(Value::File)
VALUE_TYPE_NUL(Value::Save)

#undef VALUE_TYPE_FOR
#undef VALUE_TYPE_BIN
#undef VALUE_TYPE_NUL


} // namespace glaxnimate::ps
