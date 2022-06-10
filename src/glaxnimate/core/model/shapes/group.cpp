#include "group.hpp"

#include <QPainter>

#include "glaxnimate/core/model/document.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::Group)


glaxnimate::model::Group::Group(Document* document)
    : Ctor(document)
{
    connect(transform.get(), &Object::property_changed,
            this, &Group::on_transform_matrix_changed);
}

void glaxnimate::model::Group::on_paint(QPainter* painter, glaxnimate::model::FrameTime time, glaxnimate::model::VisualNode::PaintMode, glaxnimate::model::Modifier*) const
{
    painter->setOpacity(
        painter->opacity() * opacity.get_at(time)
    );
}

void glaxnimate::model::Group::on_transform_matrix_changed()
{
    propagate_bounding_rect_changed();
    emit local_transform_matrix_changed(local_transform_matrix(time()));
    propagate_transform_matrix_changed(transform_matrix(time()), group_transform_matrix(time()));
}

void glaxnimate::model::Group::add_shapes(glaxnimate::model::FrameTime t, math::bezier::MultiBezier & bez, const QTransform& parent_transform) const
{
    QTransform trans = transform.get()->transform_matrix(t) * parent_transform;
    for ( const auto& ch : utils::Range(shapes.begin(), shapes.past_first_modifier()) )
    {
        ch->add_shapes(t, bez, trans);
    }
}

QRectF glaxnimate::model::Group::local_bounding_rect(FrameTime t) const
{
    if ( shapes.empty() )
        return QRectF(QPointF(0, 0), document()->size());
    return shapes.bounding_rect(t);
}

QTransform glaxnimate::model::Group::local_transform_matrix(glaxnimate::model::FrameTime t) const
{
    return transform.get()->transform_matrix(t);
}

QPainterPath glaxnimate::model::Group::to_painter_path(glaxnimate::model::FrameTime t) const
{
    QPainterPath path;
    for ( const auto& ch : utils::Range(shapes.begin(), shapes.past_first_modifier()) )
    {
        path.addPath(ch->to_clip(t));
    }
    return path;
}


QPainterPath glaxnimate::model::Group::to_clip(FrameTime t) const
{
    return transform.get()->transform_matrix(t).map(to_painter_path(t));
}

std::unique_ptr<glaxnimate::model::ShapeElement> glaxnimate::model::Group::to_path() const
{
    auto clone = std::make_unique<glaxnimate::model::Group>(document());

    for ( BaseProperty* prop : properties() )
    {
        if ( prop != &shapes )
            clone->get_property(prop->name())->assign_from(prop);
    }

    for ( const auto& shape : shapes )
    {
        clone->shapes.insert(shape->to_path());
        if ( shape->is_instance<glaxnimate::model::Modifier>() )
            break;
    }

    return clone;
}
