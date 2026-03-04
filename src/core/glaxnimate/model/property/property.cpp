/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/model/property/property.hpp"
#include "glaxnimate/model/object.hpp"
#include "glaxnimate/command/property_commands.hpp"

glaxnimate::model::BaseProperty::BaseProperty(Object* object, const utils::LazyLocalizedString& name, PropertyTraits traits)
    : object_(object), name_(name), traits_(traits)
{
    if ( object )
        object_->add_property(this);
}

void glaxnimate::model::BaseProperty::value_changed()
{
    object_->property_value_changed(this, value());
}

bool glaxnimate::model::BaseProperty::set_undoable ( const QVariant& val, bool commit )
{
    if ( !valid_value(val) )
        return false;

    object_->push_command(new command::SetPropertyValue(this, value(), val, commit));
    return true;
}
