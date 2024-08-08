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

LayersDock::LayersDock(GlaxnimateWindow *parent, item_models::DocumentModelBase* base_model)
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


    QMenu *menu_new_layer = new QMenu(i18n("New"));
    menu_new_layer->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));

    //menu_new_layer.addMenu();
    /*
        <widget class="QMenu" name="menu_new_comp_layer">
        <property name="title">
        <string>&amp;Composition</string>
        </property>
        <property name="icon">
        <iconset theme="component"/>
        </property>
        </widget>
        </widget>
    */
    menu_new_layer->addSeparator();
    menu_new_layer->addAction(parent->actionCollection()->action(QStringLiteral("new_complayer"))); // TODO menu_new_comp_layer
    menu_new_layer->addAction(parent->actionCollection()->action(QStringLiteral("new_layer")));
    menu_new_layer->addAction(parent->actionCollection()->action(QStringLiteral("new_group"))); // TODO action_new_group
    menu_new_layer->addAction(parent->actionCollection()->action(QStringLiteral("new_fill")));
    menu_new_layer->addAction(parent->actionCollection()->action(QStringLiteral("new_stroke"))); // TODO action_new_stroke
    menu_new_layer->addAction(parent->actionCollection()->action(QStringLiteral("insert_emoji")));

    d->ui.btn_layer_add->setMenu(menu_new_layer);

}

LayersDock::~LayersDock() = default;


glaxnimate::gui::LayerView* LayersDock::layer_view()
{
    return d->ui.view_document_node;
}
