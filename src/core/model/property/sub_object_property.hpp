/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#include "model/object.hpp"
#include "model/property/property.hpp"

#define GLAXNIMATE_SUBOBJECT(type, name)                    \
public:                                                     \
    SubObjectProperty<type> name{this, kli18n(#name)};      \
    type* get_##name() { return name.get(); }               \
private:                                                    \
    Q_PROPERTY(type* name READ get_##name)                  \
    // macro end

namespace glaxnimate::model {

class SubObjectPropertyBase : public BaseProperty
{
public:
    SubObjectPropertyBase(Object* obj, const KLazyLocalizedString& name)
        : BaseProperty(obj, name, {PropertyTraits::Object})
    {}

    virtual const model::Object* sub_object() const = 0;
    virtual model::Object* sub_object() = 0;
};

template<class Type>
class SubObjectProperty : public SubObjectPropertyBase
{
public:
    SubObjectProperty(Object* obj, const KLazyLocalizedString& name)
        : SubObjectPropertyBase(obj, name),
        sub_obj(obj->document())
    {}

    const Type* operator->() const
    {
        return &sub_obj;
    }

    Type* operator->()
    {
        return &sub_obj;
    }

    QVariant value() const override
    {
        return QVariant::fromValue(const_cast<Type*>(&sub_obj));
    }

    bool valid_value(const QVariant & v) const override
    {
        return v.value<Type*>();
    }

    bool set_value(const QVariant& val) override
    {
        if ( !val.canConvert<Type*>() )
            return false;

        if ( Type* t = val.value<Type*>() )
            return set_clone(t);

        return false;
    }

    Type* set_clone(Type* object)
    {
        if ( !object )
            return nullptr;

        sub_obj.assign_from(object);
        return &sub_obj;
    }

    Type* get() { return &sub_obj; }
    const Type* get() const { return &sub_obj; }

    model::Object * sub_object() override { return &sub_obj; }
    const model::Object * sub_object() const override { return &sub_obj; }


    void set_time(FrameTime t) override
    {
        sub_obj.set_time(t);
    }

    void transfer(Document* doc) override
    {
        sub_obj.transfer(doc);
    }

    void stretch_time(qreal multiplier) override
    {
        sub_obj.stretch_time(multiplier);
    }

private:
    Type sub_obj;
};

} // namespace glaxnimate::model
