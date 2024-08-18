/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>
#include <QString>

class QAbstractItemModel;
class QModelIndex;


namespace glaxnimate::gui::settings {

class IconSettings
{
private:
    IconSettings();
    IconSettings(const IconSettings&) = delete;
    IconSettings& operator=(const IconSettings&) = delete;

public:
    static IconSettings& instance();

    void initialize();

    QString icon_theme() const;
    void set_icon_theme(const QString& theme);
    void palette_change() const;


    QAbstractItemModel* item_model() const;
    QModelIndex current_item_index() const;
    void set_current_item_index(const QModelIndex& index);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui::settings


