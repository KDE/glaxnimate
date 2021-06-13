#pragma once

#include "shape.hpp"

#include "model/transform.hpp"
#include "model/property/sub_object_property.hpp"

namespace model {

class Repeater : public StaticOverrides<Repeater, Modifier>
{
    GLAXNIMATE_OBJECT(Repeater)
    GLAXNIMATE_SUBOBJECT(model::Transform, transform)
    GLAXNIMATE_ANIMATABLE(int, copies, 1)
    GLAXNIMATE_ANIMATABLE(float, start_opacity, 1, {}, 0, 1, false, PropertyTraits::Percent)
    GLAXNIMATE_ANIMATABLE(float, end_opacity, 1, {}, 0, 1, false, PropertyTraits::Percent)

public:
    using Ctor::Ctor;

    static QIcon static_tree_icon();
    static QString static_type_name_human();

protected:
    math::bezier::MultiBezier process(FrameTime t, const math::bezier::MultiBezier& mbez) const override;
    void on_paint(QPainter* p, FrameTime t, PaintMode) const override;
    bool process_collected() const override;

};

} // namespace model
