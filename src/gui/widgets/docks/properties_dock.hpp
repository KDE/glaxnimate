/*
 * SPDX-FileCopyrightText: 2024 Julius Künzel <julius.kuenzel@kde.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

# pragma once

#include "item_models/property_model_single.hpp"
#include "style/property_delegate.hpp"
#include "widgets/dialogs/glaxnimate_window.hpp"
#include <QDockWidget>
#include <QObject>

namespace glaxnimate::gui {

class PropertiesDock : public QDockWidget
{
    Q_OBJECT

public:
    PropertiesDock(GlaxnimateWindow* parent, item_models::PropertyModelSingle* property_model, style::PropertyDelegate* property_delegate);

    ~PropertiesDock();

    void expandAll();


    /**
     * @brief Gets the item at the given mouse position
     * @param global_pos Global mouse position
     */
    item_models::PropertyModelBase::Item item_at(const QPoint& global_pos) const;

private Q_SLOTS:
    void click_index(const QModelIndex& index);
    void custom_context_menu(const QPoint& p);

private:
    class Private;
    std::unique_ptr<Private> d;
};

}
