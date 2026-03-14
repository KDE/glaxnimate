/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QMenu>

#include "glaxnimate/model/document_node.hpp"
#include "widgets/dialogs/selection_manager.hpp"

namespace glaxnimate::gui {


class NodeMenu : public QMenu
{
    Q_OBJECT
public:
    NodeMenu(model::Object* node, SelectionManager* window, QWidget* parent, model::Object* context_object = nullptr);
};

} // namespace glaxnimate::gui
