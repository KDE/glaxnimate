/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>
#include <QString>
#include <QtGlobal>

class QAbstractItemModel;
class QModelIndex;

#if defined(Q_OS_ANDROID) || defined(Q_OS_WIN) || defined(Q_OS_DARWIN)
#   define HAS_FREEDESKTOP_ICONS false
#else
#   define HAS_FREEDESKTOP_ICONS true
#endif


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


