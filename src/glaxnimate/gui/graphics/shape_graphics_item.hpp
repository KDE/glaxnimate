#pragma once
#include "document_node_graphics_item.hpp"

#include "glaxnimate/core/model/shapes/shape.hpp"

namespace glaxnimate::gui::graphics {

class ShapeGraphicsItem : public DocumentNodeGraphicsItem
{
public:
    explicit ShapeGraphicsItem(model::ShapeElement* node, QGraphicsItem* parent = nullptr)
        : DocumentNodeGraphicsItem(node, parent)
    {
        setBoundingRegionGranularity(1);
    }

    QPainterPath shape() const override
    {
        return shape_element()->to_painter_path(shape_element()->time());
    }

    model::ShapeElement* shape_element() const
    {
        return static_cast<model::ShapeElement*>(node());
    }
};

} // namespace glaxnimate::gui::graphics
