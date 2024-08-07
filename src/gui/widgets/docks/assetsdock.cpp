/*
 * SPDX-FileCopyrightText: 2024 Julius Künzel <julius.kuenzel@kde.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "assetsdock.h"

#include "item_models/asset_proxy_model.hpp"
#include "ui_assets.h"
#include "widgets/menus/node_menu.hpp"

using namespace glaxnimate::gui;

class AssetsDock::Private
{
public:
    ::Ui::dock_assets ui;
};

AssetsDock::AssetsDock(GlaxnimateWindow *parent, item_models::AssetProxyModel* asset_model, item_models::DocumentNodeModel* document_node_model)
    : QDockWidget(parent)
    , d(std::make_unique<Private>())
{
    d->ui.setupUi(this);

    d->ui.view_assets->setModel(asset_model);
    d->ui.view_assets->header()->setSectionResizeMode(item_models::DocumentNodeModel::ColumnName-1, QHeaderView::Stretch);
    d->ui.view_assets->header()->hideSection(item_models::DocumentNodeModel::ColumnVisible-1);
    d->ui.view_assets->header()->hideSection(item_models::DocumentNodeModel::ColumnLocked-1);
    d->ui.view_assets->header()->setSectionResizeMode(item_models::DocumentNodeModel::ColumnUsers-1, QHeaderView::ResizeToContents);


    connect(d->ui.view_assets, &CustomTreeView::customContextMenuRequested, parent, [&, this](const QPoint& pos){
        auto node = document_node_model->node(asset_model->mapToSource(d->ui.view_assets->indexAt(pos)));
        if ( node )
            NodeMenu(node, parent, parent).exec(d->ui.view_assets->mapToGlobal(pos));
    });
}

AssetsDock::~AssetsDock() = default;

void AssetsDock::setRootIndex(const QModelIndex &index) {
    d->ui.view_assets->setRootIndex(index);
}


