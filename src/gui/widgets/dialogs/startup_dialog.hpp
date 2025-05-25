/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>
#include <QDialog>

#include "model/document.hpp"


namespace glaxnimate::gui {

class GlaxnimateWindow;

class StartupDialog : public QDialog
{
    Q_OBJECT

public:
    StartupDialog(GlaxnimateWindow *parent = nullptr);
    ~StartupDialog();

    std::unique_ptr<model::Document> create() const;

private Q_SLOTS:
    void reload_presets();
    void select_preset(const QModelIndex& index);
    void click_recent(const QModelIndex& index);
    void update_time_units();
    void update_startup_enabled(bool checked);

Q_SIGNALS:
    void open_recent(const QString& path);
    void open_browse();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui
