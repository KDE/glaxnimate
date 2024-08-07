/*
 * SPDX-FileCopyrightText: 2024 Julius Künzel <julius.kuenzel@kde.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

# pragma once

#include "item_models/asset_proxy_model.hpp"
#include "widgets/dialogs/glaxnimate_window.hpp"
#include <QDockWidget>
#include <QObject>

namespace glaxnimate::gui {

class AssetsDock : public QDockWidget
{
    Q_OBJECT

public:
    AssetsDock(GlaxnimateWindow* parent, item_models::AssetProxyModel *asset_model, item_models::DocumentNodeModel *document_node_model);

    ~AssetsDock();

    void setRootIndex(const QModelIndex &index);

private:
    class Private;
    std::unique_ptr<Private> d;
};

}
