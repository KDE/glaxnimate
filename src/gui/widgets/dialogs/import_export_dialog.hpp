/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QFileDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QApplication>

#include "io/io_registry.hpp"
#include "model/document.hpp"
#include "app/settings/widget_builder.hpp"

namespace glaxnimate::gui {

class ImportExportDialog
{

public:
    ImportExportDialog(const io::Options& options, QWidget* parent)
        : parent(parent)
    {
        io_options_ = options;

    }

    const io::Options& io_options() const
    {
        return io_options_;
    }

    bool export_dialog(model::Composition* comp)
    {
        QFileDialog dialog(parent);
        dialog.setWindowTitle(QObject::tr("Save file"));
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        dialog.setFileMode(QFileDialog::AnyFile);
        dialog.setOption(QFileDialog::DontUseNativeDialog, !app::settings::get<bool>("open_save", "native_dialog"));
        setup_file_dialog(dialog, io::IoRegistry::instance().exporters(), io_options_.format, false);
        while ( true )
        {
            if ( !show_file_dialog(dialog, io::IoRegistry::instance().exporters(), io::ImportExport::Export) )
                return false;

            // For some reason the file dialog shows the option to do this automatically but it's disabled
            if ( QFileInfo(io_options_.filename).completeSuffix().isEmpty() )
            {
                io_options_.filename += "." + io_options_.format->extensions()[0];

                QFileInfo finfo(io_options_.filename);
                if ( finfo.exists() )
                {
                    QMessageBox overwrite(
                        QMessageBox::Question,
                        QObject::tr("Overwrite File?"),
                        QObject::tr("The file \"%1\" already exists. Do you wish to overwrite it?")
                            .arg(finfo.baseName()),
                        QMessageBox::Yes|QMessageBox::No,
                        parent
                    );
                    overwrite.setDefaultButton(QMessageBox::Yes);
                    if ( overwrite.exec() != QMessageBox::Yes )
                        continue;
                }
            }

            break;
        }

        auto ok =  options_dialog(io_options_.format->save_settings(comp));
        io_options_.settings["default_time"] = comp->animation->last_frame.get();
        return ok;
    }

    bool import_dialog()
    {
        QFileDialog dialog(parent);
        dialog.setWindowTitle(QObject::tr("Open file"));
        dialog.setAcceptMode(QFileDialog::AcceptOpen);
        dialog.setFileMode(QFileDialog::ExistingFile);
        dialog.setOption(QFileDialog::DontUseNativeDialog, !app::settings::get<bool>("open_save", "native_dialog"));
        setup_file_dialog(dialog, io::IoRegistry::instance().importers(), io_options_.format, true);
        if ( show_file_dialog(dialog, io::IoRegistry::instance().importers(), io::ImportExport::Import) )
            return options_dialog(io_options_.format->open_settings());
        return false;
    }

    bool options_dialog(std::unique_ptr<app::settings::SettingsGroup> settings)
    {
        if ( !io_options_.format )
            return false;

        if ( !settings || settings->settings().empty() )
            return true;

        app::settings::WidgetBuilder widget_builder;
        QString title = QObject::tr("%1 Options").arg(io_options_.format->name());
        return widget_builder.show_dialog(settings->settings(), io_options_.settings, title, parent);
    }

private:

    bool show_file_dialog(QFileDialog& dialog, const std::vector<io::ImportExport*>& formats, io::ImportExport::Direction direction)
    {
        io_options_.format = nullptr;

#ifdef Q_OS_ANDROID
        dialog.setWindowFlags(Qt::Window);
        dialog.setOption(QFileDialog::DontUseNativeDialog, false);
        dialog.setFixedSize(dialog.parentWidget()->size());
        dialog.setWindowState(dialog.windowState() | Qt::WindowFullScreen);
#else
        dialog.setOption(QFileDialog::DontUseNativeDialog, !app::settings::get<bool>("open_save", "native_dialog"));
#endif

        if ( dialog.exec() == QDialog::Rejected )
            return false;

        io_options_.filename = dialog.selectedFiles()[0];
        io_options_.path = dialog.directory();

        int filter = filters.indexOf(dialog.selectedNameFilter());
        if ( filter >= 0 && filter < int(formats.size()) )
        {
            io_options_.format = formats[filter];
        }
        else
        {
            io_options_.format = io::IoRegistry::instance().from_filename(io_options_.filename, direction);
        }

        if ( !io_options_.format )
            return false;

        return true;
    }

    void setup_file_dialog(QFileDialog& dialog, const std::vector<io::ImportExport*>& formats,
                           io::ImportExport* selected, bool add_all)
    {
        dialog.setDirectory(io_options_.path);
        dialog.selectFile(io_options_.filename);

        filters.clear();

        QString all;
        for ( const auto& reg : formats )
        {
            for ( const QString& ext : reg->extensions() )
            {
                all += "*." + ext + " ";
            }

            filters << reg->name_filter();
        }

        dialog.setNameFilters(filters);

        if ( add_all )
        {
            all.resize(all.size() - 1);
            QString all_filter = QObject::tr("All files (%1)").arg(all);
            filters << all_filter;
            dialog.setNameFilters(filters);
            dialog.selectNameFilter(all_filter);
        }
        else if ( selected )
        {
            dialog.selectNameFilter(selected->name_filter());
        }

    }

    QWidget* parent;
    QStringList filters;
    io::Options io_options_;
};

} // namespace glaxnimate::gui
