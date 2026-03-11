/*
 * SPDX-FileCopyrightText: 2024 Julius Künzel <julius.kuenzel@kde.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "propertiesdock.h"

#include "ui_properties.h"

#include "glaxnimate/command/animation_commands.hpp"
#include "widgets/timeline/value_drag_event_filter.hpp"

using namespace glaxnimate::gui;

class PropertiesDock::Private
{
public:
    ::Ui::dock_properties ui;
    item_models::PropertyModelSingle* property_model;
};

PropertiesDock::PropertiesDock(GlaxnimateWindow* parent, item_models::PropertyModelSingle* property_model, style::PropertyDelegate* property_delegate)
    : QDockWidget(parent)
    , d(std::make_unique<Private>())
{
    d->ui.setupUi(this);

    d->property_model = property_model;
    d->ui.view_properties->setModel(property_model);

    d->ui.view_properties->setItemDelegateForColumn(item_models::PropertyModelSingle::ColumnValue, property_delegate);
    d->ui.view_properties->header()->setSectionResizeMode(item_models::PropertyModelSingle::ColumnName, QHeaderView::ResizeToContents);
    d->ui.view_properties->header()->setSectionResizeMode(item_models::PropertyModelSingle::ColumnToggleKeyframe, QHeaderView::ResizeToContents);
    d->ui.view_properties->header()->setSectionResizeMode(item_models::PropertyModelSingle::ColumnValue, QHeaderView::Stretch);

    new ValueDragEventFilter(d->ui.view_properties);
}

PropertiesDock::~PropertiesDock() = default;

void PropertiesDock::expandAll()
{
    d->ui.view_properties->expandAll();
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
                d->property_model->document()->push_command(new command::RemoveKeyframeTime(anprop, time));
            }
            else
            {
                d->property_model->document()->push_command(new command::SetKeyframe(anprop, time, anprop->value(), true));
            }
        }
    }
}
