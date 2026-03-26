/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "glaxnimate/model/property/sub_object_property.hpp"
#include "glaxnimate/model/animation/animatable.hpp"

namespace glaxnimate::model {


inline void all_animated_properties(model::Object* object, std::vector<AnimatedPropertyBase*>& out)
{
    for ( auto prop : object->properties() )
    {
        if ( prop->traits().flags & PropertyTraits::Animated )
        {
            out.push_back(static_cast<AnimatedPropertyBase*>(prop));
        }
        else if ( prop->traits().type == PropertyTraits::Object )
        {
            if ( !(prop->traits().flags & PropertyTraits::List) )
                all_animated_properties(static_cast<SubObjectPropertyBase*>(prop)->sub_object(), out);
        }
    }
}

inline std::vector<AnimatedPropertyBase*> all_animated_properties(model::Object* object)
{
    std::vector<AnimatedPropertyBase*> out;
    all_animated_properties(object, out);
    return out;
}


} // namespace glaxnimate::model
