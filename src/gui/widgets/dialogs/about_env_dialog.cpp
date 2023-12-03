/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "about_env_dialog.hpp"
#include "ui_about_env_dialog.h"

#include <QSysInfo>
#include <QDesktopServices>
#include <QMessageBox>
#include <QClipboard>
#include <QUrl>
#include <QScrollBar>

#include "glaxnimate_settings.hpp"
#include "glaxnimate_app.hpp"
#include "utils/trace.hpp"
#include "utils/tar.hpp"
#include "io/video/video_format.hpp"
#include "io/io_registry.hpp"

using namespace glaxnimate::gui;
using namespace glaxnimate;

static void populate_io(QTableWidget* widget, const std::vector<io::ImportExport*>& data)
{
    widget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    widget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    widget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    int height = 0;

    widget->setRowCount(data.size());
    int row = 0;
    for ( auto item : data )
    {
        widget->setItem(row, 0, new QTableWidgetItem(item->name()));
        widget->setItem(row, 1, new QTableWidgetItem(item->slug()));
        widget->setItem(row, 2, new QTableWidgetItem(item->extensions().join(", ")));
        height += widget->rowHeight(row);
        row++;
    }

    height += height / data.size() / 2;
    height += widget->horizontalHeader()->height();
    height += widget->horizontalScrollBar()->height();
    widget->setMinimumHeight(height);
}

AboutEnvironmentDialog::AboutEnvironmentDialog(QWidget* parent)
    : QDialog(parent), d(new Ui::AboutEnvironmentDialog)
{
    d->setupUi(this);

    QString config_file = GlaxnimateSettings::self()->config()->name();
    if ( !QDir::isAbsolutePath(config_file) )
        config_file = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, config_file, QStandardPaths::LocateFile);
    d->line_settings->setText(config_file);

    d->line_user_data->setText(app::Application::instance()->writable_data_path(""));
    d->line_backup->setText(GlaxnimateApp::instance()->backup_path());

    // Formats
    populate_view(d->view_data, app::Application::instance()->data_paths_unchecked(""));
    populate_view(d->view_icons, QIcon::themeSearchPaths());

    populate_io(d->table_formats_input, io::IoRegistry::instance().importers());
    populate_io(d->table_formats_output, io::IoRegistry::instance().exporters());
}

AboutEnvironmentDialog::~AboutEnvironmentDialog() = default;

void AboutEnvironmentDialog::open_user_data()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(d->line_user_data->text()));
}

void AboutEnvironmentDialog::open_settings_file()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(d->line_settings->text()));
}

void AboutEnvironmentDialog::populate_view(QListWidget* wid, const QStringList& paths)
{
    int h = 0;
    int c = 0;
    for ( const QString& str : paths )
    {
        if ( str.startsWith(":/") )
            continue;
        wid->addItem(str);
        h += wid->sizeHintForRow(c++);
    }
    if (c > 0)
        h += h/c/2;
    wid->setMinimumHeight(h);
}

void AboutEnvironmentDialog::dir_open(const QModelIndex& index)
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(index.data().toString()));
}

void AboutEnvironmentDialog::open_backup()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(d->line_backup->text()));
}
