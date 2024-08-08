/*
 * SPDX-FileCopyrightText: 2024 Julius Künzel <julius.kuenzel@kde.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "layersdock.h"

#include "kactioncollection.h"
#include "ui_layers.h"
#include "widgets/menus/node_menu.hpp"

using namespace glaxnimate::gui;

class LayersDock::Private
{
public:
    ::Ui::dock_layers ui;
};

LayersDock::LayersDock(GlaxnimateWindow *parent, item_models::DocumentModelBase* base_model, QMenu* menu_new_layer)
    : QDockWidget(parent)
    , d(std::make_unique<Private>())
{
    d->ui.setupUi(this);

    d->ui.view_document_node->set_base_model(base_model);

    d->ui.view_document_node->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(d->ui.view_document_node, &QWidget::customContextMenuRequested, parent,
                     [&, this](const QPoint& pos){
                         auto index = d->ui.view_document_node->indexAt(pos);
                         if ( auto node = d->ui.view_document_node->node(index) )
                             NodeMenu(node, parent, parent).exec(d->ui.view_document_node->mapToGlobal(pos));
                     }
                );

    d->ui.btn_layer_add->setMenu(menu_new_layer);

    d->ui.btn_layer_top->setDefaultAction(parent->actionCollection()->action("object_raise_to_top"));
    d->ui.btn_layer_up->setDefaultAction(parent->actionCollection()->action("object_raise"));
    d->ui.btn_layer_down->setDefaultAction(parent->actionCollection()->action("object_lower"));
    d->ui.btn_layer_bottom->setDefaultAction(parent->actionCollection()->action("object_lower_to_bottom"));

    connect(d->ui.btn_layer_add, &QToolButton::clicked, this, &LayersDock::add_layer);
    connect(d->ui.btn_layer_duplicate, &QToolButton::clicked, this, &LayersDock::duplicate_layer);
    connect(d->ui.btn_layer_remove, &QToolButton::clicked, this, &LayersDock::delete_layer);
}

LayersDock::~LayersDock() = default;


glaxnimate::gui::LayerView* LayersDock::layer_view()
{
    return d->ui.view_document_node;
}
