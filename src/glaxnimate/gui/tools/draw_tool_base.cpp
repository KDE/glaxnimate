#include "draw_tool_base.hpp"


void glaxnimate::gui::tools::DrawToolBase::draw_shape(const PaintEvent& event, const QPainterPath& path)
{
    ShapeToolWidget* options = widget();

    if ( !options->create_stroke() && !options->create_fill() )
    {
        event.painter->setBrush(Qt::transparent);

        //event.painter->setCompositionMode(QPainter::CompositionMode_Difference);
        QPen p(Qt::white, 1);
        p.setCosmetic(true);
        p.setDashPattern({4., 4.});
        event.painter->setPen(p);
        event.painter->drawPath(path);

        //event.painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
        p.setDashOffset(4);
        p.setColor(Qt::black);
        event.painter->setPen(p);
        event.painter->drawPath(path);
    }
    else
    {
        if ( options->create_fill() )
            event.painter->setBrush(event.window->current_color());
        else
            event.painter->setBrush(Qt::transparent);

        if ( options->create_stroke() )
        {
            QPen p = event.window->current_pen_style();
            p.setWidthF(p.widthF() * event.window->current_zoom());
            event.painter->setPen(p);
        }
        else
        {
            event.painter->setPen(Qt::NoPen);
        }

        event.painter->drawPath(path);
    }

}

void glaxnimate::gui::tools::DrawToolBase::create_shape(
    const QString& command_name,
    const Event& event,
    std::unique_ptr<model::ShapeElement> shape
)
{
    auto document = event.window->document();
    command::UndoMacroGuard macro(command_name, document);

    model::VisualNode* select = shape.get();

    QString name = document->get_best_name(shape.get(), shape->name.get());

    model::ShapeListProperty* prop = get_container(event.window);

    ShapeToolWidget* options = widget();
    int index = prop->size();

    // Layer
    if ( options->create_layer() )
    {
        std::unique_ptr<model::Group> layer = std::make_unique<model::Layer>(document);
        layer->name.set(name);
        model::ShapeListProperty* group_container = &event.window->document()->main()->shapes;
        select = layer.get();
        prop = &layer->shapes;
        document->undo_stack().push(
            new command::AddShape(group_container, std::move(layer), group_container->size())
        );
        index = 0;
    }

    if ( options->create_layer() || options->create_group() )
    {
        // Group
        std::unique_ptr<model::Group> group = std::make_unique<model::Group>(document);
        group->name.set(name);
        model::ShapeListProperty* group_container = prop;
        prop = &group->shapes;
        if ( !options->create_layer() )
            select = group.get();
        model::VisualNode* owner_node = static_cast<model::VisualNode*>(group_container->object());
        QTransform parent_t = owner_node->transform_matrix(owner_node->time());
        QTransform parent_t_inv = parent_t.inverted();
        group->transform.get()->set_transform_matrix(parent_t_inv);

        QPointF center = shape->local_bounding_rect(0).center();
        group->transform.get()->anchor_point.set(center);
        group->transform.get()->position.set(parent_t_inv.map(center));

        document->undo_stack().push(
            new command::AddShape(group_container, std::move(group), index)
        );
        index = 0;

        // Fill
        auto fill = std::make_unique<model::Fill>(document);
        fill->name.set(QObject::tr("Fill"));
        fill->color.set(event.window->current_color());
        fill->use.set(event.window->linked_brush_style(false));
        fill->visible.set(options->create_fill());

        document->undo_stack().push(
            new command::AddShape(prop, std::move(fill), index)
        );
        index++;

        // Stroke
        auto stroke = std::make_unique<model::Stroke>(document);
        stroke->name.set(QObject::tr("Stroke"));
        stroke->set_pen_style(event.window->current_pen_style());
        stroke->use.set(event.window->linked_brush_style(true));
        stroke->visible.set(options->create_stroke());

        document->undo_stack().push(
            new command::AddShape(prop, std::move(stroke), index)
        );
        index++;
    }

    shape->name.set(name);
    document->undo_stack().push(
        new command::AddShape(prop, std::move(shape), index)
    );

    event.window->set_current_document_node(select);
}

void glaxnimate::gui::tools::DrawToolBase::shape_style_change_event(const Event& event)
{
    event.repaint();
}



