/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QFileDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QApplication>

#include "glaxnimate/io/io_registry.hpp"
#include "glaxnimate/model/document.hpp"
#include "settings/widget_builder.hpp"
#include "glaxnimate_settings.hpp"

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

    bool export_dialog(model::Composition* comp, io::ImportExport::Direction direction = io::ImportExport::Export)
    {
        QFileDialog dialog(parent);
        dialog.setWindowTitle(direction == io::ImportExport::FrameExport ? i18n("Save Frame Image") : i18n("Save File"));
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        dialog.setFileMode(QFileDialog::AnyFile);
        dialog.setOption(QFileDialog::DontUseNativeDialog, !GlaxnimateSettings::use_native_io_dialog());
        setup_file_dialog(dialog, io::IoRegistry::instance().handlers(direction), io_options_.format, false, direction);
        while ( true )
        {
            if ( !show_file_dialog(dialog, io::IoRegistry::instance().handlers(direction), direction) )
                return false;

            // For some reason the file dialog shows the option to do this automatically but it's disabled
            if ( QFileInfo(io_options_.filename).completeSuffix().isEmpty() )
            {
                io_options_.filename += "." + io_options_.format->extensions(direction)[0];

                QFileInfo finfo(io_options_.filename);
                if ( finfo.exists() )
                {
                    QMessageBox overwrite(
                        QMessageBox::Question,
                        i18n("Overwrite File?"),
                        i18n("The file \"%1\" already exists. Do you wish to overwrite it?", finfo.baseName()),
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
        dialog.setWindowTitle(i18n("Open File"));
        dialog.setAcceptMode(QFileDialog::AcceptOpen);
        dialog.setFileMode(QFileDialog::ExistingFile);
        dialog.setOption(QFileDialog::DontUseNativeDialog, !GlaxnimateSettings::use_native_io_dialog());
        setup_file_dialog(dialog, io::IoRegistry::instance().importers(), io_options_.format, true, io::ImportExport::Import);
        if ( show_file_dialog(dialog, io::IoRegistry::instance().importers(), io::ImportExport::Import) )
            return options_dialog(io_options_.format->open_settings());
        return false;
    }

    bool options_dialog(std::unique_ptr<glaxnimate::settings::SettingsGroup> settings)
    {
        if ( !io_options_.format )
            return false;

        if ( !settings || settings->settings().empty() )
            return true;

        glaxnimate::gui::WidgetBuilder widget_builder;
        QString title = i18n("%1 Options", io_options_.format->name());
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
        dialog.setOption(QFileDialog::DontUseNativeDialog, !GlaxnimateSettings::use_native_io_dialog());
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
                           io::ImportExport* selected, bool add_all, io::ImportExport::Direction direction)
    {
        dialog.setDirectory(io_options_.path);
        dialog.selectFile(io_options_.filename);

        filters.clear();

        QString all;
        for ( const auto& reg : formats )
        {
            for ( const QString& ext : reg->extensions(direction) )
            {
                all += "*." + ext + " ";
            }

            filters << reg->name_filter(direction);
        }

        dialog.setNameFilters(filters);

        if ( add_all )
        {
            all.resize(all.size() - 1);
            QString all_filter = i18n("All files (%1)", all);
            filters << all_filter;
            dialog.setNameFilters(filters);
            dialog.selectNameFilter(all_filter);
        }
        else if ( selected )
        {
            dialog.selectNameFilter(selected->name_filter(direction));
        }
    }

    QWidget* parent;
    QStringList filters;
    io::Options io_options_;
};

} // namespace glaxnimate::gui
