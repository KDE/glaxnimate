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

class Value;

class String
{
public:
    using container = QByteArray;
    using value_type = quint8;
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

    std::size_t hash() const { return qHash(*data); }

    bool starts_with(const String& prefix) const
    {
        return data->startsWith(*prefix.data);
    }

private:
    std::shared_ptr<container> data;
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
    reference back() { return data->back(); }

    iterator begin() { return data->begin(); }
    iterator end() { return data->end(); }
    iterator begin() const { return data->begin(); }
    iterator end() const { return data->end(); }

    QString to_pretty_string(bool as_executable=false) const;
    friend QDebug operator<<(QDebug dbg, const ValueArray& arr) { return dbg << arr.to_pretty_string(); }


    bool operator==(const ValueArray& oth) const;

    bool operator!=(const ValueArray& oth) const
    {
        return !(*this == oth);
    }

    QByteArray hash_key() const { return "ARRAY_KEY " + QByteArray::number(reinterpret_cast<std::uintptr_t>(data.get())); }

    bool shallow_equal(const ValueArray& oth) const { return data == oth.data; }

private:
    std::shared_ptr<container> data;
};


class ValueDict
{
public:
    using container = std::unordered_map<QByteArray, Value>;
    using value_type = container::value_type;
    using key_type = container::key_type;
    using mapped_type = container::mapped_type;
    using iterator = container::iterator;
    using const_iterator = container::const_iterator;
    using reference = container::reference;
    using const_reference = container::const_reference;
    using size_type = int;

    ValueDict(std::initializer_list<value_type> init)
        : data(std::make_shared<container>(std::move(init)))
    {}

    ValueDict() : data(std::make_shared<container>()) {}
    ValueDict(const ValueDict&) = default;
    ValueDict(ValueDict&&) = default;
    ValueDict& operator=(const ValueDict&) = default;
    ValueDict& operator=(ValueDict&&) = default;

    size_type size() const { return data->size(); }
    bool empty() const { return data->empty(); }

    mapped_type& operator[](const key_type& index);
    const mapped_type& operator[](const key_type& index) const;

    void put(const key_type& key, Value val);

    iterator begin() { return data->begin(); }
    iterator end() { return data->end(); }
    const_iterator begin() const { return data->begin(); }
    const_iterator end() const { return data->end(); }
    iterator find(const key_type& v) { return data->find(v); }
    const_iterator find(const key_type& v) const { return data->find(v); }

    void erase(const key_type& key)
    {
        data->erase(key);
    }

    QString to_pretty_string() const;
    friend QDebug operator<<(QDebug dbg, const ValueDict& arr) { return dbg << arr.to_pretty_string(); }

    bool operator==(const ValueDict& oth) const;

    bool operator!=(const ValueDict& oth) const
    {
        return !(*this == oth);
    }

    std::size_t hash() const { return std::hash<void*>()(data.get()); }

    /**
     * @brief Helper for the load command
     * @param key Key to find
     * @param out Value to write into
     * @return true if the value has been found
     */
    bool load_into(const key_type &key, Value &out) const;

    /**
     * @brief Helper for the store command
     * Sets the value at @p key if it already exists, otherwise returns false
     */
    bool store(const key_type &key, const Value &value);

    bool contains(const key_type& key) const
    {
        return data->count(key) > 0;
    }

    bool shallow_equal(const ValueDict& oth) const;


    QByteArray hash_key() const { return "DICT_KEY " + QByteArray::number(reinterpret_cast<std::uintptr_t>(data.get())); }

private:
    std::shared_ptr<container> data;
};


class Value
{
    Q_GADGET

private:
    using Variant = std::variant<
        int,
        float,
        bool,
        ValueArray,
        ps::String,
        ps::ValueDict
    >;

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
        Executable = 1,
        Writable = 2,
        Readable = 4,
        Deferred = 8,
        Name = 16
    };

    template<Type Tp> struct type_for;
    template<class Ty> struct index_for;
    template <class To, Type Tp> struct disabled_converter
    {
        static constexpr const bool enabled = false;
        static To convert(const Variant&) { return {}; }
    };
    template <class To, Type Tp> struct enabled_converter
    {
        static constexpr const bool enabled = true;
        static To convert(const Variant& v) { return std::get<typename type_for<Tp>::type>(v); }
    };
    template <class To, Type Tp> struct converter : disabled_converter<To, Tp> {};


    Value() : Value(Null, {}, 0) {}

    Value(int num) : Value(Integer, num, 0) {}
    Value(float num) : Value(Real, num, 0) {}
    Value(double num) : Value(Real, float(num), 0) {}
    Value(bool num) : Value(Boolean, num, 0) {}
    Value(ValueArray v);
    Value(const char* str);
    Value(class String v);
    Value(const QByteArray &v);
    Value(ValueDict v);
    Value(const Value&) = default;
    Value(Value&&) = default;
    Value& operator=(const Value&) = default;
    Value& operator=(Value&&) = default;

    template<Type Tp>
    static Value from(typename type_for<Tp>::type val, int attributes = None)
    {
        return Value(Tp, std::move(val), attributes);
    }
    template<Type Tp> static Value from();

    Type type() const noexcept { return type_; }

    // const QVariant& value() const { return value_; }

    QString to_string() const;

    QString to_pretty_string() const;

    template<class T>
    T cast() const
    {
        if ( value_.index() == index_for<T>::index )
            return std::get<T>(value_);
        if ( type_ == Real && converter<T, Real>::enabled )
            return converter<T, Real>::convert(value_);
        if ( type_ == Integer && converter<T, Integer>::enabled )
            return converter<T, Integer>::convert(value_);
        return {};
    }

    bool can_convert(Type t) const;

    void set_attributes(int attributes)
    {
        attributes_ = attributes;
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
        if ( !can_convert(oth.type_) )
            return false;
        if ( type_ == Real || oth.type_ == Real )
            return qFuzzyCompare(cast<float>(), oth.cast<float>());
        return value_ == oth.value_;
    }

    bool operator!=(const Value& oth) const
    {
        return !(*this == oth);
    }

    bool shallow_equal(const Value& oth) const;

    friend QDebug& operator<<(QDebug& debug, const Value& v)
    {
        QDebugStateSaver saver(debug);
        return debug.noquote() << v.to_pretty_string();
    }

    static QByteArray type_name(Type t);
    QByteArray value_type_name() const;

    QByteArray hash_key() const;

private:
    Value(Type type, Variant value, int attributes) : type_(type), value_(std::move(value)), attributes_(attributes) {}

    Type type_;
    Variant value_;
    int attributes_ = None;
};

#define VALUE_TYPE_FOR_IMPL(Tp, Type) \
    template<> struct Value::type_for<Tp> { \
        using type = Type; \
    };

#define VALUE_TYPE_FOR(Tp, Type) \
    VALUE_TYPE_FOR_IMPL(Tp, Type) \
    template<> struct Value::index_for<Type> { \
        static constexpr const int index = int(Tp); \
    };

#define VALUE_TYPE_NUL(Tp) \
template<>inline Value Value::from<Tp>() { return Value(Tp, {}, 0); } \
VALUE_TYPE_FOR_IMPL(Tp, void)

VALUE_TYPE_FOR(Value::Integer, int)
VALUE_TYPE_FOR(Value::Real, float)
VALUE_TYPE_FOR(Value::Boolean, bool)
VALUE_TYPE_FOR(Value::Array, ValueArray)
VALUE_TYPE_FOR(Value::String, ps::String)
VALUE_TYPE_FOR(Value::Dict, ValueDict)
VALUE_TYPE_NUL(Value::Mark)
VALUE_TYPE_NUL(Value::Null)
VALUE_TYPE_NUL(Value::File)
VALUE_TYPE_NUL(Value::Save)

#undef VALUE_TYPE_FOR
#undef VALUE_TYPE_NUL

template<> struct Value::converter<int, Value::Real> : Value::enabled_converter<int, Value::Real> {};
template<> struct Value::converter<float, Value::Integer> : Value::enabled_converter<float, Value::Integer> {};

} // namespace glaxnimate::ps

