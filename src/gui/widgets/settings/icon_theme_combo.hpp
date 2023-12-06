/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once


#include <QComboBox>
#include "settings/icon_theme.hpp"


namespace glaxnimate::gui {

class IconThemeCombo : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(QString current_theme_id READ current_theme_id WRITE set_current_theme_id NOTIFY current_theme_id_changed USER true)

public:
    explicit IconThemeCombo(QWidget* parent = nullptr);

    QString current_theme_id() const;

public Q_SLOTS:
    void set_current_theme_id(const QString& id);

Q_SIGNALS:
    void current_theme_id_changed(const QString& id);

private:
    IconThemeModel theme_model;

};

} // namespace glaxnimate::gui
