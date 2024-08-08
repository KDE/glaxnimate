/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 * SPDX-FileCopyrightText: 2024 Julius Künzel <julius.kuenzel@kde.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate_window_p.hpp"

#include <QComboBox>
#include <QLabel>
#include <QScrollBar>
#include <QInputDialog>
#include <QLabel>
#include <QPushButton>
#include <QAction>
#include <QActionGroup>
#include <QScreen>
#include <QDialogButtonBox>
#include <QStatusBar>
#include <QClipboard>

#include <KHelpMenu>
#include <KActionCategory>
#include <KActionCollection>
#include <KShortcutsDialog>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KRecentFilesAction>
#include <KToolBar>
#include <KSandbox>
#include <KAboutData>
#include <KCoreAddons>
#include <KColorSchemeMenu>
#include <KColorSchemeManager>
#include <KActionMenu>

#include "tools/base.hpp"
#include "model/shapes/group.hpp"
#include "model/shapes/image.hpp"
#include "model/shapes/repeater.hpp"
#include "model/shapes/trim.hpp"
#include "model/shapes/inflate_deflate.hpp"
#include "model/shapes/round_corners.hpp"
#include "model/shapes/offset_path.hpp"
#include "model/shapes/zig_zag.hpp"
#include "io/io_registry.hpp"

#include "widgets/dialogs/io_status_dialog.hpp"
#include "widgets/dialogs/about_env_dialog.hpp"
#include "widgets/dialogs/resize_dialog.hpp"
#include "widgets/dialogs/timing_dialog.hpp"
#include "widgets/dialogs/document_metadata_dialog.hpp"
#include "widgets/dialogs/trace_dialog.hpp"
#include "widgets/dialogs/startup_dialog.hpp"
#include "widgets/docks/aligndock.h"
#include "widgets/docks/assetsdock.h"
#include "widgets/docks/logsdock.h"
#include "widgets/docks/propertiesdock.h"
#include "widgets/docks/script_console.hpp"
#include "widgets/docks/timelinedock.h"
#include "widgets/lottiefiles/lottiefiles_search_dialog.hpp"

#include "widgets/view_transform_widget.hpp"
#include "widgets/flow_layout.hpp"
#include "widgets/menus/node_menu.hpp"
#include "widgets/shape_style/shape_style_preview_widget.hpp"

#include "style/better_elide_delegate.hpp"
#include "tools/edit_tool.hpp"
#include "plugin/action.hpp"
#include "glaxnimate_app.hpp"
#include "settings/document_templates.hpp"
#include "emoji/emoji_set_dialog.hpp"

#include "widgets/docks/layersdock.h"


using namespace glaxnimate::gui;


void GlaxnimateWindow::Private::setupUi(bool restore_state, bool debug, GlaxnimateWindow* parent)
{
    this->parent = parent;

    // Central Widget
    QWidget *centralWidget = new QWidget(parent);
    QVBoxLayout *centralLayout = new QVBoxLayout(centralWidget);
    centralWidget->setLayout(centralLayout);

    canvas = new Canvas(centralWidget);
    canvas->setAcceptDrops(true);

    message_widget = new WindowMessageWidget(centralWidget);
    tab_bar = new CompositionTabBar(centralWidget);

    centralLayout->addWidget(message_widget);
    centralLayout->addWidget(tab_bar);
    centralLayout->addWidget(canvas);

    // Actions
    setup_file_actions();
    setup_edit_actions();
    tools::Tool* to_activate = setup_tools_actions();
    setup_view_actions();
    setup_path_actions();
    setup_object_actions();
    setup_playback_actions();
    setup_document_actions();
    setup_layers_actions();

    // Actions: Text
    KActionCategory *textActions = new KActionCategory(i18n("Text"), parent->actionCollection());

    QAction *textOnPath = add_action(textActions, QStringLiteral("text_put_on_path"), i18n("Put on Path"), QStringLiteral("text-put-on-path"));
    connect(textOnPath, &QAction::triggered, parent, [this]{text_put_on_path();});

    QAction *textRemovePath = add_action(textActions, QStringLiteral("text_remove_from_path"), i18n("Remove from Path"), QStringLiteral("text-remove-from-path"));
    connect(textRemovePath, &QAction::triggered, parent, [this]{text_remove_from_path();});

    // Load themes
    KColorSchemeManager *manager = new KColorSchemeManager(parent);
    auto *colorSelectionMenu = KColorSchemeMenu::createMenu(manager, parent);
    parent->actionCollection()->addAction(QStringLiteral("colorscheme_menu"), colorSelectionMenu);

    // Actions: Settings and Help
    QAction *copyDebug = new QAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy Debug Information"), parent);
    parent->actionCollection()->addAction(QStringLiteral("copy_debuginfo"), copyDebug);
    connect(copyDebug, &QAction::triggered, parent, &GlaxnimateWindow::copyDebugInfo);

    QAction *aboutEnv = new QAction(QIcon::fromTheme(QStringLiteral("help-about-symbolic")), i18n("About Environment"), parent);
    parent->actionCollection()->addAction(QStringLiteral("about_env"), aboutEnv);

    KStandardAction::preferences(parent, &GlaxnimateWindow::preferences, parent->actionCollection());

    // Main Window
    parent->setupGUI();

    parent->setDockOptions(parent->dockOptions() | QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks);
    parent->setDockOptions(parent->dockOptions() | QMainWindow::GroupedDragging);

    parent->setCentralWidget(centralWidget);

    // Docks
    colors_dock = new ColorsDock(parent);
    parent->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, colors_dock);
    stroke_dock = new StrokeDock(parent);
    parent->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, stroke_dock);

    tools::EditTool* edit_tool = static_cast<tools::EditTool*>(tools::Registry::instance().tool("edit"));
    connect(edit_tool, &tools::EditTool::gradient_stop_changed, colors_dock, &ColorsDock::set_gradient_stop);
    connect(edit_tool, &tools::EditTool::gradient_stop_changed, stroke_dock, &StrokeDock::set_gradient_stop);

    init_status_bar();

    // Graphics scene
    canvas->setScene(&scene);
    canvas->set_tool_target(parent);
    connect(&scene, &graphics::DocumentScene::node_user_selected, parent, &GlaxnimateWindow::scene_selection_changed);
    connect(canvas, &Canvas::dropped, parent, [this](const QMimeData* d){dropped(d);});

    // Dialogs
    dialog_import_status = new IoStatusDialog(QIcon::fromTheme("document-open"), i18n("Open File"), false, parent);
    dialog_export_status = new IoStatusDialog(QIcon::fromTheme("document-save"), i18n("Save File"), false, parent);

    about_env_dialog = new AboutEnvironmentDialog(parent);
    connect(aboutEnv, &QAction::triggered, parent, &GlaxnimateWindow::help_about_env);

    // Docks
    init_docks();

    init_item_views();

    // Initialize tools
    init_tools_ui();
    init_tools(to_activate);

    connect_playback_actions();

    auto preset = LayoutPreset(app::settings::get<int>("ui", "layout"));

    switch ( preset )
    {
    case LayoutPreset::Unknown:
    case LayoutPreset::Auto:
        layout_auto();
        break;
    case LayoutPreset::Wide:
        layout_wide();
        break;
    case LayoutPreset::Compact:
        layout_compact();
        break;
    case LayoutPreset::Medium:
        layout_medium();
        break;
    case LayoutPreset::Custom:
        layout_auto();
        app::settings::set("ui", "layout", int(LayoutPreset::Custom));
        QAction *layoutCustom = parent->actionCollection()->action(QStringLiteral("layout_custom"));
        layoutCustom->setChecked(true);
        break;
    }

    // Menus
    init_menus();

    // Debug menu
    if ( debug )
        init_debug();

    // Restore state
    // NOTE: keep at the end so we do this once all the widgets are in their default spots
    if ( restore_state )
        init_restore_state();
}

template<class T>
void GlaxnimateWindow::Private::add_modifier_menu_action(KActionCategory *collection)
{
    QAction *action = add_action(collection, "new_" + T::static_class_name().toLower(), T::static_type_name_human());
    action->setIcon(T::static_tree_icon());
    connect(action, &QAction::triggered, [this]{
        auto layer = std::make_unique<T>(current_document.get());
        parent->layer_new_impl(std::move(layer));
    });
}

QAction* GlaxnimateWindow::Private::add_action(KActionCategory *category, const QString &id, const QString &text, const QString &iconName, const QString &toolTip, const QKeySequence &shortcut)
{
    QAction *action = new QAction(parent);
    action->setText(text);
    action->setToolTip(toolTip);
    if (!iconName.isEmpty()) {
        action->setIcon(QIcon::fromTheme(iconName));
    }
    parent->actionCollection()->setDefaultShortcut(action, shortcut);
    category->addAction(id, action);
    return action;
}

void GlaxnimateWindow::Private::setup_file_actions()
{
    //QAction *clearAction = add_action(QStringLiteral("clear"), i18n("Clear"), QIcon::fromTheme("edit-select-symbolic"), {}, Qt::CTRL | Qt::Key_L)

    KActionCategory *fileActions = new KActionCategory(i18n("File"), parent->actionCollection());

    // File
    fileActions->addAction(KStandardAction::New, parent, SLOT(document_new()));
    fileActions->addAction(KStandardAction::Open, parent, SLOT(document_open_dialog()));
    QAction *importImage = add_action(fileActions, QStringLiteral("import_image"), i18n("Add Image…"), QStringLiteral("insert-image"), {}, Qt::CTRL | Qt::Key_I);
    connect(importImage, &QAction::triggered, parent, [this]{import_image();});

    QAction *documentImport = add_action(fileActions, QStringLiteral("import"), i18n("Import…"), QStringLiteral("document-import"), {}, Qt::CTRL | Qt::SHIFT | Qt::Key_I);
    connect(documentImport, &QAction::triggered, parent, [this]{import_file();});

    QAction *importLottie = add_action(fileActions, QStringLiteral("open_lottiefiles"), i18n("Import from LottieFiles…"), QStringLiteral("lottiefiles"));
    connect(importLottie, &QAction::triggered, parent, [this]{import_from_lottiefiles();});

    QAction *openLast = add_action(fileActions, QStringLiteral("open_last"), i18n("Open Most Recent"), QStringLiteral("document-open-recent"), {}, Qt::CTRL | Qt::SHIFT | Qt::Key_O);
    connect(openLast, &QAction::triggered, parent, [this]{
        QList<QUrl> recent_file_urls = m_recentFilesAction->urls();
        if ( !recent_file_urls.isEmpty() )
        {
            // Avoid references to recent_files
            QUrl url = recent_file_urls.first();
            document_open_from_url(url);
        }
    });

    m_recentFilesAction = KStandardAction::openRecent(parent, &GlaxnimateWindow::document_open_recent, fileActions);
    fileActions->addAction(KStandardAction::name(KStandardAction::OpenRecent), m_recentFilesAction);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(m_recentFilesAction, &KRecentFilesAction::enabledChanged, openLast, &QAction::setEnabled);
#else
    connect(m_recentFilesAction, &KRecentFilesAction::changed, [this, openLast](){
        openLast->setEnabled(m_recentFilesAction->isEnabled());
    });
#endif

    m_recentFilesAction->loadEntries(KConfigGroup(KSharedConfig::openConfig(), QString()));

    QAction *revertAction = fileActions->addAction(KStandardAction::Revert, parent, SLOT(document_reload()));

    parent->actionCollection()->setDefaultShortcut(revertAction, Qt::CTRL | Qt::Key_F5);
    fileActions->addAction(KStandardAction::Save, parent, SLOT(document_save()));
    fileActions->addAction(KStandardAction::SaveAs, parent, SLOT(document_save_as()));
    QAction *saveAsTemplate = add_action(fileActions, QStringLiteral("save_as_template"), i18n("Save as Template"), QStringLiteral("document-save-as-template"));
    connect(saveAsTemplate, &QAction::triggered, parent, [this]{
        bool ok = true;

        QString old_name = comp->name.get();
        QString name = QInputDialog::getText(parent, i18n("Save as Template"), i18n("Name"), QLineEdit::Normal, old_name, &ok);
        if ( !ok )
            return;

        comp->name.set(name);
        if ( !settings::DocumentTemplates::instance().save_as_template(current_document.get()) )
            show_warning(i18n("Save as Template"), i18n("Could not save template"));
        comp->name.set(old_name);
    });

    QAction *documentExport = add_action(fileActions, QStringLiteral("export"), i18n("Export…"), QStringLiteral("document-export"), {}, Qt::CTRL | Qt::Key_E);
    connect(documentExport, &QAction::triggered, parent, &GlaxnimateWindow::document_export);

    QAction *exportAs = add_action(fileActions, QStringLiteral("export_as"), i18n("Export As…"), QStringLiteral("document-export"), {}, Qt::CTRL | Qt::SHIFT | Qt::Key_E);
    connect(exportAs, &QAction::triggered, parent, &GlaxnimateWindow::document_export_as);

    QAction *exportSequence = add_action(fileActions, QStringLiteral("export_sequence"), i18n("Export as Image Sequence…"), QStringLiteral("folder-images"));
    connect(exportSequence, &QAction::triggered, parent, &GlaxnimateWindow::document_export_sequence);

    KStandardAction::close(parent, &GlaxnimateWindow::close, parent->actionCollection());
    KStandardAction::quit(qApp, &QCoreApplication::quit, parent->actionCollection());
}

void GlaxnimateWindow::Private::setup_edit_actions()
{
    KActionCategory *editActions = new KActionCategory(i18n("Edit"), parent->actionCollection());

    QAction *undo = editActions->addAction(KStandardAction::Undo, &parent->undo_group(), SLOT(undo()));
    QObject::connect(&parent->undo_group(), &QUndoGroup::canUndoChanged, undo, &QAction::setEnabled);
    QObject::connect(&parent->undo_group(), &QUndoGroup::undoTextChanged, undo, [this, undo](const QString& s){
        undo->setText(i18n("Undo %1", s));
    });

    QAction *redo = editActions->addAction(KStandardAction::Redo, &parent->undo_group(), SLOT(undo()));
    QObject::connect(&parent->undo_group(), &QUndoGroup::canRedoChanged, redo, &QAction::setEnabled);
    QObject::connect(&parent->undo_group(), &QUndoGroup::redoTextChanged, redo, [this, redo](const QString& s){
        redo->setText(i18n("Redo %1", s));
    });
    editActions->addAction(KStandardAction::Cut, parent, SLOT(cut()));
    editActions->addAction(KStandardAction::Copy, parent, SLOT(copy()));
    editActions->addAction(KStandardAction::Paste, parent, SLOT(paste()));

    // edit_paste_as_completion
    QAction *pasteAsComposition = add_action(editActions, QStringLiteral("edit_paste_as_composition"), i18n("Paste as Composition"), QStringLiteral("special_paste"), {}, Qt::CTRL | Qt::SHIFT | Qt::Key_V);
    connect(pasteAsComposition, &QAction::triggered, parent, [this]{parent->paste_as_composition();});

    QAction *duplicate = add_action(editActions, QStringLiteral("duplicate"), i18n("Duplicate"), QStringLiteral("edit-duplicate"), {}, Qt::CTRL | Qt::Key_D);
    connect(duplicate, &QAction::triggered, parent, &GlaxnimateWindow::duplicate_selection);

    QAction *selectAll = editActions->addAction(KStandardAction::SelectAll);
    selectAll->setEnabled(false);
    QAction *editDelete = add_action(editActions, QStringLiteral("edit_delete"), i18n("Delete"), QStringLiteral("edit-delete"), {}, Qt::Key_Delete);
    connect(editDelete, &QAction::triggered, [this](){
        if (active_tool->id() == QStringLiteral("edit")) {
            parent->actionCollection()->action(QStringLiteral("node_remove"))->trigger();
        } else if (active_tool->id() == QStringLiteral("select")) {
            parent->delete_selected();
        }
    });
    this->tool_actions["select"] = {
        editDelete,
    };
}

tools::Tool* GlaxnimateWindow::Private::setup_tools_actions()
{
    KActionCategory *toolActions = new KActionCategory(i18n("Tools"), parent->actionCollection());

    // Tool Actions
    QActionGroup *tool_actions = new QActionGroup(parent);
    tool_actions->setExclusive(true);

    tools::Tool* to_activate = nullptr;
    for ( const auto& grp : tools::Registry::instance() )
    {
        for ( const auto& tool : grp.second )
        {
            QAction *action = tool.second->get_action();
            action->setActionGroup(tool_actions);
            parent->actionCollection()->setDefaultShortcut(action, tool.second->key_sequence());
            toolActions->addAction(tool.second->action_name(), action);

            connect(action, &QAction::triggered, parent, &GlaxnimateWindow::tool_triggered);

            if ( !to_activate )
            {
                to_activate = tool.second.get();
                action->setChecked(true);
            }
        }
    }

    return to_activate;
}

void GlaxnimateWindow::Private::setup_view_actions()
{
    KActionCategory *viewActions = new KActionCategory(i18n("View"), parent->actionCollection());

    QActionGroup *layout_actions = new QActionGroup(parent);
    layout_actions->setExclusive(true);

    QAction *layoutCustom = add_action(viewActions, QStringLiteral("layout_custom"), i18n("Custom"), {}, QStringLiteral("Customized Layout"));
    layoutCustom->setActionGroup(layout_actions);
    layoutCustom->setCheckable(true);

    connect(layoutCustom, &QAction::triggered, parent, [this]{layout_update();});

    QAction *layoutAuto = add_action(viewActions, QStringLiteral("layout_automatic"), i18n("Automatic"), {}, QStringLiteral("Determines the best layout based on screen size"));
    layoutAuto->setActionGroup(layout_actions);
    layoutAuto->setCheckable(true);
    connect(layoutAuto, &QAction::triggered, parent, [this]{layout_update();});

    QAction *layoutWide = add_action(viewActions, QStringLiteral("layout_wide"), i18n("Wide"), {}, QStringLiteral("Layout best suited for larger screens"));
    layoutWide->setActionGroup(layout_actions);
    layoutWide->setCheckable(true);
    connect(layoutWide, &QAction::triggered, parent, [this]{layout_update();});

    QAction *layoutMedium = add_action(viewActions, QStringLiteral("layout_medium"), i18n("Medium"), {}, QStringLiteral("More compact than Wide but larger than Compact"));
    layoutMedium->setActionGroup(layout_actions);
    layoutMedium->setCheckable(true);
    connect(layoutMedium, &QAction::triggered, parent, [this]{layout_update();});

    QAction *layoutCompact = add_action(viewActions, QStringLiteral("layout_compact"), i18n("Compact"), {}, QStringLiteral("Layout best suited for smaller screens"));
    layoutCompact->setActionGroup(layout_actions);
    layoutCompact->setCheckable(true);
    connect(layoutCompact, &QAction::triggered, parent, [this]{layout_update();});


    viewActions->addAction(KStandardAction::ZoomIn, canvas, SLOT(zoom_in()));
    viewActions->addAction(KStandardAction::ZoomOut, canvas, SLOT(zoom_out()));
    viewActions->addAction(KStandardAction::FitToPage, parent, SLOT(view_fit()));
    viewActions->addAction(KStandardAction::ActualSize, canvas, SLOT(reset_zoom()));

    QAction *toolResetRotation = add_action(viewActions, QStringLiteral("view_reset_rotation"), i18n("Reset Rotation"), QStringLiteral("rotation-allowed"));
    connect(toolResetRotation, &QAction::triggered, canvas, &Canvas::reset_rotation);
    QAction *toolFlipView = add_action(viewActions, QStringLiteral("flip_view"), i18n("Flip View"), QStringLiteral("object-flip-horizontal"));
    connect(toolFlipView, &QAction::triggered, canvas, &Canvas::flip_horizontal);
}

void GlaxnimateWindow::Private::setup_document_actions()
{
    KActionCategory *documentActions = new KActionCategory(i18n("Document"), parent->actionCollection());

    QAction *renderRaster = add_action(documentActions, QStringLiteral("render_frame_raster"), i18n("Raster…"), QStringLiteral("image-png"));
    connect(renderRaster, &QAction::triggered, parent, &GlaxnimateWindow::save_frame_bmp);
    QAction *renderSvg = add_action(documentActions, QStringLiteral("render_frame_svg"), i18n("SVG…"), QStringLiteral("image-svg+xml"));
    connect(renderSvg, &QAction::triggered, parent, &GlaxnimateWindow::save_frame_svg);

    QAction *previewLottie = add_action(documentActions, QStringLiteral("lottie_preview"), i18n("Lottie (SVG)"));
    connect(previewLottie, &QAction::triggered, parent, [this]{preview_lottie("svg");});

    QAction *previewLottieCanvas = add_action(documentActions, QStringLiteral("lottie_canvas_preview"), i18n("Lottie (canvas)"));
    connect(previewLottieCanvas, &QAction::triggered, parent, [this]{preview_lottie("canvas");});

    QAction *previewSvg = add_action(documentActions, QStringLiteral("svg_preview"), i18n("SVG (SMIL)"));
    connect(previewSvg, &QAction::triggered, parent, [this]{preview_svg();});

    QAction *previewRive = add_action(documentActions, QStringLiteral("rive_preview"), i18n("RIVE (canvas)"));
    connect(previewRive, &QAction::triggered, parent, [this]{preview_rive();});

    QAction *validateTgs = add_action(documentActions, QStringLiteral("validate_tgs"), i18n("Validate Telegram Sticker"), QStringLiteral("telegram"));
    connect(validateTgs, &QAction::triggered, parent, &GlaxnimateWindow::validate_tgs);

    QAction *validateDiscord = add_action(documentActions, QStringLiteral("validate_discord"), i18n("Validate Discord Sticker"), QStringLiteral("discord"));
    connect(validateDiscord, &QAction::triggered, parent, [this]{validate_discord();});

    QAction *documentResize = add_action(documentActions, QStringLiteral("document_resize"), i18n("Resize…"), QStringLiteral("transform-scale"));
    connect(documentResize, &QAction::triggered, parent, [this]{ ResizeDialog(this->parent).resize_composition(comp); });

    QAction *documentCleanup = add_action(documentActions, QStringLiteral("document_cleanup"), i18n("Cleanup"), QStringLiteral("document-cleanup"), QStringLiteral("Remove unused assets"));
    connect(documentCleanup, &QAction::triggered, parent, [this]{cleanup_document();});

    QAction *documentTiming = add_action(documentActions, QStringLiteral("document_timing"), i18n("Timing…"), QStringLiteral("player-time"));
    connect(documentTiming, &QAction::triggered, parent, [this]{
        TimingDialog(comp, this->parent).exec();
    });

    QAction *documentMetadata = add_action(documentActions, QStringLiteral("document_metadata"), i18n("Metadata…"), QStringLiteral("documentinfo"));
    connect(documentMetadata, &QAction::triggered, parent, [this]{
        DocumentMetadataDialog(current_document.get(), this->parent).exec();
    });
}

void GlaxnimateWindow::Private::setup_playback_actions()
{
    KActionCategory *playbackActions = new KActionCategory(i18n("Playback"), parent->actionCollection());


    add_action(playbackActions, QStringLiteral("play"), i18n("Play"), QStringLiteral("media-playback-start"), {}, Qt::Key_Space);

    add_action(playbackActions, QStringLiteral("play_loop"), i18n("Loop"), QStringLiteral("media-playlist-repeat"));

    add_action(playbackActions, QStringLiteral("record"), i18n("Record Keyframes"), QStringLiteral("media-record"));

    add_action(playbackActions, QStringLiteral("frame_first"), i18n("Jump to Start"), QStringLiteral("go-first"));

    add_action(playbackActions, QStringLiteral("frame_last"), i18n("Jump to End"), QStringLiteral("go-last"));

    add_action(playbackActions, QStringLiteral("frame_next"), i18n("Next Frame"), QStringLiteral("go-next"), {}, Qt::Key_Right);

    add_action(playbackActions, QStringLiteral("frame_prev"), i18n("Previous Frame"), QStringLiteral("go-previous"), {}, Qt::Key_Left);
}

void GlaxnimateWindow::Private::connect_playback_actions()
{
    QAction *play = parent->actionCollection()->action(QStringLiteral("play"));
    connect(play, &QAction::triggered, timeline_dock->playControls(), &FrameControlsWidget::toggle_play);
    connect(timeline_dock->playControls(), &FrameControlsWidget::play_started, parent, [play]{
        play->setText(i18n("Pause"));
        play->setIcon(QIcon::fromTheme("media-playback-pause"));
    });
    connect(timeline_dock->playControls(), &FrameControlsWidget::play_stopped, parent, [play]{
        play->setText(i18n("Play"));
        play->setIcon(QIcon::fromTheme("media-playback-start"));
    });
    QAction *record = parent->actionCollection()->action(QStringLiteral("record"));
    connect(timeline_dock->playControls(), &FrameControlsWidget::record_toggled, record, &QAction::setChecked);

    QAction *playLoop = parent->actionCollection()->action(QStringLiteral("play_loop"));
    connect(timeline_dock->playControls(), &FrameControlsWidget::loop_changed, playLoop, &QAction::setChecked);
    connect(playLoop, &QAction::triggered, timeline_dock->playControls(), &FrameControlsWidget::set_loop);

    connect(record, &QAction::triggered, timeline_dock->playControls(), &FrameControlsWidget::set_record_enabled);

    QAction *frameFirst = parent->actionCollection()->action(QStringLiteral("frame_first"));
    connect(frameFirst, &QAction::triggered, timeline_dock->playControls(), &FrameControlsWidget::go_first);

    QAction *frameLast = parent->actionCollection()->action(QStringLiteral("frame_last"));
    connect(frameLast, &QAction::triggered, timeline_dock->playControls(), &FrameControlsWidget::go_last);

    QAction *frameNext = parent->actionCollection()->action(QStringLiteral("frame_next"));
    connect(frameNext, &QAction::triggered, timeline_dock->playControls(), &FrameControlsWidget::go_next);

    QAction *framePrev = parent->actionCollection()->action(QStringLiteral("frame_prev"));
    connect(framePrev, &QAction::triggered, timeline_dock->playControls(), &FrameControlsWidget::go_prev);

    connect(timeline_dock->playControls(),   &FrameControlsWidget::min_changed,    time_slider_dock->playControls(), &FrameControlsWidget::set_min);
    connect(timeline_dock->playControls(),   &FrameControlsWidget::max_changed,    time_slider_dock->playControls(), &FrameControlsWidget::set_max);
    connect(timeline_dock->playControls(),   &FrameControlsWidget::fps_changed,    time_slider_dock->playControls(), &FrameControlsWidget::set_fps);
    connect(timeline_dock->playControls(),   &FrameControlsWidget::play_started,   time_slider_dock->playControls(), &FrameControlsWidget::play);
    connect(timeline_dock->playControls(),   &FrameControlsWidget::play_stopped,   time_slider_dock->playControls(), &FrameControlsWidget::pause);
    connect(timeline_dock->playControls(),   &FrameControlsWidget::loop_changed,   time_slider_dock->playControls(), &FrameControlsWidget::set_loop);
    connect(timeline_dock->playControls(),   &FrameControlsWidget::frame_selected, time_slider_dock->playControls(), &FrameControlsWidget::set_frame);
    connect(time_slider_dock->playControls(), &FrameControlsWidget::play_started,   timeline_dock->playControls(),   &FrameControlsWidget::play);
    connect(time_slider_dock->playControls(), &FrameControlsWidget::play_stopped,   timeline_dock->playControls(),   &FrameControlsWidget::pause);
    connect(time_slider_dock->playControls(), &FrameControlsWidget::loop_changed,   timeline_dock->playControls(),   &FrameControlsWidget::set_loop);
    connect(time_slider_dock->playControls(), &FrameControlsWidget::frame_selected, timeline_dock->playControls(),   &FrameControlsWidget::set_frame);
    connect(timeline_dock->playControls(), &FrameControlsWidget::min_changed, time_slider_dock->timeSlider(), &QSlider::setMinimum);
    connect(timeline_dock->playControls(), &FrameControlsWidget::max_changed, time_slider_dock->timeSlider(), &QSlider::setMaximum);
    connect(timeline_dock->playControls(), &FrameControlsWidget::frame_selected, time_slider_dock->timeSlider(), &QSlider::setValue);
}

void GlaxnimateWindow::Private::setup_layers_actions()
{
    KActionCategory *layersActions = new KActionCategory(i18n("Layers"), parent->actionCollection());

    add_modifier_menu_action<model::Repeater>(layersActions);
    add_modifier_menu_action<model::Trim>(layersActions);
    add_modifier_menu_action<model::InflateDeflate>(layersActions);
    add_modifier_menu_action<model::RoundCorners>(layersActions);
    add_modifier_menu_action<model::OffsetPath>(layersActions);
    add_modifier_menu_action<model::ZigZag>(layersActions);

    QAction *newPrecompSelection = add_action(layersActions, QStringLiteral("new_precomp_selection"), i18n("Precompose Selection"), QStringLiteral("archive-extract"));
    connect(newPrecompSelection, &QAction::triggered, parent, [this]{
        objects_to_new_composition(comp, cleaned_selection(), &comp->shapes, -1);
    });
    QAction *newComp = add_action(layersActions, QStringLiteral("new_comp"), i18n("New Composition"), QStringLiteral("folder-video"));
    connect(newComp, &QAction::triggered, parent, [this]{add_composition();});

    QAction *newLayer = add_action(layersActions, QStringLiteral("new_layer"), i18n("Layer"), QStringLiteral("folder"));
    connect(newLayer, &QAction::triggered, parent, [this]{layer_new_layer();});

    QAction *newGroup = add_action(layersActions, QStringLiteral("new_layer_group"), i18n("Group"), QStringLiteral("shapes-symbolic"));
    connect(newGroup, &QAction::triggered, parent, [this]{layer_new_group();});

    QAction *newFill = add_action(layersActions, QStringLiteral("new_fill"), i18n("Fill"), QStringLiteral("format-fill-color"));
    connect(newFill, &QAction::triggered, parent, [this]{layer_new_fill();});

    QAction *newStroke = add_action(layersActions, QStringLiteral("new_stroke"), i18n("Stroke"), QStringLiteral("object-stroke-style"));
    connect(newStroke, &QAction::triggered, parent, [this]{layer_new_stroke();});

    QAction *insertEmoji = add_action(layersActions, QStringLiteral("insert_emoji"), i18n("Emoji…"), QStringLiteral("smiley-shape"));
    connect(insertEmoji, &QAction::triggered, parent, [this]{insert_emoji();});
#ifdef Q_OS_WIN32
    // Can't get emoji_data.cpp to compile on windows for qt6 for some reason
    // the compiler errors out without message
    insertEmoji->setEnabled(false);
#endif
}

void GlaxnimateWindow::Private::setup_object_actions()
{
    KActionCategory *objectActions = new KActionCategory(i18n("Object"), parent->actionCollection());

    QAction *raiseToTop = add_action(objectActions, QStringLiteral("object_raise_to_top"), i18n("Raise to Top"), QStringLiteral("layer-top"), {}, Qt::Key_Home);
    connect(raiseToTop, &QAction::triggered, parent, &GlaxnimateWindow::document_reload);

    QAction *raise = add_action(objectActions, QStringLiteral("object_raise"), i18n("Raise"), QStringLiteral("layer-raise"), {}, Qt::Key_PageUp);
    connect(raise, &QAction::triggered, parent, &GlaxnimateWindow::layer_raise);

    QAction *lower = add_action(objectActions, QStringLiteral("object_lower"), i18n("Lower"), QStringLiteral("layer-lower"), {}, Qt::Key_PageDown);
    connect(lower, &QAction::triggered, parent, &GlaxnimateWindow::layer_lower);

    QAction *lowerToBottom = add_action(objectActions, QStringLiteral("object_lower_to_bottom"), i18n("Lower to Bottom"), QStringLiteral("layer-bottom"), {}, Qt::Key_End);
    connect(lowerToBottom, &QAction::triggered, parent, &GlaxnimateWindow::layer_lower);

    QAction *moveTo = add_action(objectActions, QStringLiteral("move_to"), i18n("Move to…"), QStringLiteral("selection-move-to-layer-above"));
    connect(moveTo, &QAction::triggered, parent, &GlaxnimateWindow::move_to);

    QAction *group = add_action(objectActions, QStringLiteral("action_group"), i18n("Group"), QStringLiteral("object-group"), {}, Qt::CTRL | Qt::Key_G);
    connect(group, &QAction::triggered, parent, &GlaxnimateWindow::group_shapes);

    QAction *ungroup = add_action(objectActions, QStringLiteral("action_ungroup"), i18n("Ungroup"), QStringLiteral("object-ungroup"), {}, Qt::CTRL | Qt::SHIFT | Qt::Key_G);
    connect(ungroup, &QAction::triggered, parent, &GlaxnimateWindow::ungroup_shapes);

    KActionCategory *alginActions = new KActionCategory(i18n("Align"), parent->actionCollection());

    QAction *alignToSelection = add_action(alginActions, QStringLiteral("align_to_selection"), i18n("Selection"), QStringLiteral("select-rectangular"));
    alignToSelection->setCheckable(true);
    QAction *alignToCanvas = add_action(alginActions, QStringLiteral("align_to_canvas"), i18n("Canvas"), QStringLiteral("snap-page"));
    alignToCanvas->setCheckable(true);
    QAction *alignToCanvasGroup = add_action(alginActions, QStringLiteral("align_to_canvas_group"), i18n("Canvas (as Group)"), QStringLiteral("object-group"));
    alignToCanvasGroup->setCheckable(true);

    QAction *alignHorLeft = add_action(alginActions, QStringLiteral("align_hor_left"), i18n("Left"), QStringLiteral("align-horizontal-left"));
    connect(alignHorLeft, &QAction::triggered, parent, [this]{align(AlignDirection::Horizontal, AlignPosition::Begin, false);});

    QAction *alignHorCenter = add_action(alginActions, QStringLiteral("align_hor_center"), i18n("Center"), QStringLiteral("align-horizontal-center"));
    connect(alignHorCenter, &QAction::triggered, parent, [this]{align(AlignDirection::Horizontal, AlignPosition::Center, false);});

    QAction *alignHorRight = add_action(alginActions, QStringLiteral("align_hor_right"), i18n("Right"), QStringLiteral("align-horizontal-right"));
    connect(alignHorRight, &QAction::triggered, parent, [this]{align(AlignDirection::Horizontal, AlignPosition::End, false);});

    QAction *alignVertTop = add_action(alginActions, QStringLiteral("align_vert_top"), i18n("Top"), QStringLiteral("align-vertical-top"));
    connect(alignVertTop, &QAction::triggered, parent, [this]{align(AlignDirection::Vertical, AlignPosition::Begin, false);});

    QAction *alignVertCenter = add_action(alginActions, QStringLiteral("align_vert_center"), i18n("Center"), QStringLiteral("align-vertical-center"));
    connect(alignVertCenter, &QAction::triggered, parent, [this]{align(AlignDirection::Vertical, AlignPosition::Center, false);});

    QAction *alignVertBottom = add_action(alginActions, QStringLiteral("align_vert_bottom"), i18n("Bottom"), QStringLiteral("align-vertical-bottom"));
    connect(alignVertBottom, &QAction::triggered, parent, [this]{align(AlignDirection::Vertical, AlignPosition::End, false);});


    // addAction(QStringLiteral("separator_align_relative_to"), i18n("Relative To"), QStringLiteral(""));
    // addAction(QStringLiteral("separator_align_horizontal"), i18n("Horizontal"), QStringLiteral(""));
    // addAction(QStringLiteral("separator_align_vertical"), i18n("Vertical"), QStringLiteral(""));
    QAction *alignHorLeftOut = add_action(alginActions, QStringLiteral("align_hor_left_out"), i18n("Outside Left"), QStringLiteral("align-horizontal-right-out"));
    connect(alignHorLeftOut, &QAction::triggered, parent, [this]{align(AlignDirection::Horizontal, AlignPosition::Begin, true);});

    QAction *alignHorRightOut = add_action(alginActions, QStringLiteral("align_hor_right_out"), i18n("Outside Right"), QStringLiteral("align-horizontal-left-out"));
    connect(alignHorRightOut, &QAction::triggered, parent, [this]{align(AlignDirection::Horizontal, AlignPosition::End, true);});

    QAction *alignVertTopOut = add_action(alginActions, QStringLiteral("align_vert_top_out"), i18n("Outside Top"), QStringLiteral("align-vertical-bottom-out"));
    connect(alignVertTopOut, &QAction::triggered, parent, [this]{align(AlignDirection::Vertical, AlignPosition::Begin, true);});

    QAction *alignVertBottomOut = add_action(alginActions, QStringLiteral("align_vert_bottom_out"), i18n("Outside Bottom"), QStringLiteral("align-vertical-top-out"));
    connect(alignVertBottomOut, &QAction::triggered, parent, [this]{align(AlignDirection::Vertical, AlignPosition::End, true);});
}

void GlaxnimateWindow::Private::setup_path_actions()
{
    KActionCategory *pathActions = new KActionCategory(i18n("Path"), parent->actionCollection());

    QAction *objectToPath = add_action(pathActions, QStringLiteral("object_to_path"), i18n("Object to Path"), QStringLiteral("object-to-path"));
    connect(objectToPath, &QAction::triggered, parent, [this]{to_path();});

    QAction *traceBitmap = add_action(pathActions, QStringLiteral("trace_bitmap"), i18n("Trace Bitmap…"), QStringLiteral("bitmap-trace"));
    connect(traceBitmap, &QAction::triggered, parent, [this]{
        trace_dialog(parent->current_shape());
    });

    QAction *pathAdd = add_action(pathActions, QStringLiteral("path_add"), i18n("Union"), QStringLiteral("path-union"));
    pathAdd->setEnabled(false);
    QAction *pathSubstract = add_action(pathActions, QStringLiteral("path_subtract"), i18n("Difference"), QStringLiteral("path-difference"));
    pathSubstract->setEnabled(false);
    QAction *pathIntersect = add_action(pathActions, QStringLiteral("path_intersect"), i18n("Intersect"), QStringLiteral("path-intersection"));
    pathIntersect->setEnabled(false);
    QAction *pathXor = add_action(pathActions, QStringLiteral("path_xor"), i18n("Exclusion"), QStringLiteral("path-exclusion"));
    pathXor->setEnabled(false);

    QAction *pathReverse = add_action(pathActions, QStringLiteral("path_reverse"), i18n("Reverse"), QStringLiteral("path-reverse"));
    pathReverse->setEnabled(false);

    QAction *nodeRemove = add_action(pathActions, QStringLiteral("node_remove"), i18n("Delete Nodes"), QStringLiteral("format-remove-node"));
    QAction *nodeAdd = add_action(pathActions, QStringLiteral("node_add"), i18n("Add Node…"), QStringLiteral("format-insert-node"));

    QAction *nodeJoin = add_action(pathActions, QStringLiteral("node_join"), i18n("Join Nodes"), QStringLiteral("format-join-node"));
    nodeJoin->setEnabled(false);
    QAction *nodeSplit = add_action(pathActions, QStringLiteral("node_split"), i18n("Split Nodes"), QStringLiteral("format-break-node"));
    nodeSplit->setEnabled(false);

    QAction *nodeTypeCorner = add_action(pathActions, QStringLiteral("node_type_corner"), i18n("Cusp"), QStringLiteral("node-type-cusp"));
    QAction *nodeTypeSmooth = add_action(pathActions, QStringLiteral("node_type_smooth"), i18n("Smooth"), QStringLiteral("node-type-smooth"));
    QAction *nodeTypeSymmetric = add_action(pathActions, QStringLiteral("node_type_symmetric"), i18n("Symmetric"), QStringLiteral("node-type-auto-smooth"));
    QAction *segmentLines = add_action(pathActions, QStringLiteral("segment_lines"), i18n("Make segments straight"), QStringLiteral("node-segment-line"));
    QAction *segmentCurve = add_action(pathActions, QStringLiteral("segment_curve"), i18n("Make segments curved"), QStringLiteral("node-segment-curve"));
    QAction *nodeDissolve = add_action(pathActions, QStringLiteral("node_dissolve"), i18n("Dissolve Nodes"), QStringLiteral("format-node-curve"));

    this->tool_actions["edit"] = {
        parent->actionCollection()->action(QStringLiteral("edit_delete")),
        nodeRemove,
        nodeTypeCorner,
        nodeTypeSmooth,
        nodeTypeSymmetric,
        segmentLines,
        segmentCurve,
        nodeAdd,
        nodeDissolve,
    };

    tools::EditTool* edit_tool = static_cast<tools::EditTool*>(tools::Registry::instance().tool("edit"));
    connect(nodeTypeCorner, &QAction::triggered, parent, [edit_tool]{
        edit_tool->selection_set_vertex_type(math::bezier::Corner);
    });
    connect(nodeTypeSmooth, &QAction::triggered, parent, [edit_tool]{
        edit_tool->selection_set_vertex_type(math::bezier::Smooth);
    });
    connect(nodeTypeSymmetric, &QAction::triggered, parent, [edit_tool]{
        edit_tool->selection_set_vertex_type(math::bezier::Symmetrical);
    });
    connect(nodeRemove, &QAction::triggered, parent, [edit_tool]{
        edit_tool->selection_delete();
    });
    connect(segmentLines, &QAction::triggered, parent, [edit_tool]{
        edit_tool->selection_straighten();
    });
    connect(segmentCurve, &QAction::triggered, parent, [edit_tool]{
        edit_tool->selection_curve();
    });
    connect(nodeAdd, &QAction::triggered, parent, [edit_tool]{
        edit_tool->add_point_mode();
    });
    connect(nodeDissolve, &QAction::triggered, parent, [edit_tool]{
        edit_tool->selection_dissolve();
    });
}

void GlaxnimateWindow::Private::init_tools_ui()
{
    tools_dock = new QDockWidget(i18n("Tools"), this->parent);
    tools_dock->setWindowIcon(QIcon::fromTheme(QStringLiteral("tools")));
    tools_dock->setObjectName(QStringLiteral("dock_tools"));
    parent->addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, tools_dock);

    QWidget *widget = new QWidget(tools_dock);
    dock_tools_layout = new FlowLayout();
    widget->setLayout(dock_tools_layout);
    tools_dock->setWidget(widget);

    for ( const auto& grp : tools::Registry::instance() )
    {
        for ( const auto& tool : grp.second )
        {
            QAction *action = parent->actionCollection()->action(tool.second->action_name());

            ScalableButton *button = tool.second->get_button();

            connect(action, &QAction::toggled, button, &QAbstractButton::setChecked);
            connect(button, &QAbstractButton::clicked, action, &QAction::trigger);
            //ui.toolbar_tools->addAction(action);

            button->resize(16, 16);
            dock_tools_layout->addWidget(button);

            QWidget* widget = tool.second->get_settings_widget();
            widget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
            tool_options_dock->addSettingsWidget(widget);
        }
    }

    KToolBar *toolbar_node = parent->toolBar(QStringLiteral("nodeToolBar"));
    toolbar_node->setVisible(false);
    toolbar_node->setEnabled(false);
}

void GlaxnimateWindow::Private::init_item_views()
{
    // Item views
    layers_dock = new LayersDock(parent, &document_node_model, parent->create_layer_menu());
    connect(layers_dock, &LayersDock::add_layer, parent, [this]{layer_new_layer();});
    connect(layers_dock, &LayersDock::duplicate_layer, parent, &GlaxnimateWindow::layer_duplicate);
    connect(layers_dock, &LayersDock::delete_layer, parent, &GlaxnimateWindow::layer_delete);
    parent->addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, layers_dock);

    QObject::connect(layers_dock->layer_view(), &LayerView::current_node_changed,
            parent, &GlaxnimateWindow::document_treeview_current_changed);

    connect(layers_dock->layer_view(), &LayerView::selection_changed, parent, &GlaxnimateWindow::document_treeview_selection_changed);

    properties_dock = new PropertiesDock(parent, &property_model, &property_delegate);
    parent->addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, properties_dock);

    asset_model.setSourceModel(&document_node_model);

    assets_dock = new AssetsDock(parent, &asset_model, &document_node_model);
    parent->addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, assets_dock);

    connect(timeline_dock->timelineWidget(), &CompoundTimelineWidget::current_node_changed, parent, [this](model::VisualNode* node){
        timeline_current_node_changed(node);
    });
    connect(timeline_dock->timelineWidget(), &CompoundTimelineWidget::selection_changed, parent, [this](const std::vector<model::VisualNode*>& selected,  const std::vector<model::VisualNode*>& deselected){
        selection_changed(selected, deselected);
    });
}

static QWidget* status_bar_separator()
{
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::VLine);
    line->setFrameShadow(QFrame::Sunken);
    return line;
}

void GlaxnimateWindow::Private::init_status_bar()
{
    // Recording
    widget_recording = new QWidget();
    parent->statusBar()->addPermanentWidget(widget_recording);


    QHBoxLayout* lay = new QHBoxLayout;
    widget_recording->setLayout(lay);
    lay->setContentsMargins(0, 0, 0, 0);
    widget_recording->setVisible(false);

    QLabel* label_recording_icon = new QLabel();
    label_recording_icon->setPixmap(QIcon::fromTheme("media-record").pixmap(parent->statusBar()->height()));
    lay->addWidget(label_recording_icon);

    label_recording = new QLabel();
    label_recording->setText(i18n("Recording Keyframes"));
    lay->addWidget(label_recording);

    lay->addWidget(status_bar_separator());

    // X: ... Y: ...
    label_mouse_pos = new QLabel();
    parent->statusBar()->addPermanentWidget(label_mouse_pos);
    QFont font;
    font.setFamily("monospace");
    font.setStyleHint(QFont::Monospace);
    label_mouse_pos->setFont(font);
    mouse_moved(QPointF(0, 0));
    connect(canvas, &Canvas::mouse_moved, parent, [this](const QPointF& p){mouse_moved(p);});

    parent->statusBar()->addPermanentWidget(status_bar_separator());

    // Current Style
    widget_current_style = new ShapeStylePreviewWidget();
    parent->statusBar()->addPermanentWidget(widget_current_style);
    widget_current_style->setFixedSize(parent->statusBar()->height(), parent->statusBar()->height());
    connect(colors_dock, &ColorsDock::current_color_changed,
            widget_current_style, &ShapeStylePreviewWidget::set_fill_color);
    connect(stroke_dock, &StrokeDock::color_changed,
            widget_current_style, &ShapeStylePreviewWidget::set_stroke_color);
    parent->statusBar()->addPermanentWidget(status_bar_separator());
    widget_current_style->set_fill_color(colors_dock->current_color());
    widget_current_style->set_stroke_color(stroke_dock->current_color());

    // Transform widget
    view_trans_widget = new ViewTransformWidget(parent->statusBar());
    parent->statusBar()->addPermanentWidget(view_trans_widget);
    connect(view_trans_widget, &ViewTransformWidget::zoom_changed, canvas, &Canvas::set_zoom);
    connect(canvas, &Canvas::zoomed, view_trans_widget, &ViewTransformWidget::set_zoom);
    connect(view_trans_widget, &ViewTransformWidget::zoom_in, canvas, &Canvas::zoom_in);
    connect(view_trans_widget, &ViewTransformWidget::zoom_out, canvas, &Canvas::zoom_out);
    connect(view_trans_widget, &ViewTransformWidget::angle_changed, canvas, &Canvas::set_rotation);
    connect(canvas, &Canvas::rotated, view_trans_widget, &ViewTransformWidget::set_angle);
    connect(view_trans_widget, &ViewTransformWidget::view_fit, parent, &GlaxnimateWindow::view_fit);

    QAction *flipView = parent->actionCollection()->action(QStringLiteral("flip_view"));
    connect(view_trans_widget, &ViewTransformWidget::flip_view, flipView, &QAction::trigger);
}

void GlaxnimateWindow::Private::init_docks()
{
    scriptconsole_dock = new ScriptConsoleDock(this->parent);
    // Scripting
    connect(scriptconsole_dock, &ScriptConsoleDock::error, parent, [this](const QString& plugin, const QString& message){
        show_warning(plugin, message, app::log::Error);
    });
    scriptconsole_dock->set_global("window", QVariant::fromValue(parent));

    parent->addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, scriptconsole_dock);

    init_plugins();

    // Logs
    log_model.populate(GlaxnimateApp::instance()->log_lines());

    logs_dock = new LogsDock(this->parent, &log_model);
    parent->addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, logs_dock);

    // Swatches
    palette_model.setSearchPaths(app::Application::instance()->data_paths_unchecked("palettes"));
    palette_model.setSavePath(app::Application::instance()->writable_data_path("palettes"));
    palette_model.load();
    colors_dock->set_palette_model(&palette_model);
    stroke_dock->set_palette_model(&palette_model);

    swatches_dock = new SwatchesDock(this->parent, &palette_model);
    parent->addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, swatches_dock);

    connect(swatches_dock, &SwatchesDock::needs_new_color, [this]{
        swatches_dock->add_new_color(colors_dock->current_color());
    });
    connect(swatches_dock, &SwatchesDock::current_color_def, [this](model::BrushStyle* sty){
        set_color_def(sty, false);
    });
    connect(swatches_dock, &SwatchesDock::secondary_color_def, [this](model::BrushStyle* sty){
        set_color_def(sty, true);
    });

    // Gradients
    gradients_dock = new GradientsDock(this->parent);
    parent->addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, gradients_dock);
    connect(gradients_dock, &GradientsDock::selected, parent, [this](model::BrushStyle* sty, bool secondary){
            set_brush_reference(sty, secondary);
    });

    // Fill/Stroke
    connect(colors_dock, &ColorsDock::current_color_changed, parent, [this]{style_change_event();});
    connect(colors_dock, &ColorsDock::secondary_color_changed, parent, [this]{style_change_event();});
    connect(stroke_dock, &StrokeDock::pen_style_changed, parent, [this]{style_change_event();});

    // Tab bar
    connect(tab_bar, &CompositionTabBar::switch_composition, parent, &GlaxnimateWindow::switch_composition);

    timeline_dock = new TimelineDock(this->parent);
    parent->addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, timeline_dock);
    connect(timeline_dock->timelineWidget(), &CompoundTimelineWidget::switch_composition, parent, &GlaxnimateWindow::switch_composition);

    // Align
    /*ui.separator_align_relative_to->setSeparator(true);
    ui.separator_align_horizontal->setSeparator(true);
    ui.separator_align_vertical->setSeparator(true);
    QActionGroup *align_relative = new QActionGroup(parent);
    align_relative->setExclusive(true);
    ui.action_align_to_canvas->setActionGroup(align_relative);
    ui.action_align_to_selection->setActionGroup(align_relative);
    ui.action_align_to_canvas_group->setActionGroup(align_relative);

    auto combo_align_to = new QComboBox(ui.dock_align->widget());
    combo_align_to->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui.dock_align_grid->addWidget(combo_align_to, 0, 0, 1, 3);

    action_combo(combo_align_to, ui.action_align_to_selection);
    action_combo(combo_align_to, ui.action_align_to_canvas);
    action_combo(combo_align_to, ui.action_align_to_canvas_group);
    connect(combo_align_to, qOverload<int>(&QComboBox::currentIndexChanged), parent, [combo_align_to](int i){
        combo_align_to->itemData(i).value<QAction*>()->setChecked(true);
    });
    */

    align_dock = new AlignDock(this->parent);
    parent->addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, align_dock);

    time_slider_dock = new TimeSliderDock(this->parent);
    parent->addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, time_slider_dock);

    snippets_dock = new SnippetsDock(this->parent);
    parent->addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, snippets_dock);

    undo_dock = new UndoDock(this->parent);
    parent->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, undo_dock);
    undo_dock->setUndoGroup(&parent->undo_group());

    tool_options_dock = new ToolOptionsDock(this->parent);
    parent->addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, tool_options_dock);
}

void GlaxnimateWindow::Private::layout_update()
{
    QAction *layoutWide = parent->actionCollection()->action(QStringLiteral("layout_wide"));
    QAction *layoutCompact = parent->actionCollection()->action(QStringLiteral("layout_compact"));
    QAction *layoutCustom = parent->actionCollection()->action(QStringLiteral("layout_custom"));
    QAction *layoutMedium = parent->actionCollection()->action(QStringLiteral("layout_medium"));


    if ( layoutWide->isChecked() )
        layout_wide();
    else if ( layoutCompact->isChecked() )
        layout_compact();
    else if ( layoutCustom->isChecked() )
        layout_custom();
    else if ( layoutMedium->isChecked() )
        layout_medium();
    else
        layout_auto();
}

void GlaxnimateWindow::Private::layout_medium()
{
    // Bottom
    parent->addDockWidget(Qt::BottomDockWidgetArea, timeline_dock);
    parent->tabifyDockWidget(timeline_dock, properties_dock);
    parent->tabifyDockWidget(properties_dock, scriptconsole_dock);
    parent->tabifyDockWidget(scriptconsole_dock, logs_dock);
    parent->tabifyDockWidget(logs_dock, time_slider_dock);
    timeline_dock->raise();
    time_slider_dock->setVisible(false);
    timeline_dock->setVisible(true);
    properties_dock->setVisible(true);

    parent->addDockWidget(Qt::BottomDockWidgetArea, snippets_dock);

    // Bottom Right
    parent->addDockWidget(Qt::BottomDockWidgetArea, layers_dock);
    parent->tabifyDockWidget(layers_dock, gradients_dock);
    parent->tabifyDockWidget(gradients_dock, swatches_dock);
    parent->tabifyDockWidget(swatches_dock, assets_dock);
    gradients_dock->raise();
    gradients_dock->setVisible(true);
    assets_dock->setVisible(false);
    swatches_dock->setVisible(false);
    layers_dock->setVisible(true);


    // Right
    parent->addDockWidget(Qt::RightDockWidgetArea, colors_dock);
    parent->tabifyDockWidget(colors_dock, stroke_dock);
    parent->tabifyDockWidget(stroke_dock, undo_dock);
    colors_dock->raise();
    colors_dock->setVisible(true);
    stroke_dock->setVisible(true);
    undo_dock->setVisible(true);


    // Left
    parent->addDockWidget(Qt::LeftDockWidgetArea, tools_dock);

    parent->addDockWidget(Qt::LeftDockWidgetArea, tool_options_dock);
    parent->tabifyDockWidget(tool_options_dock, align_dock);
    tool_options_dock->raise();
    tool_options_dock->setVisible(true);
    align_dock->setVisible(true);

    // Resize
    parent->resizeDocks({snippets_dock}, {1}, Qt::Horizontal);
    parent->resizeDocks({layers_dock}, {1}, Qt::Horizontal);
    parent->resizeDocks({tools_dock}, {200}, Qt::Horizontal);
    parent->resizeDocks({tool_options_dock, align_dock, tools_dock}, {1, 1, 4000}, Qt::Vertical);
    parent->resizeDocks({timeline_dock}, {300}, Qt::Vertical);
    scriptconsole_dock->setVisible(false);
    logs_dock->setVisible(false);
    tools_dock->setVisible(false);
    snippets_dock->setVisible(false);

    // Resize parent to have a reasonable default size
    parent->resize(1440, 900);

    app::settings::set("ui", "layout", int(LayoutPreset::Medium));
    QAction *layoutMedium = parent->actionCollection()->action(QStringLiteral("layout_medium"));
    layoutMedium->setChecked(true);
}

void GlaxnimateWindow::Private::layout_wide()
{
    // Bottom
    parent->addDockWidget(Qt::BottomDockWidgetArea, timeline_dock);
    parent->tabifyDockWidget(timeline_dock, properties_dock);
    parent->tabifyDockWidget(properties_dock, scriptconsole_dock);
    parent->tabifyDockWidget(scriptconsole_dock, logs_dock);
    parent->tabifyDockWidget(logs_dock, time_slider_dock);
    timeline_dock->raise();
    time_slider_dock->setVisible(false);
    timeline_dock->setVisible(true);
    properties_dock->setVisible(true);

    parent->addDockWidget(Qt::BottomDockWidgetArea, snippets_dock);

    // Bottom Right
    parent->addDockWidget(Qt::BottomDockWidgetArea, layers_dock);
    parent->tabifyDockWidget(layers_dock, gradients_dock);
    parent->tabifyDockWidget(gradients_dock, swatches_dock);
    parent->tabifyDockWidget(swatches_dock, assets_dock);
    gradients_dock->raise();
    gradients_dock->setVisible(true);
    assets_dock->setVisible(false);
    swatches_dock->setVisible(false);
    layers_dock->setVisible(true);


    // Right
    parent->addDockWidget(Qt::RightDockWidgetArea, colors_dock);
    parent->tabifyDockWidget(colors_dock, stroke_dock);
    parent->tabifyDockWidget(stroke_dock, undo_dock);
    colors_dock->raise();
    colors_dock->setVisible(true);
    stroke_dock->setVisible(true);
    undo_dock->setVisible(true);


    // Left
    parent->addDockWidget(Qt::LeftDockWidgetArea, tools_dock);

    parent->addDockWidget(Qt::LeftDockWidgetArea, tool_options_dock);
    parent->addDockWidget(Qt::LeftDockWidgetArea, align_dock);
    tool_options_dock->setVisible(true);
    align_dock->setVisible(true);

    // Resize
    parent->resizeDocks({snippets_dock}, {1}, Qt::Horizontal);
    parent->resizeDocks({layers_dock}, {1}, Qt::Horizontal);
    parent->resizeDocks({tools_dock}, {200}, Qt::Horizontal);
    parent->resizeDocks({tool_options_dock, align_dock, tools_dock}, {1, 1, 4000}, Qt::Vertical);
    parent->resizeDocks({timeline_dock}, {1080/3}, Qt::Vertical);
    scriptconsole_dock->setVisible(false);
    logs_dock->setVisible(false);
    tools_dock->setVisible(false);
    snippets_dock->setVisible(false);

    // Resize parent to have a reasonable default size
    parent->resize(1920, 1080);

    app::settings::set("ui", "layout", int(LayoutPreset::Wide));
    QAction *layoutWide = parent->actionCollection()->action(QStringLiteral("layout_wide"));
    layoutWide->setChecked(true);
}

void GlaxnimateWindow::Private::layout_compact()
{
    // Bottom
    parent->addDockWidget(Qt::BottomDockWidgetArea, time_slider_dock);
    parent->tabifyDockWidget(time_slider_dock, timeline_dock);
    parent->tabifyDockWidget(timeline_dock, scriptconsole_dock);
    parent->tabifyDockWidget(scriptconsole_dock, logs_dock);
    time_slider_dock->raise();
    time_slider_dock->setVisible(true);
    timeline_dock->setVisible(false);
    logs_dock->setVisible(false);

    parent->addDockWidget(Qt::BottomDockWidgetArea, snippets_dock);

    // Right
    parent->addDockWidget(Qt::RightDockWidgetArea, colors_dock);
    parent->tabifyDockWidget(colors_dock, stroke_dock);
    parent->tabifyDockWidget(stroke_dock, layers_dock);
    parent->tabifyDockWidget(layers_dock, properties_dock);
    parent->tabifyDockWidget(properties_dock, undo_dock);
    parent->tabifyDockWidget(undo_dock, gradients_dock);
    parent->tabifyDockWidget(gradients_dock, swatches_dock);
    parent->tabifyDockWidget(swatches_dock, assets_dock);
    colors_dock->raise();
    colors_dock->setVisible(true);
    stroke_dock->setVisible(true);
    gradients_dock->setVisible(false);
    assets_dock->setVisible(false);
    swatches_dock->setVisible(false);
    undo_dock->setVisible(false);
    properties_dock->setVisible(true);
    layers_dock->setVisible(true);

    // Left
    parent->addDockWidget(Qt::LeftDockWidgetArea, tools_dock);

    parent->addDockWidget(Qt::LeftDockWidgetArea, tool_options_dock);
    parent->tabifyDockWidget(tool_options_dock, align_dock);
    tool_options_dock->raise();
    tool_options_dock->setVisible(true);
    align_dock->setVisible(true);

    // Resize
    parent->resizeDocks({tool_options_dock, align_dock, tools_dock}, {1, 1, 4000}, Qt::Vertical);
    parent->resizeDocks({time_slider_dock}, {64}, Qt::Vertical);
    scriptconsole_dock->setVisible(false);
    logs_dock->setVisible(false);
    tools_dock->setVisible(false);
    snippets_dock->setVisible(false);

    // Resize parent to have a reasonable default size
    parent->resize(1366, 768);

    app::settings::set("ui", "layout", int(LayoutPreset::Compact));
    QAction *layoutCompact = parent->actionCollection()->action(QStringLiteral("layout_compact"));
    layoutCompact->setChecked(true);
}

void GlaxnimateWindow::Private::layout_auto()
{
    auto real_estate = qApp->primaryScreen()->availableSize();
    if ( real_estate.width() >= 1920 && real_estate.height() >= 1080 )
        layout_wide();
    else if ( real_estate.width() >= 1440 && real_estate.height() >= 900 )
        layout_medium();
    else
        layout_compact();

    QAction *layoutAuto = parent->actionCollection()->action(QStringLiteral("layout_automatic"));
    layoutAuto->setChecked(true);
}

void GlaxnimateWindow::Private::layout_custom()
{
    init_restore_state();
    QAction *layoutCustom = parent->actionCollection()->action(QStringLiteral("layout_custom"));
    layoutCustom->setChecked(true);
}

void GlaxnimateWindow::Private::init_template_menu()
{
    QList<QAction*> template_actions;

    for ( const auto& templ : settings::DocumentTemplates::instance().templates() )
        template_actions.append(settings::DocumentTemplates::instance().create_action(templ, this->parent));

    parent->unplugActionList( "template_actionlist" );
    parent->plugActionList( "template_actionlist", template_actions );
}

void GlaxnimateWindow::Private::init_menus()
{
    // Menu Views
    QList<QAction*> view_actions;

    for ( QDockWidget* wid : parent->findChildren<QDockWidget*>() )
    {
        QAction* act = wid->toggleViewAction();
        act->setIcon(wid->windowIcon());
        view_actions.append(act);
        wid->setStyle(&dock_style);
    }
    parent->unplugActionList( "view_actionlist" );
    parent->plugActionList( "view_actionlist", view_actions );

    // Menu Toolbars
    /*for ( QToolBar* wid : parent->findChildren<QToolBar*>() )
    {
        QAction* act = wid->toggleViewAction();
        ui.menu_toolbars->addAction(act);
        wid->setStyle(&dock_style);
    }*/

    // Menu Templates
    init_template_menu();

    connect(&settings::DocumentTemplates::instance(), &settings::DocumentTemplates::loaded, parent, [this]{
        init_template_menu();
    });

    connect(&settings::DocumentTemplates::instance(), &settings::DocumentTemplates::create_from, parent,
        [this](const settings::DocumentTemplate& templ){
            if ( !close_document() )
                return;

            bool ok = false;
            setup_document_ptr(templ.create(&ok));
            if ( !ok )
                show_warning(i18n("New from Template"), i18n("Could not load template"));
        }
    );


}


void GlaxnimateWindow::Private::init_tools(tools::Tool* to_activate)
{
    tools::Event event{canvas, &scene, parent};
    for ( const auto& grp : tools::Registry::instance() )
    {
        for ( const auto& tool : grp.second )
        {
            tool.second->initialize(event);

            if ( to_activate == tool.second.get() )
                switch_tool(tool.second.get());
        }
    }
}

void GlaxnimateWindow::Private::init_restore_state()
{
    parent->restoreState(app::settings::get<QByteArray>("ui", "window_state"));
    timeline_dock->timelineWidget()->load_state(app::settings::get<QByteArray>("ui", "timeline_splitter"));
    parent->restoreGeometry(app::settings::get<QByteArray>("ui", "window_geometry"));

    // Hide tool widgets, as they might get shown by restoreState
    KToolBar *toolbar_node = parent->toolBar(QStringLiteral("nodeToolBar"));
    toolbar_node->setVisible(false);
    toolbar_node->setEnabled(false);
}

void GlaxnimateWindow::Private::view_fit()
{
    canvas->view_fit();
}

void GlaxnimateWindow::Private::show_warning(const QString& title, const QString& message, app::log::Severity icon)
{
    KMessageWidget::MessageType message_type;
    switch ( icon )
    {
        case app::log::Severity::Info: message_type = KMessageWidget::Information; break;
        case app::log::Severity::Warning: message_type = KMessageWidget::Warning; break;
        case app::log::Severity::Error: message_type = KMessageWidget::Error; break;
    }
    message_widget->queue_message({message, message_type});
    app::log::Log(title).log(message, icon);
}

void GlaxnimateWindow::Private::help_about_env()
{
    about_env_dialog->show();
}

void GlaxnimateWindow::Private::copyDebugInfo()
{
    // General note for this function: since the information targets developers, we don't want it it be translated

    QString debuginfo = QStringLiteral("%1: %2\n").arg(KAboutData::applicationData().displayName(), KAboutData::applicationData().version());

    QString packageType = QStringLiteral("Unknown/Default");

    if (KSandbox::isFlatpak()) {
        packageType = QStringLiteral("Flatpak");
    }

    if (KSandbox::isSnap()) {
        packageType = QStringLiteral("Snap");
    }

    QString appPath = qApp->applicationDirPath();
    if (appPath.contains(QStringLiteral("/tmp/.mount_"))) {
        packageType = QStringLiteral("AppImage");
    }

    debuginfo.append(QStringLiteral("Package Type: %1\n").arg(packageType));
    debuginfo.append(QStringLiteral("Qt: %1 (built against %2 %3)\n").arg(QString::fromLocal8Bit(qVersion()), QT_VERSION_STR, QSysInfo::buildAbi()));
    debuginfo.append(QStringLiteral("Frameworks: %2\n").arg(KCoreAddons::versionString()));
    debuginfo.append(QStringLiteral("System: %1\n").arg(QSysInfo::prettyProductName()));
    debuginfo.append(QStringLiteral("Kernel: %1 %2\n").arg(QSysInfo::kernelType(), QSysInfo::kernelVersion()));
    debuginfo.append(QStringLiteral("CPU: %1\n").arg(QSysInfo::currentCpuArchitecture()));
    debuginfo.append(QStringLiteral("Windowing System: %1\n").arg(QGuiApplication::platformName()));

    for ( KAboutComponent &component : KAboutData::applicationData().components())
    {
        debuginfo.append(QStringLiteral("%1: %2\n").arg(component.name(), component.version()));
    }

    QClipboard *clipboard = qApp->clipboard();
    clipboard->setText(debuginfo);
}

void GlaxnimateWindow::Private::shutdown()
{
    KSharedConfigPtr config = KSharedConfig::openConfig();
    app::settings::set("ui", "window_geometry", parent->saveGeometry());
    app::settings::set("ui", "window_state", parent->saveState());
    app::settings::set("ui", "timeline_splitter", timeline_dock->timelineWidget()->save_state());
    m_recentFilesAction->saveEntries(KConfigGroup(config, QString()));

    colors_dock->save_settings();

    stroke_dock->save_settings();

    scriptconsole_dock->save_settings();
    scriptconsole_dock->clear_contexts();
}


void GlaxnimateWindow::Private::switch_tool(tools::Tool* tool)
{
    if ( !tool || tool == active_tool )
        return;

    QAction *action = parent->actionCollection()->action(tool->action_name());
    if ( !action->isChecked() )
        action->setChecked(true);

    if ( active_tool )
    {
        if (active_tool->id() == QStringLiteral("edit") )
        {
            KToolBar *toolbar_node = parent->toolBar(QStringLiteral("nodeToolBar"));
            toolbar_node->setVisible(false);
            toolbar_node->setEnabled(false);
        }

        for ( const auto& action : tool_actions[active_tool->id()] )
        {
            action->setEnabled(false);
        }
    }

    if (tool->id() == QStringLiteral("edit") )
    {
        KToolBar *toolbar_node = parent->toolBar(QStringLiteral("nodeToolBar"));
        toolbar_node->setVisible(true);
        toolbar_node->setEnabled(true);
    }

    for ( const auto& action : tool_actions[tool->id()] )
    {
        action->setEnabled(true);
    }

    active_tool = tool;
    scene.set_active_tool(tool);
    canvas->set_active_tool(tool);

    if ( auto old_wid = tool_options_dock->currentWidget() )
        old_wid->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    auto new_wid = tool->get_settings_widget();
    new_wid->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    tool_options_dock->setCurrentWidget(new_wid);
    if ( active_tool->group() == tools::Registry::Draw || active_tool->group() == tools::Registry::Shape )
        widget_current_style->clear_gradients();
}

void GlaxnimateWindow::Private::switch_tool_action(QAction* action)
{
    switch_tool(action->data().value<tools::Tool*>());
}

void GlaxnimateWindow::Private::status_message(const QString& message, int duration)
{
    parent->statusBar()->showMessage(message, duration);
}

void GlaxnimateWindow::Private::trace_dialog(model::DocumentNode* object)
{
    model::Image* bmp = nullptr;
    if ( object )
    {
        bmp = object->cast<model::Image>();
    }

    if ( !bmp )
    {
        for ( const auto& sel : scene.selection() )
        {
            if ( auto image = sel->cast<model::Image>() )
            {
                if ( bmp )
                {
                    show_warning(i18n("Trace Bitmap"), i18n("Only select one image"), app::log::Info);
                    return;
                }
                bmp = image;
            }
        }

        if ( !bmp )
        {
            show_warning(i18n("Trace Bitmap"), i18n("You need to select an image to trace"), app::log::Info);
            return;
        }
    }

    if ( !bmp->image.get() )
    {
        show_warning(i18n("Trace Bitmap"), i18n("You selected an image with no data"), app::log::Info);
        return;
    }

    TraceDialog dialog(bmp, parent);
    dialog.exec();
    if ( auto created = dialog.created() )
        layers_dock->layer_view()->set_current_node(created);
}


void GlaxnimateWindow::Private::init_plugins()
{
    auto& par = plugin::PluginActionRegistry::instance();
    for ( auto act : par.enabled() )
    {
        plugin_actions.append(par.make_qaction(act));
    }

    parent->unplugActionList("plugins_actionlist");
    parent->plugActionList("plugins_actionlist", plugin_actions);

    connect(&par, &plugin::PluginActionRegistry::action_added, parent, [this](plugin::ActionService* action, plugin::ActionService* before) {
        qsizetype index = -1;
        for ( auto act : plugin_actions )
        {
            if ( act->data().value<plugin::ActionService*>() == before )
            {
                index = plugin_actions.indexOf(act);
                break;
            }
        }
        QAction* qaction = plugin::PluginActionRegistry::instance().make_qaction(action);
        plugin_actions.insert(qMax(qsizetype(0), qsizetype(index -1)), qaction);

        parent->unplugActionList("plugins_actionlist");
        parent->plugActionList("plugins_actionlist", plugin_actions);
    });

    // TODO ?
    /*
    connect(&par, &plugin::PluginActionRegistry::action_removed, parent, [](plugin::ActionService* action) {
        QString slug = "action_plugin_" + action->plugin()->data().name.toLower() + "_" + action->label.toLower();
    });
    */

    connect(
        &plugin::PluginRegistry::instance(),
        &plugin::PluginRegistry::loaded,
        scriptconsole_dock,
        &ScriptConsoleDock::clear_contexts
    );

    plugin::PluginRegistry::instance().set_executor(scriptconsole_dock);
}

void GlaxnimateWindow::Private::mouse_moved(const QPointF& pos)
{
    label_mouse_pos->setText(ki18n("X: %1 Y: %2").subs(pos.x(), 8, 'f', 3).subs(pos.y(), 8, 'f', 3).toString());
}

void GlaxnimateWindow::Private::show_startup_dialog()
{
    if ( !app::settings::get<bool>("ui", "startup_dialog") )
        return;

    StartupDialog dialog(parent);
    connect(&dialog, &StartupDialog::open_recent, parent, &GlaxnimateWindow::document_open);
    connect(&dialog, &StartupDialog::open_browse, parent, &GlaxnimateWindow::document_open_dialog);
    if ( dialog.exec() )
        setup_document_ptr(dialog.create());
}


void GlaxnimateWindow::Private::drop_file(const QString& file)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(i18n("Drop File"));
    QIcon icon = QIcon::fromTheme("dialog-question");
    dialog.setWindowIcon(icon);
    QVBoxLayout lay;
    dialog.setLayout(&lay);

    QHBoxLayout lay1;
    QLabel label_icon;
    label_icon.setPixmap(icon.pixmap(64));
    lay1.addWidget(&label_icon);
    QLabel label_text;
    label_text.setText(i18n("Add to current file?"));
    label_text.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    lay1.addWidget(&label_text);
    lay.addLayout(&lay1);

    QHBoxLayout lay2;
    QPushButton btn1(i18n("Add as Object"));
    lay2.addWidget(&btn1);
    connect(&btn1, &QPushButton::clicked, &dialog, [&dialog, this, &file]{
        drop_document(file, false);
        dialog.accept();
    });
    QPushButton btn2(i18n("Add as Composition"));
    lay2.addWidget(&btn2);
    connect(&btn2, &QPushButton::clicked, &dialog, [&dialog, this, &file]{
        drop_document(file, true);
        dialog.accept();
    });
    QPushButton btn3(i18n("Open"));
    lay2.addWidget(&btn3);
    connect(&btn3, &QPushButton::clicked, &dialog, [&dialog, this, &file]{
        document_open_from_filename(file);
        dialog.accept();
    });
    lay.addLayout(&lay2);

    dialog.exec();
}

void GlaxnimateWindow::Private::insert_emoji()
{
    emoji::EmojiSetDialog dialog;
    if ( !dialog.exec() || dialog.selected_svg().isEmpty() )
        return;
    import_file(dialog.selected_svg(), {{"forced_size", comp->size()}});
}

void GlaxnimateWindow::Private::import_from_lottiefiles()
{
    LottieFilesSearchDialog dialog;
    if ( !dialog.exec() )
        return;

    io::Options options;
    options.format = io::IoRegistry::instance().from_slug("lottie");
    options.filename = dialog.selected_name() + ".json";

    load_remote_document(dialog.selected_url(), options, dialog.result() == LottieFilesSearchDialog::Open);
}

QVariant GlaxnimateWindow::Private::choose_option(const QString& label, const QVariantMap& options, const QString& title) const
{
    QDialog dialog(parent);
    if ( !title.isEmpty() )
        dialog.setWindowTitle(title);

    QVBoxLayout* layout = new QVBoxLayout();
    dialog.setLayout(layout);

    QLabel qlabel;
    qlabel.setText(label);
    layout->addWidget(&qlabel);

    QComboBox box;
    layout->addWidget(&box);
    int count = 0;
    for ( auto it = options.begin(); it != options.end(); ++it )
    {
        box.insertItem(count++, it.key(), *it);
    }

    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

    QDialogButtonBox buttons;
    buttons.addButton(QDialogButtonBox::Ok);
    buttons.addButton(QDialogButtonBox::Cancel);
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(&buttons);

    if ( !dialog.exec() )
        return {};

    return box.currentData();
}
