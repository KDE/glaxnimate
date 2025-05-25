/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate_window_p.hpp"
#include "widgets/shape_style/shape_style_preview_widget.hpp"


void GlaxnimateWindow::Private::scene_selection_changed(const std::vector<model::VisualNode*>& selected, const std::vector<model::VisualNode*>& deselected)
{
    if ( update_selection )
        return;

    selection_changed(selected, deselected);


    if ( !selected.empty() )
    {
        set_current_object(selected.back());
    }
    else
    {
        auto current = layers_dock->layer_view()->currentIndex();
        if ( current.isValid() && ! layers_dock->layer_view()->selectionModel()->isSelected(current) )
            set_current_object(nullptr);
    }
}

void GlaxnimateWindow::Private::timeline_current_node_changed(model::VisualNode* node)
{
    set_current_object(node);
}

void GlaxnimateWindow::Private::set_current_document_node(model::VisualNode* node)
{
    layers_dock->layer_view()->replace_selection(node);
    layers_dock->layer_view()->set_current_node(node);
}

void GlaxnimateWindow::Private::set_current_object(model::DocumentNode* node)
{
    if ( update_selection )
        return;

    auto lock = update_selection.get_lock();

    current_node = node;

    model::Stroke* stroke = nullptr;
    model::Fill* fill = nullptr;

    if ( node )
    {
        stroke = qobject_cast<model::Stroke*>(node);
        fill = qobject_cast<model::Fill*>(node);
        if ( !stroke && !fill )
        {
            auto group = qobject_cast<model::Group*>(node);

            if ( !group )
            {
                if ( auto parent = node->docnode_parent() )
                    group = qobject_cast<model::Group*>(parent);
            }

            if ( group )
            {
                int stroke_count = 0;
                int fill_count = 0;
                for ( const auto& shape : group->shapes )
                {
                    if ( auto s = qobject_cast<model::Stroke*>(shape.get()) )
                    {
                        stroke = s;
                        stroke_count++;
                    }
                    else if ( auto f = qobject_cast<model::Fill*>(shape.get()) )
                    {
                        fill = f;
                        fill_count++;
                    }
                }

                if ( stroke_count > 1 )
                    stroke = nullptr;

                if ( fill_count > 1 )
                    fill = nullptr;
            }
        }
    }

    // Property view
    property_model.set_object(node);
    properties_dock->expandAll();

    // Timeline Widget
    if ( parent->sender() != timeline_dock->timelineWidget() )
        timeline_dock->timelineWidget()->set_current_node(node);

    // Document tree view
    if ( parent->sender() != layers_dock->layer_view() )
    {
        layers_dock->layer_view()->set_current_node(node);
        layers_dock->layer_view()->repaint();
    }

    // Styles
    stroke_dock->set_current(stroke);;
    colors_dock->set_current(fill);
    gradients_dock->set_current(fill, stroke);
    widget_current_style->clear_gradients();

    if ( fill )
    {
        set_brush_reference(fill->use.get(), false);
        if ( !fill->visible.get() )
            widget_current_style->set_fill_color(Qt::transparent);
    }

    if ( stroke )
    {
        set_brush_reference(stroke->use.get(), true);
        if ( !stroke->visible.get() )
            widget_current_style->set_stroke_color(Qt::transparent);
    }
}


void GlaxnimateWindow::Private::selection_changed(const std::vector<model::VisualNode*>& selected, const std::vector<model::VisualNode*>& deselected)
{
    if ( update_selection )
        return;

    auto lock = update_selection.get_lock();

    if ( parent->sender() != layers_dock->layer_view() && parent->sender() != layers_dock->layer_view()->selectionModel() )
        layers_dock->layer_view()->update_selection(selected, deselected);

    if ( parent->sender() != &scene )
    {
        scene.user_select(deselected, graphics::DocumentScene::Remove);
        scene.user_select(selected, graphics::DocumentScene::Append);
    }

    if ( parent->sender() != timeline_dock->timelineWidget() )
        timeline_dock->timelineWidget()->select(selected, deselected);

    const auto& selection = scene.selection();

    if ( std::find(deselected.begin(), deselected.end(), current_node) != deselected.end() )
    {
        lock.unlock();
        set_current_object(selection.empty() ? nullptr : selection[0]);
    }

    std::vector<model::Fill*> fills;
    std::vector<model::Stroke*> strokes;
    for ( auto node : selection )
    {
        for ( const auto styler : node->docnode_find_by_type<model::Styler>() )
        {
            if ( auto fill = styler->cast<model::Fill>() )
                fills.push_back(fill);
            else if ( auto stroke = styler->cast<model::Stroke>() )
                strokes.push_back(stroke);
        }
    }

    colors_dock->set_targets(fills);
    stroke_dock->set_targets(strokes);
    gradients_dock->set_targets(fills, strokes);
}

