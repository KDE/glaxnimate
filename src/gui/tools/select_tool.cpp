/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "base.hpp"

#include <variant>

#include "model/shapes/path.hpp"
#include "command/structure_commands.hpp"
#include "command/animation_commands.hpp"
#include "math/geom.hpp"

#ifndef Q_OS_ANDROID
    #include "widgets/dialogs/glaxnimate_window.hpp"
    #include "widgets/menus/node_menu.hpp"
    #include "handle_menu.hpp"
#endif

namespace glaxnimate::gui::tools {

class SelectTool : public Tool
{
public:
    QString id() const override { return "select"; }
    QIcon icon() const override { return QIcon::fromTheme("edit-select"); }
    QString name() const override { return QObject::tr("Select"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F1"), QKeySequence::PortableText); }
    static int static_group() noexcept { return Registry::Core; }
    int group() const noexcept override { return static_group(); }

private:
    enum DragMode
    {
        None,
        Click,
        RubberBand,
        ForwardEvents,
        DrawSelect,
        DragObject,
    };

    struct DragObjectData
    {
        template<class Prop>
        struct PropData
        {
            Prop* property;
            typename Prop::value_type start_value;
        };
        using Variant = std::variant<
            PropData<model::AnimatedProperty<QPointF>>,
            PropData<model::AnimatedProperty<math::bezier::Bezier>>
        >;

        template<class T>
        DragObjectData(model::VisualNode* node, T* property, const QPointF& scene_pos)
        : transform(node->docnode_fuzzy_parent()->transform_matrix(node->time()).inverted()),
          data(PropData<T>{property, property->get()}),
          start_point(transform.map(scene_pos))
        {}

        static void push(model::VisualNode* node, const QPointF& scene_pos, std::vector<DragObjectData>& out)
        {
            if ( auto prop = node->get_property("position") )
                out.push_back(DragObjectData(node, static_cast<model::AnimatedProperty<QPointF>*>(prop), scene_pos));
            if ( auto prop = node->get_property("transform") )
                out.push_back(DragObjectData(
                    node,
                    &static_cast<model::SubObjectProperty<model::Transform>*>(prop)->get()->position,
                    scene_pos)
                );
            else if ( auto shape = qobject_cast<model::Path*>(node) )
                out.push_back(DragObjectData(node, &shape->shape, scene_pos));
        }

        void drag(const QPointF& dragged_to, command::SetMultipleAnimated* cmd) const
        {
            QPointF delta = transform.map(dragged_to) - start_point;


            if ( data.index() == 0 )
            {
                cmd->push_property(std::get<0>(data).property, std::get<0>(data).start_value + delta);
                return;
            }

            math::bezier::Bezier new_bezier = std::get<1>(data).start_value;
            for ( auto& point : new_bezier )
                point.translate(delta);

            cmd->push_property(std::get<1>(data).property, QVariant::fromValue(new_bezier));
        }

        model::Document* doc() const
        {
            if ( data.index() == 0 )
                return std::get<0>(data).property->object()->document();
            return std::get<1>(data).property->object()->document();
        }

        QTransform transform;
        Variant data;
        QPointF start_point;
    };

    void do_drag(QPointF scene_pos, Qt::KeyboardModifiers mods, bool commit)
    {
        if ( drag_data.empty() )
            return;

        rubber_p2 = scene_pos;
        if ( mods & Qt::ControlModifier )
        {
            std::array<QPointF, 2> directions = {QPointF(1, 0), QPointF(0, 1)};
            QPointF best;
            qreal min_dist = std::numeric_limits<qreal>::max();
            for ( const auto& dir : directions )
            {
                auto p = math::line_closest_point(rubber_p1, rubber_p1 + dir, scene_pos);
                qreal dist = math::length_squared(p - scene_pos);
                if ( dist < min_dist )
                {
                    min_dist = dist;
                    best = p;
                }
            }
            scene_pos = best;
        }

        auto cmd = new command::SetMultipleAnimated(tr("Drag"), commit);
        model::Document* doc = drag_data[0].doc();

        for ( const auto& dragger : drag_data )
            dragger.drag(scene_pos, cmd);

        doc->push_command(cmd);
    }

    void mouse_press(const MouseEvent& event) override
    {
        if ( event.press_button == Qt::LeftButton )
        {
            drag_data.clear();

            if ( event.modifiers() & Qt::AltModifier )
            {
                drag_mode = DrawSelect;
                draw_path.moveTo(event.scene_pos);
                return;
            }

            drag_mode = Click;
            rubber_p1 = event.event->localPos();

            auto selection_mode = event.modifiers() & Qt::ControlModifier ? SelectionMode::Shape : SelectionMode::Group;
            auto clicked_on = under_mouse(event, true, selection_mode);
            if ( clicked_on.handle )
            {
                drag_mode = ForwardEvents;
                event.forward_to_scene();
                return;
            }
            else if ( !clicked_on.nodes.empty() )
            {
                rubber_p1 = event.scene_pos;
                replace_selection = nullptr;

                bool drag_selection = false;

                for ( auto node : clicked_on.nodes )
                {
                    if ( event.scene->is_descendant_of_selection(node->node()) )
                    {
                        drag_selection = true;
                        break;
                    }
                }

                if ( drag_selection )
                {
                    for ( auto node : event.scene->cleaned_selection() )
                        DragObjectData::push(node, event.scene_pos, drag_data);
                }
                else
                {
                    replace_selection = clicked_on.nodes[0]->node();
                    DragObjectData::push(clicked_on.nodes[0]->node(), event.scene_pos, drag_data);
                }
            }

        }
    }

    void mouse_move(const MouseEvent& event) override
    {
        if ( event.press_button == Qt::LeftButton )
        {
            switch ( drag_mode )
            {
                case None:
                    break;
                case ForwardEvents:
                    event.forward_to_scene();
                    break;
                case DrawSelect:
                    draw_path.lineTo(event.scene_pos);
                    break;
                case Click:
                    if ( !drag_data.empty() )
                    {
                        if ( replace_selection )
                        {
                            event.scene->user_select({replace_selection}, graphics::DocumentScene::Replace);
                            replace_selection = nullptr;
                        }
                        drag_mode = DragObject;
                    }
                    else
                    {
                        drag_mode = RubberBand;
                    }
                    mouse_move(event);
                    break;
                case RubberBand:
                    rubber_p2 = event.event->localPos();
                    break;
                case DragObject:
                    do_drag(event.scene_pos, event.modifiers(), false);
                    break;
            }
        }
    }

    void complex_select(const MouseEvent& event, const std::vector<graphics::DocumentNodeGraphicsItem*>& items)
    {

        auto mode = graphics::DocumentScene::Replace;
        if ( event.modifiers() & Qt::ShiftModifier )
            mode = graphics::DocumentScene::Append;

        auto selection_mode = event.modifiers() & Qt::ControlModifier ? SelectionMode::Shape : SelectionMode::Group;

        std::vector<model::VisualNode*> selection;
        for ( auto item : items )
        {
            if ( item->node()->docnode_selectable() && item->selection_mode() >= selection_mode )
                selection.push_back(item->node());
        }

        event.scene->user_select(selection, mode);
    }

    void mouse_release(const MouseEvent& event) override
    {
        if ( event.button() == Qt::LeftButton )
        {
            switch ( drag_mode )
            {
                case None:
                    break;
                case ForwardEvents:
                    event.forward_to_scene();
                    break;
                case DrawSelect:
                    draw_path.lineTo(event.scene_pos);
                    complex_select(event, event.scene->nodes(draw_path, event.view->viewportTransform()));
                    draw_path = {};
                    event.view->viewport()->update();
                    break;
                case RubberBand:
                    rubber_p2 = event.event->localPos();
                    complex_select(event, event.scene->nodes(
                        event.view->mapToScene(QRect(rubber_p1.toPoint(), rubber_p2.toPoint()).normalized()),
                        event.view->viewportTransform(),
                        Qt::ContainsItemShape
                    ));
                    drag_mode = None;
                    event.view->viewport()->update();
                    break;
                case Click:
                {
                    replace_selection = nullptr;

                    std::vector<model::VisualNode*> selection;

                    auto selection_mode = event.modifiers() & Qt::ControlModifier ? SelectionMode::Shape : SelectionMode::Group;
                    auto nodes = under_mouse(event, true, selection_mode).nodes;
                    if ( !nodes.empty() )
                        selection.push_back(nodes[0]->node());

                    auto mode = graphics::DocumentScene::Replace;
                    if ( event.modifiers() & Qt::ShiftModifier )
                        mode = graphics::DocumentScene::Toggle;

                    event.scene->user_select(selection, mode);
                }
                break;
                case DragObject:
                    do_drag(event.scene_pos, event.modifiers(), true);
                    break;
            }

            replace_selection = nullptr;
            drag_data.clear();
            drag_mode = None;
        }
        else if ( event.press_button == Qt::RightButton )
        {
            context_menu(event);
        }
    }

    void mouse_double_click(const MouseEvent& event) override
    {
        edit_clicked(event);
    }

    void key_press(const KeyEvent& event) override
    {
        if ( drag_mode == DragObject && event.key() == Qt::Key_Control )
        {
            do_drag(rubber_p2, event.modifiers(), false);
        }
    }


    void key_release(const KeyEvent& event) override
    {
        if ( drag_mode == None && (event.key() == Qt::Key_Backspace) )
        {
            event.window->delete_selected();
            event.accept();
        }
        else if ( drag_mode == DragObject && event.key() == Qt::Key_Control )
        {
            do_drag(rubber_p2, event.modifiers(), false);
        }
    }

    void paint(const PaintEvent& event) override
    {
        if ( !selected_shapes.empty() )
        {
            QColor select_color = event.view->palette().color(QPalette::Highlight);
            QPen pen(select_color, 1);
            event.painter->setPen(pen);
            event.painter->setBrush(Qt::NoBrush);
            for ( auto shape : selected_shapes )
            {
                QPainterPath p;
                shape->to_bezier(shape->time()).add_to_painter_path(p);
                QTransform trans = shape->transform_matrix(shape->time()) * event.view->viewportTransform();
                event.painter->drawPath(trans.map(p));
            }
        }

        if ( drag_mode == DrawSelect )
        {
            event.painter->setTransform(event.view->viewportTransform());
            event.painter->setBrush(Qt::transparent);
            QPen pen(event.view->palette().color(QPalette::Highlight), 2);
            pen.setCosmetic(true);
            event.painter->setPen(pen);
            event.painter->drawPath(draw_path);
        }
        else if ( drag_mode == RubberBand )
        {
            event.painter->setBrush(Qt::transparent);
            QColor select_color = event.view->palette().color(QPalette::Highlight);
            QPen pen(select_color, 1);
            pen.setCosmetic(true);
            event.painter->setPen(pen);
            select_color.setAlpha(128);
            event.painter->setBrush(select_color);
            event.painter->drawRect(QRectF(rubber_p1, rubber_p2));
        }
    }

    void enable_event(const Event&) override { selected_shapes.clear(); }
    void disable_event(const Event&) override { selected_shapes.clear(); }

    QCursor cursor() override { return Qt::ArrowCursor; }

    void context_menu(const MouseEvent& event)
    {
#ifndef MOBILE_UI
        auto window = static_cast<GlaxnimateWindow*>(event.window);

        auto targets = under_mouse(event, true, graphics::DocumentNodeGraphicsItem::None);

        QMenu menu;
        auto undo_stack = &window->undo_group();
        menu.addAction(
            QIcon::fromTheme("edit-undo"),
            GlaxnimateWindow::tr("Undo %1").arg(undo_stack->undoText()),
            undo_stack, &QUndoGroup::undo
        )->setEnabled(undo_stack->canUndo());
        menu.addAction(
            QIcon::fromTheme("edit-redo"),
            GlaxnimateWindow::tr("Redo %1").arg(undo_stack->redoText()),
            undo_stack, &QUndoGroup::redo
        )->setEnabled(undo_stack->canRedo());

        menu.addSeparator();
        menu.addAction(QIcon::fromTheme("edit-cut"), GlaxnimateWindow::tr("Cut"),
                       window, &GlaxnimateWindow::cut);
        menu.addAction(QIcon::fromTheme("edit-copy"), GlaxnimateWindow::tr("Copy"),
                       window, &GlaxnimateWindow::copy);
        menu.addAction(QIcon::fromTheme("edit-paste"), GlaxnimateWindow::tr("Paste"),
                       window, &GlaxnimateWindow::paste);

        menu.addSeparator();
        menu.addAction(QIcon::fromTheme("edit-delete-remove"), GlaxnimateWindow::tr("Delete"),
                       window, &GlaxnimateWindow::delete_selected);


        menu.addSeparator();
        menu.addAction(QIcon::fromTheme("object-group"), GlaxnimateWindow::tr("Group Shapes"),
                       window, &GlaxnimateWindow::group_shapes);

        menu.addAction(QIcon::fromTheme("object-ungroup"), GlaxnimateWindow::tr("Ungroup Shapes"),
                       window, &GlaxnimateWindow::ungroup_shapes);

        menu.addSeparator();
        menu.addAction(QIcon::fromTheme("selection-move-to-layer-above"), GlaxnimateWindow::tr("Move to..."),
                       window, &GlaxnimateWindow::move_to);


        menu.addSeparator();

        model::DocumentNode* preferred = window->current_shape();

        for ( auto item : targets.nodes )
        {
            auto obj_menu = new NodeMenu(item->node(), window, &menu);
            if ( item->node() == preferred )
                preferred = nullptr;
            if ( obj_menu->actions().size() > 1 )
                menu.addAction(obj_menu->menuAction());
            else
                delete obj_menu;
        }

        if ( preferred )
            menu.addAction((new NodeMenu(preferred, window, &menu))->menuAction());

        if ( targets.handle )
            add_property_menu_actions(&menu, targets.handle, event.window);


        menu.exec(QCursor::pos());
#else
        Q_UNUSED(event);
#endif
    }

    QWidget* on_create_widget() override
    {
        return new QWidget();
    }

    void on_selected(graphics::DocumentScene * scene, model::VisualNode * node) override
    {
        if ( node->has("transform") || node->has("position") || node->is_instance<model::Composition>() )
        {
            scene->show_editors(node);
        }
        else if ( auto shape = node->cast<model::Shape>() )
        {
            selected_shapes.push_back(shape);
            scene->invalidate(
                shape->transform_matrix(shape->time())
                .map(shape->local_bounding_rect(shape->time()))
                .boundingRect()
            );
        }
    }

    void on_deselected(graphics::DocumentScene * scene, model::VisualNode * node) override
    {
        Tool::on_deselected(scene, node);

        if ( auto shape = node->cast<model::Shape>() )
        {
            selected_shapes.erase(std::remove(selected_shapes.begin(), selected_shapes.end(), shape), selected_shapes.end());
            scene->invalidate(
                shape->transform_matrix(shape->time())
                .map(shape->local_bounding_rect(shape->time()))
                .boundingRect()
            );
        }
    }

    void close_document_event(const glaxnimate::gui::tools::Event &) override
    {
        drag_data.clear();
        replace_selection = nullptr;
        selected_shapes.clear();
    }

    DragMode drag_mode;
    QPainterPath draw_path;
    QPointF rubber_p1;
    QPointF rubber_p2;
    std::vector<DragObjectData> drag_data;
    model::VisualNode* replace_selection = nullptr;
    std::vector<model::Shape*> selected_shapes;

    static Autoreg<SelectTool> autoreg;
};


tools::Autoreg<tools::SelectTool> tools::SelectTool::autoreg{max_priority};

} // namespace glaxnimate::gui::tools
