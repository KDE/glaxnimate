/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "icon_theme_combo.hpp"

glaxnimate::gui::IconThemeCombo::IconThemeCombo(QWidget* parent)
    : QComboBox(parent)
{
    setModel(&theme_model);
    connect(this, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]{
        Q_EMIT current_theme_id_changed(current_theme_id());
    });
}

void glaxnimate::gui::IconThemeCombo::set_current_theme_id(const QString& id)
{
    for ( int i = 0, e = model()->rowCount(); i < e; i++ )
    {
        if ( model()->data(model()->index(i, 0), Qt::UserRole) == id )
        {
            setCurrentIndex(i);
            return;
        }
    }

    setCurrentIndex(0);
}

QString glaxnimate::gui::IconThemeCombo::current_theme_id() const
{
    return currentData(Qt::UserRole).toString();
}







