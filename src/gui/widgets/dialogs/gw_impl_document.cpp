/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate_window_p.hpp"

#include <QTemporaryFile>
#include <QDesktopServices>
#include <QFileDialog>
#include <QImageWriter>
#include <QDropEvent>
#include <QtConcurrent>
#include <QEventLoop>
#include <QLocalSocket>
#include <QDataStream>
#include <QSharedMemory>
#include <QtGlobal>
#include <QNetworkReply>

#include <KRecentFilesAction>

#include "io/lottie/lottie_html_format.hpp"
#include "io/svg/svg_renderer.hpp"
#include "io/svg/svg_html_format.hpp"
#include "io/glaxnimate/glaxnimate_format.hpp"
#include "io/raster/raster_mime.hpp"
#include "io/lottie/tgs_format.hpp"
#include "io/lottie/validation.hpp"
#include "io/rive/rive_html_format.hpp"
#include "plugin/io.hpp"

#include "model/visitor.hpp"
#include "widgets/font/font_loader.hpp"

#include "command/undo_macro_guard.hpp"
#include "command/undo_macro_guard.hpp"

#include "tools/base.hpp"

#include "glaxnimate_app.hpp"
#include "app_info.hpp"
#include "widgets/dialogs/import_export_dialog.hpp"
#include "widgets/dialogs/io_status_dialog.hpp"
#include "widgets/shape_style/shape_style_preview_widget.hpp"
#include "widgets/dialogs/stalefiles_dialog.hpp"

template<class T>
static void process_events(const QFuture<T>& promise)
{
    while ( !promise.isFinished() )
    {
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents|QEventLoop::WaitForMoreEvents, 10);
    }
}

void GlaxnimateWindow::Private::setup_document_ptr(std::unique_ptr<model::Document> doc)
{
    if ( !close_document() )
        return;

    current_document = std::move(doc);

    do_setup_document();

    QDir path = GlaxnimateSettings::path();
    auto opts = current_document->io_options();
    opts.path = path;
    current_document->set_io_options(opts);

    view_fit();
    if ( comp && !comp->shapes.empty() )
        layers_dock->layer_view()->set_current_node(comp->shapes[0]);

    timeline_dock->timelineWidget()->reset_view();
}

void GlaxnimateWindow::Private::do_setup_document()
{
    current_document_has_file = false;

    // Composition
    comp = nullptr;
    connect(current_document->assets()->compositions.get(), &model::CompositionList::docnode_child_remove_end, parent, [this](model::DocumentNode*, int index){on_remove_precomp(index);});
    connect(current_document->assets()->compositions.get(), &model::CompositionList::precomp_added, parent, [this](model::Composition* node, int row){setup_composition(node, row);});
    parent->plugActionList( "new_comp_actionlist", new_comp_actions );

    for ( const auto& precomp : current_document->assets()->compositions->values )
        setup_composition(precomp.get());

    // Undo Redo
    parent->undo_group().addStack(&current_document->undo_stack());
    parent->undo_group().setActiveStack(&current_document->undo_stack());

    // Views
    document_node_model.set_document(current_document.get());
    layers_dock->layer_view()->set_composition(comp);
    timeline_dock->timelineWidget()->set_document(current_document.get());
    timeline_dock->timelineWidget()->set_composition(comp);
    assets_dock->setRootIndex(asset_model.mapFromSource(document_node_model.node_index(current_document->assets()).siblingAtColumn(1)));

    property_model.set_document(current_document.get());
    property_model.set_object(comp);
    tab_bar->set_document(current_document.get());

    scene.set_document(current_document.get());

    if ( !current_document->assets()->compositions->values.empty() )
        switch_composition(current_document->assets()->compositions->values[0], 0);

    swatches_dock->set_document(current_document.get());
    gradients_dock->set_document(current_document.get());

    // Scripting
    scriptconsole_dock->clear_contexts();
    scriptconsole_dock->set_global("document", QVariant::fromValue(current_document.get()));

    // Title
    QObject::connect(current_document.get(), &model::Document::filename_changed, parent, &GlaxnimateWindow::refresh_title);
    QObject::connect(&current_document->undo_stack(), &QUndoStack::cleanChanged, parent, &GlaxnimateWindow::refresh_title);
    refresh_title();

    // Playback
    ///...
    QObject::connect(timeline_dock->playControls(), &FrameControlsWidget::frame_selected, current_document.get(), [this](int frame){current_document->set_current_time(frame);});
    QObject::connect(current_document.get(), &model::Document::current_time_changed, timeline_dock->playControls(), [this](float frame){timeline_dock->playControls()->set_frame(frame);});
    QObject::connect(current_document.get(), &model::Document::record_to_keyframe_changed, timeline_dock->playControls(), &FrameControlsWidget::set_record_enabled);
    QObject::connect(current_document.get(), &model::Document::record_to_keyframe_changed, time_slider_dock->playControls(), &FrameControlsWidget::set_record_enabled);
    QObject::connect(timeline_dock->playControls(), &FrameControlsWidget::record_toggled, current_document.get(), &model::Document::set_record_to_keyframe);
    QObject::connect(time_slider_dock->playControls(), &FrameControlsWidget::record_toggled, current_document.get(), &model::Document::set_record_to_keyframe);

    widget_recording->setVisible(false);
    QObject::connect(current_document.get(), &model::Document::record_to_keyframe_changed, widget_recording, &QWidget::setVisible);

    // Export
    export_options = {};
    parent->actionCollection()->action(QStringLiteral("export"))->setText(i18n("Export..."));
}

void GlaxnimateWindow::Private::setup_document_new(const QString& filename)
{
    if ( !close_document() )
        return;

    current_document = std::make_unique<model::Document>(filename);
    current_document->assets()->add_comp_no_undo();

    do_setup_document();
    QUrl url = QUrl::fromLocalFile(i18n("Unsaved Animation"));
    autosave_file.setManagedFile(url);
    comp->name.set(comp->type_name_human());
    comp->width.set(GlaxnimateSettings::width());
    comp->height.set(GlaxnimateSettings::height());
    comp->fps.set(GlaxnimateSettings::fps());
    float duration = GlaxnimateSettings::duration();
    int out_point = comp->fps.get() * duration;
    comp->animation->last_frame.set(out_point);


    auto layer = std::make_unique<model::Layer>(current_document.get());
    layer->animation->last_frame.set(out_point);
    layer->name.set(layer->type_name_human());
    QPointF pos(
        comp->width.get() / 2.0,
        comp->height.get() / 2.0
    );
    layer->transform.get()->anchor_point.set(pos);
    layer->transform.get()->position.set(pos);
    model::ShapeElement* ptr = layer.get();
    comp->shapes.insert(std::move(layer), 0);

    QDir path = GlaxnimateSettings::path();
    auto opts = current_document->io_options();
    opts.path = path;
    current_document->set_io_options(opts);

    layers_dock->layer_view()->set_current_node(ptr);
    timeline_dock->playControls()->set_range(0, out_point);
    view_fit();

    timeline_dock->timelineWidget()->reset_view();
}

bool GlaxnimateWindow::Private::setup_document_open(QIODevice* file, const io::Options& options, bool is_file)
{
    if ( !close_document() )
        return false;

    current_document = std::make_unique<model::Document>(options.filename);

    dialog_import_status->reset(options.format, options.filename);
    /*auto promise = QtConcurrent::run(
        [options, current_document=current_document.get()]{
            QFile file(options.filename);
            return options.format->open(file, options.filename, current_document, options.settings);
        });

    process_events(promise);

    bool ok = promise.result();*/

    current_document->set_io_options(options);

    bool ok = options.format->open(*file, options.filename, current_document.get(), options.settings);

    do_setup_document();

    if ( is_file )
    {
        current_document_has_file = true;

        app::settings::set<QString>("open_save", "path", options.path.absolutePath());

        if ( ok && !autosave_load )
            push_recent_file(QUrl::fromLocalFile(options.filename));
    }

    view_fit();


    QUrl file_url = QUrl::fromLocalFile(options.filename);
    auto stale = KAutoSaveFile::staleFiles(file_url);
    autosave_file.setManagedFile(file_url);

    if ( !autosave_load && !stale.empty() )
    {
        // This ensures the stale file is not leaked if the user doesn't load it.
        stale[0]->setParent(current_document.get());

        WindowMessageWidget::Message msg{
            i18n("Looks like this file is being edited by another Glaxnimate instance or it was being edited when Glaxnimate crashed."),
            KMessageWidget::Information
        };

        msg.add_action(
            QIcon::fromTheme("document-close"),
            i18n("Close Document"),
            parent,
            &GlaxnimateWindow::document_new
        );

        msg.add_action(
            QIcon::fromTheme("document-revert"),
            i18n("Load Backup"),
            parent,
            [this, file=stale[0]]{ load_backup(file, true); }
        );

        for ( int i = 1; i < stale.size(); i++ )
            delete stale[i];

        message_widget->queue_message(std::move(msg));
    }

    export_options = options;
    export_options.filename = "";

    timeline_dock->timelineWidget()->reset_view();

    load_pending();

    return ok;
}

bool GlaxnimateWindow::Private::setup_document_open(const io::Options& options)
{
    QFile file(options.filename);
    return setup_document_open(&file, options, true);
}


void GlaxnimateWindow::Private::refresh_title()
{
    QString title = current_document->filename();
    if ( !current_document->undo_stack().isClean() )
        title += " *";
    parent->setWindowTitle(title);
}

bool GlaxnimateWindow::Private::close_document()
{
    if ( current_document )
    {
        if ( !autosave_load ) {
            autosave_file.remove();
            autosave_file.releaseLock();
        }

        if ( !current_document->undo_stack().isClean() )
        {
            QMessageBox warning(parent);
            warning.setWindowTitle(i18n("Closing Animation"));
            warning.setText(i18n("The animation has unsaved changes.\nDo you want to save your changes?"));
            warning.setInformativeText(current_document->filename());
            warning.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            warning.setDefaultButton(QMessageBox::Save);
            warning.setIcon(QMessageBox::Warning);
            int result = warning.exec();

            if ( result == QMessageBox::Save )
            {
                if ( !save_document(false, false) )
                    return false;
            }
            else if ( result == QMessageBox::Cancel )
            {
                return false;
            }

            // Prevent signals on the destructor
            current_document->undo_stack().clear();
        }
    }

    if ( active_tool )
        active_tool->close_document_event({canvas, &scene, parent});

    comp = nullptr;
    stroke_dock->clear_document();
    colors_dock->clear_document();
    document_node_model.clear_document();
    property_model.clear_document();
    scene.clear_document();
    timeline_dock->clear_document();
    swatches_dock->clear_document();
    gradients_dock->clear_document();
    render_widget.set_composition(nullptr);
    layers_dock->layer_view()->set_composition(nullptr);
    tab_bar->set_document(nullptr);

    for ( const auto& stack : parent->undo_group().stacks() )
        parent->undo_group().removeStack(stack);

    scriptconsole_dock->clear_contexts();
    scriptconsole_dock->clear_output();
    scriptconsole_dock->set_global("document", QVariant{});

    comp_selections.clear();
    parent->unplugActionList("new_comp_actionlist");
    new_comp_actions.clear();

    return true;
}

bool GlaxnimateWindow::Private::save_document(bool force_dialog, bool export_opts)
{
    io::Options opts = export_opts ? export_options : current_document->io_options();

    if ( !opts.format || !opts.format->can_save() || !current_document_has_file || opts.filename.isEmpty() )
        force_dialog = true;

    if ( force_dialog )
    {
        ImportExportDialog dialog(opts, parent);

        if ( !dialog.export_dialog(comp) )
            return false;

        opts = dialog.io_options();
    }

    dialog_export_status->reset(opts.format, opts.filename);

    if ( !qobject_cast<plugin::IoFormat*>(opts.format) )
    {
        auto promise = QtConcurrent::run(
            [opts, comp=comp]{
                QSaveFile file(opts.filename);
                bool result = opts.format->save(file, opts.filename, comp, opts.settings);

                if ( result )
                    file.commit();

                return result;
            });

        process_events(promise);

        if ( !promise.result() )
            return false;
    }
    else
    {
        QSaveFile file(opts.filename);
        bool result = opts.format->save(file, opts.filename, comp, opts.settings);

        if ( !result )
            return false;

        file.commit();
    }

    if ( export_opts )
    {
        export_options = opts;
        parent->actionCollection()->action(QStringLiteral("export"))->setText(i18n("Export to %1", QFileInfo(opts.filename).fileName()));

    }
    else
    {
        if ( opts.format->can_open() )
            push_recent_file(QUrl::fromLocalFile(opts.filename));
        current_document->undo_stack().setClean();
        current_document_has_file = true;
        app::settings::set<QString>("open_save", "path", opts.path.absolutePath());
        current_document->set_io_options(opts);

        if ( !export_opts && !export_options.format )
            export_options.path = opts.path;
    }

    autosave_file.remove();
    autosave_file.setManagedFile(QUrl::fromLocalFile(opts.filename));
    return true;
}

void GlaxnimateWindow::Private::document_open()
{
    io::Options options = current_document->io_options();

    ImportExportDialog dialog(options, parent);
    if ( dialog.import_dialog() )
        setup_document_open(dialog.io_options());
}


io::Options GlaxnimateWindow::Private::options_from_filename(const QString& filename, const QVariantMap& settings)
{
    QFileInfo finfo(filename);
    if ( finfo.isFile() )
    {
        io::Options opts;
        opts.format = io::IoRegistry::instance().from_filename(filename, io::ImportExport::Import);
        opts.path = finfo.dir();
        opts.filename = filename;
        opts.settings = settings;

        if ( opts.format )
        {
            ImportExportDialog dialog(opts, parent);
            if ( dialog.options_dialog(opts.format->open_settings()) )
                return dialog.io_options();

            return {};
        }
        else
        {
            show_warning(i18n("Open File"), i18n("No importer found for %1", filename));
        }
    }
    else
    {
        show_warning(i18n("Open File"), i18n("The file might have been moved or deleted\n%1", filename));
    }

    return {};
}


void GlaxnimateWindow::Private::document_open_from_filename(const QString& filename, const QVariantMap& settings)
{
    io::Options opts = options_from_filename(filename, settings);
    if ( opts.format )
    {
        setup_document_open(opts);
        // TODO
        //most_recent_file(filename);
    }
}

void GlaxnimateWindow::Private::document_open_from_url(const QUrl& url)
{
    QString filename = url.scheme() == "tmp" ? url.path() : url.toLocalFile();
    io::Options opts = options_from_filename(filename, {});
    if ( opts.format )
    {
        setup_document_open(opts);
        push_recent_file(url);
    }
}

void GlaxnimateWindow::Private::push_recent_file(const QUrl& url)
{
    if ( url.isLocalFile() )
    {
        QString file = url.toLocalFile();
        if ( file.startsWith(QDir::tempPath()) )
        {
            m_recentFilesAction->addUrl(QUrl(QStringLiteral("tmp://") + file));
            return;
        }
    }
    m_recentFilesAction->addUrl(url);
}

void GlaxnimateWindow::Private::drop_document(const QString& filename, bool as_comp)
{
    auto options = options_from_filename(filename);
    if ( !options.format )
        return;

    model::Document imported(options.filename);
    QFile file(options.filename);
    bool ok = options.format->open(file, options.filename, &imported, options.settings);
    if ( !ok )
    {
        show_warning(i18n("Import File"), i18n("Could not import %1", options.filename));
        return;
    }

    parent->paste_document(&imported, i18n("Import File"), as_comp);
}

void GlaxnimateWindow::Private::document_reload()
{
    if ( !current_document_has_file )
    {
        status_message(i18n("No file to reload from"));
        return;
    }

    auto options = current_document->io_options();
    setup_document_open(options);
}


void GlaxnimateWindow::Private::preview(io::ImportExport& exporter, const QVariantMap& options)
{
    dialog_export_status->reset(&exporter, i18n("Web Preview"));

    auto promise = QtConcurrent::run(
        [&exporter, comp=comp, options]() -> QString {
            QTemporaryFile tempf(GlaxnimateApp::temp_path() + "/XXXXXX." + exporter.extensions()[0]);
            tempf.setAutoRemove(false);
            bool ok = tempf.open() && exporter.save(
                tempf, tempf.fileName(), comp, options
            );
            if ( !ok )
                return "";
            return tempf.fileName();
        });

    process_events(promise);

    QString path = promise.result();

    dialog_export_status->disconnect_import_export();

    if ( path.isEmpty() )
    {
        show_warning(i18n("Web Preview"), i18n("Could not create file"));
        return;
    }

    if ( !QDesktopServices::openUrl(QUrl::fromLocalFile(path)) )
    {
        show_warning(i18n("Web Preview"), i18n("Could not open browser"));
    }
}

void GlaxnimateWindow::Private::preview_lottie(const QString& renderer)
{
    io::lottie::LottieHtmlFormat fmt;
    preview(fmt, {{"renderer", renderer}});
}

void GlaxnimateWindow::Private::preview_svg()
{
    io::svg::SvgHtmlFormat fmt;
    preview(fmt, {});
}

void GlaxnimateWindow::Private::preview_rive()
{
    io::rive::RiveHtmlFormat fmt;
    preview(fmt, {});
}

void GlaxnimateWindow::Private::save_frame_bmp()
{
    int frame = current_document->current_time();
    QFileDialog fd(parent, i18n("Save Frame Image"));
    fd.setDirectory(GlaxnimateSettings::render_path());
    fd.setDefaultSuffix("png");
    fd.selectFile(i18n("Frame%1.png", frame));
    fd.setAcceptMode(QFileDialog::AcceptSave);
    fd.setFileMode(QFileDialog::AnyFile);
    fd.setOption(QFileDialog::DontUseNativeDialog, !GlaxnimateSettings::use_native_io_dialog());

    QString formats;
    for ( const auto& fmt : QImageWriter::supportedImageFormats() )
        formats += QString("*.%1 ").arg(QString::fromUtf8(fmt));
    fd.setNameFilter(i18n("Image files (%1)", formats));

    if ( fd.exec() == QDialog::Rejected )
        return;

    GlaxnimateSettings::setRender_path(fd.directory().path());

    QImage image = io::raster::RasterMime().to_image({comp});
    if ( !image.save(fd.selectedFiles()[0]) )
        show_warning(i18n("Render Frame"), i18n("Could not save image"));
}


void GlaxnimateWindow::Private::save_frame_svg()
{
    int frame = current_document->current_time();
    QFileDialog fd(parent, i18n("Save Frame Image"));
    fd.setDirectory(GlaxnimateSettings::render_path());
    fd.setDefaultSuffix("svg");
    fd.selectFile(i18n("Frame%1.svg", frame));
    fd.setAcceptMode(QFileDialog::AcceptSave);
    fd.setFileMode(QFileDialog::AnyFile);
    fd.setNameFilter(i18n("Scalable Vector Graphics (*.svg)"));
    fd.setOption(QFileDialog::DontUseNativeDialog, !GlaxnimateSettings::use_native_io_dialog());

    if ( fd.exec() == QDialog::Rejected )
        return;

    GlaxnimateSettings::setRender_path(fd.directory().path());

    QFile file(fd.selectedFiles()[0]);
    if ( !file.open(QFile::WriteOnly) )
    {
        show_warning(i18n("Render Frame"), i18n("Could not save image"));
        return;
    }

    io::svg::SvgRenderer rend(io::svg::NotAnimated, io::svg::CssFontType::FontFace);
    rend.write_main(comp, frame);
    rend.write(&file, true);
}

void GlaxnimateWindow::Private::validate_discord()
{
    IoStatusDialog dialog(QIcon::fromTheme("discord"), "", false, parent);
    io::lottie::LottieFormat fmt;
    dialog.reset(&fmt, i18n("Validate Discord Sticker"));
    io::lottie::validate_discord(current_document.get(), comp, &fmt);
    dialog.show_errors(i18n("No issues found"), i18n("Some issues detected"));
    dialog.exec();
}

void GlaxnimateWindow::Private::validate_tgs()
{
    IoStatusDialog dialog(QIcon::fromTheme("telegram"), "", false, parent);
    io::lottie::TgsFormat fmt;
    dialog.reset(&fmt, i18n("Validate Telegram Sticker"));
    fmt.validate(current_document.get(), comp);
    dialog.show_errors(i18n("No issues found"), i18n("Some issues detected"));
    dialog.exec();
}

void GlaxnimateWindow::Private::autosave_timer_start(int mins)
{
    if ( mins == -1 )
        mins = GlaxnimateSettings::backup_frequency();
    autosave_timer_mins = mins;
    if ( autosave_timer_mins )
        autosave_timer = parent->startTimer(autosave_timer_mins * 1000 * 60);
}

void GlaxnimateWindow::Private::autosave(bool force)
{
    if ( current_document && (force || (!current_document->undo_stack().isClean() && !autosave_load)) )
    {
        autosave_file.open(QIODevice::WriteOnly);
        io::glaxnimate::GlaxnimateFormat().save(autosave_file, autosave_file.fileName(), comp, {});
        autosave_file.close();
    }
}

void GlaxnimateWindow::Private::autosave_timer_load_settings()
{
    int mins = GlaxnimateSettings::backup_frequency();
    if ( mins != autosave_timer_mins )
    {
        if ( autosave_timer )
            parent->killTimer(autosave_timer);
        autosave_timer = 0;
        autosave_timer_start(mins);
    }
}

void GlaxnimateWindow::Private::load_backup(KAutoSaveFile* file, bool io_options_from_current)
{
    file->setParent(nullptr);
    std::unique_ptr<KAutoSaveFile> fptr(file);

    if ( !file->open(QIODevice::ReadOnly) )
    {
        show_warning(i18n("Load Backup"), i18n("Could not open the backup file"));
        return;
    }

    QFileInfo path(file->managedFile().toString());
    QDir dir = path.dir();
    if ( dir.isRoot() )
        dir = QDir();

    io::Options io_options_bak {
        io::glaxnimate::GlaxnimateFormat::instance(),
        dir,
        path.filePath(),
        {}
    };

    io::Options io_options;
    if ( io_options_from_current )
    {
        io_options = current_document->io_options();
    }
    else
    {
        io_options = io_options_bak;
        io_options.format = io::IoRegistry::instance().from_filename(path.filePath(), io::ImportExport::Import);
    }

    auto lock = autosave_load.get_lock();
    setup_document_open(file, io_options_bak, true);
    file->close();
    autosave_file.setManagedFile(file->managedFile());
    current_document->set_io_options(io_options);
    current_document->undo_stack().resetClean();
    autosave(true);
}


QString GlaxnimateWindow::Private::drop_event_data(QDropEvent* event)
{
    const QMimeData* data = event->mimeData();

    if ( !data->hasUrls() )
       return {};

    for ( const auto& url : data->urls() )
    {
        if ( url.isLocalFile() )
        {
            QString filename = url.toLocalFile();
            QString extension = QFileInfo(filename).completeSuffix();
            if ( io::IoRegistry::instance().from_extension(extension, io::ImportExport::Import) )
                return filename;
        }
    }

    return {};
}

void GlaxnimateWindow::Private::set_color_def(model::BrushStyle* def, bool secondary)
{
    model::Styler* target;
    QString what;
    if ( secondary )
    {
        target = stroke_dock->current();
        what = i18n("Stroke");
    }
    else
    {
        target = colors_dock->current();
        what = i18n("Fill");
    }

    if ( target )
    {
        auto old = target->use.get();

        if ( !def )
        {
            command::UndoMacroGuard macro(i18n("Unlink %1 Color", what), current_document.get());
            if ( auto col = qobject_cast<model::NamedColor*>(target->use.get()) )
                target->color.set_undoable(col->color.get());
            target->use.set_undoable(QVariant::fromValue(def));
            target->visible.set_undoable(false);
            if ( old )
                old->remove_if_unused(false);
        }
        else
        {
            command::UndoMacroGuard macro(i18n("Link %1 Color", what), current_document.get());
            target->use.set_undoable(QVariant::fromValue(def));
            target->visible.set_undoable(true);
            if ( old )
                old->remove_if_unused(false);
        }
    }

    set_brush_reference(def, secondary);
}

void GlaxnimateWindow::Private::set_brush_reference ( model::BrushStyle* sty, bool secondary )
{
    if ( secondary )
        widget_current_style->set_stroke_ref(sty);
    else
        widget_current_style->set_fill_ref(sty);

    if ( qobject_cast<model::Gradient*>(sty) )
        sty = nullptr;

    if ( secondary )
        secondary_brush = sty;
    else
        main_brush = sty;

    style_change_event();
}

void GlaxnimateWindow::Private::style_change_event()
{
    if ( active_tool )
        active_tool->shape_style_change_event({canvas, &scene, parent});
}


void GlaxnimateWindow::Private::import_file()
{
    io::Options options = current_document->io_options();
    QString path = GlaxnimateSettings::import_path();
    if ( !path.isEmpty() )
        options.path.setPath(path);

    ImportExportDialog dialog(options, parent);
    if ( dialog.import_dialog() )
    {
        options = dialog.io_options();
        GlaxnimateSettings::setImport_path(options.path.path());
        import_file(options);
    }

}

void GlaxnimateWindow::Private::import_file(const io::Options& options)
{
    QFile file(options.filename);
    import_file(&file, options);
}

void GlaxnimateWindow::Private::import_file(QIODevice* file, const io::Options& options)
{
    model::Document imported(options.filename);

    dialog_export_status->reset(options.format, options.filename);
    auto settings = options.settings;
    settings["default_time"] = comp->animation->last_frame.get();
    bool ok = options.format->open(*file, options.filename, &imported, settings);
    if ( !ok )
    {
        show_warning(i18n("Import File"), i18n("Could not import %1", options.filename));
        return;
    }

    /// \todo ask if comp
    parent->paste_document(&imported, i18n("Import File"), true);

    load_pending();
}

void GlaxnimateWindow::Private::import_file(const QString& filename, const QVariantMap& settings)
{
    QFileInfo finfo(filename);
    io::Options opts;
    opts.settings = settings;
    opts.format = io::IoRegistry::instance().from_extension(finfo.suffix(), io::ImportExport::Import);
    if ( !opts.format )
        show_warning(i18n("Import File"), i18n("Could not import %1", filename));
    opts.filename = filename;
    opts.path = finfo.dir();
    import_file(opts);
}

void GlaxnimateWindow::Private::ipc_connect(const QString &name)
{
    ipc_socket = qobject_make_unique<QLocalSocket>();
    ipc_stream = std::make_unique<QDataStream>(ipc_socket.get());
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    ipc_stream->setVersion(QDataStream::Qt_5_15);
    QObject::connect(ipc_socket.get(), &QLocalSocket::errorOccurred, parent, &GlaxnimateWindow::ipc_error);
#endif
    QObject::connect(ipc_socket.get(), &QLocalSocket::readyRead, parent, &GlaxnimateWindow::ipc_read);
    ipc_socket->connectToServer(name);
    ipc_memory = std::make_unique<QSharedMemory>(name);
}

static void on_font_loader_finished(glaxnimate::gui::font::FontLoader* loader)
{
    if ( !loader->fonts().empty() )
    {
        auto document = static_cast<glaxnimate::model::Document*>(loader->parent());
        bool clear = document->undo_stack().count() == 0;

        glaxnimate::command::UndoMacroGuard guard(i18n("Download fonts"), document);
        for ( const auto& font : loader->fonts() )
            document->assets()->add_font(font);
        guard.finish();

        if ( clear )
            document->undo_stack().clear();
    }
    loader->deleteLater();
}

void glaxnimate::gui::GlaxnimateWindow::Private::load_pending()
{
    auto font_loader = new glaxnimate::gui::font::FontLoader();
    font_loader->setParent(current_document.get());
    connect(
        font_loader, &gui::font::FontLoader::error, parent,
        [](const QString& msg){ app::log::Log("Font Loader").log(msg); }
    );
    font_loader->queue_pending(current_document.get());
    connect(
        font_loader, &gui::font::FontLoader::finished, current_document.get(),
        [font_loader]{on_font_loader_finished(font_loader);}
    );
    font_loader->load_queue();
}

void glaxnimate::gui::GlaxnimateWindow::Private::load_remote_document(const QUrl& url, io::Options options, bool open)
{
    auto reply = http.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, parent, [this, options, open, reply]() {
        if ( reply->error() )
        {
            auto code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
            QString message;
            if ( code.isValid() )
            {
                auto reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
                message = i18n("HTTP Error %1 %2", code.toInt(), reason);
            }
            else
            {
                message = i18n("Network Error");
            }

            show_warning(i18n("Load URL"), i18n("Could not load %1: %2", reply->url().toString(), message));
            return;
        }

        if ( open )
            setup_document_open(reply, options, false);
        else
            import_file(reply, options);
    });
}

void glaxnimate::gui::GlaxnimateWindow::Private::check_autosaves()
{
    auto stale = KAutoSaveFile::allStaleFiles();
    if ( stale.empty() )
        return;

    WindowMessageWidget::Message msg{
        i18n("There are %1 auto-save files that can be restored.", stale.size()),
        KMessageWidget::Information
    };

    msg.add_action(QIcon::fromTheme("dialog-cancel"), i18n("Ignore"));

    msg.add_action(
        QIcon::fromTheme("document-revert"),
        i18n("Load Backup..."),
        parent,
        [this, stale]{ show_stale_autosave_list(stale); }
    );

    message_widget->queue_message(std::move(msg));
}

void glaxnimate::gui::GlaxnimateWindow::Private::show_stale_autosave_list(const QList<KAutoSaveFile*>& stale)
{
    StalefilesDialog dialog(stale, parent);
    if ( dialog.exec() == QDialog::Accepted && dialog.selected() )
    {
        dialog.cleanup(dialog.selected());
        load_backup(dialog.selected(), true);
    }
    else
    {
        dialog.cleanup(nullptr);
    }
}
