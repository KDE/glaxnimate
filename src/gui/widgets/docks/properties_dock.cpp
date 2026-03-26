/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 * SPDX-FileCopyrightText: 2024 Julius Künzel <julius.kuenzel@kde.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "properties_dock.hpp"

#include "ui_properties_dock.h"

#include "glaxnimate/command/animation_commands.hpp"
#include "widgets/timeline/value_drag_event_filter.hpp"
#include "widgets/menus/animated_property_menu.hpp"
#include "widgets/menus/node_menu.hpp"

using namespace glaxnimate::gui;

class PropertiesDock::Private
{
public:
    ::Ui::dock_properties ui;
    item_models::PropertyModelSingle* property_model;
    GlaxnimateWindow* window;
};

PropertiesDock::PropertiesDock(GlaxnimateWindow* parent, item_models::PropertyModelSingle* property_model, style::PropertyDelegate* property_delegate)
    : QDockWidget(parent)
    , d(std::make_unique<Private>())
{
    d->window = parent;
    d->ui.setupUi(this);

    d->property_model = property_model;
    d->ui.view_properties->setModel(property_model);

    d->ui.view_properties->setItemDelegateForColumn(item_models::PropertyModelSingle::ColumnValue, property_delegate);
    d->ui.view_properties->header()->setSectionResizeMode(item_models::PropertyModelSingle::ColumnName, QHeaderView::ResizeToContents);
    d->ui.view_properties->header()->setSectionResizeMode(item_models::PropertyModelSingle::ColumnToggleKeyframe, QHeaderView::ResizeToContents);
    d->ui.view_properties->header()->setSectionResizeMode(item_models::PropertyModelSingle::ColumnValue, QHeaderView::Stretch);

    new ValueDragEventFilter(d->ui.view_properties, item_models::PropertyModelSingle::ColumnValue);

    connect(this, &QWidget::customContextMenuRequested, this, &PropertiesDock::custom_context_menu);
}

PropertiesDock::~PropertiesDock() = default;

void PropertiesDock::expandAll()
{
    d->ui.view_properties->expandAll();
}

item_models::PropertyModelBase::Item PropertiesDock::item_at(const QPoint &glob) const
{
    QPoint pos = d->ui.view_properties->mapFromGlobal(glob);
    if ( !d->ui.view_properties->rect().contains(pos) )
        return {};

    return d->property_model->item(
        d->ui.view_properties->indexAt(
        d->ui.view_properties->viewport()->mapFromGlobal(glob)
    ));
}

void glaxnimate::gui::PropertiesDock::click_index(const QModelIndex& index)
{
    if ( auto anprop = d->property_model->animatable(index) )
    {
        if ( index.column() == item_models::PropertyModelSingle::ColumnToggleKeyframe )
        {
            auto time = d->property_model->document()->current_time();
            if ( anprop->has_keyframe(time) )
            {
                d->property_model->document()->push_command(anprop->command_remove_keyframe(time));
            }
            else
            {
                d->property_model->document()->push_command(
                    anprop->command_add_smooth_keyframe(time, anprop->static_value())
                );
            }
        }
    }
}

void glaxnimate::gui::PropertiesDock::custom_context_menu(const QPoint& p)
{
    auto index = d->ui.view_properties->indexAt(p);
    auto item = d->property_model->item(index);

    if ( item.property && !(item.property->traits().flags & model::PropertyTraits::List) )
    {
        if ( item.property->traits().flags & model::PropertyTraits::Animated )
        {
            AnimatedPropertyMenu menu(this);
            menu.set_controller(d->window);
            menu.set_property(static_cast<model::AnimatedPropertyBase*>(item.property));
            menu.exec(QCursor::pos());
        }
        else if ( item.property->traits().type == model::PropertyTraits::Object )
        {
            auto op = static_cast<model::SubObjectPropertyBase*>(item.property);
            NodeMenu(op->sub_object(), d->window, this, item.property->object()).exec(QCursor::pos());
        }
        else if ( item.property->traits().type ==
            model::PropertyTraits::ObjectReference )
        {
            auto op = static_cast<model::ReferencePropertyBase*>(item.property);
            if ( auto ref = op->get_ref() )
                NodeMenu(ref, d->window, this, item.property->object()).exec(QCursor::pos());
        }
    }
    else if ( item.object )
    {
        NodeMenu(item.object, d->window, this).exec(QCursor::pos());
    }
}
