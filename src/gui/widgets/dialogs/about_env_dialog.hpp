/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>
#include <QDialog>

namespace glaxnimate::gui::Ui
{
class AboutEnvironmentDialog;
}

class QListWidget;

namespace glaxnimate::gui {

class AboutEnvironmentDialog : public QDialog
{
    Q_OBJECT

public:
    AboutEnvironmentDialog(QWidget* parent = nullptr);

    ~AboutEnvironmentDialog();

protected:
    void populate_view(QListWidget* wid, const QStringList& paths);

private Q_SLOTS:
    void open_user_data();
    void open_settings_file();
    void dir_open(const QModelIndex& index);
    void open_backup();

private:
    std::unique_ptr<Ui::AboutEnvironmentDialog> d;
};

} // namespace glaxnimate::gui
