/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 * SPDX-FileCopyrightText: 2024 Julius Künzel <julius.kuenzel@kde.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/model/animation/edit_utils.hpp"
#include "glaxnimate_window_p.hpp"
#include "widgets/timeline/timeline_widget.hpp"

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
#include <QKeySequence>
#include <QList>
#include <QMenu>

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
#define HAVE_STYLE_MANAGER __has_include(<KStyleManager>)
#if HAVE_STYLE_MANAGER
#include <KStyleManager>
#endif
#include <KXMLGUIFactory>

#include "glaxnimate_settings.hpp"
#include "keyframe_editor_dialog.hpp"

#include "tools/base.hpp"
#include "glaxnimate/model/shapes/composable/image.hpp"
#include "glaxnimate/model/shapes/modifiers/repeater.hpp"
#include "glaxnimate/model/shapes/modifiers/trim.hpp"
#include "glaxnimate/model/shapes/modifiers/inflate_deflate.hpp"
#include "glaxnimate/model/shapes/modifiers/round_corners.hpp"
#include "glaxnimate/model/shapes/modifiers/offset_path.hpp"
#include "glaxnimate/model/shapes/modifiers/zig_zag.hpp"
#include "glaxnimate/io/io_registry.hpp"
#include "glaxnimate/utils/data_paths.hpp"

#include "glaxnimate/command/undo_macro_guard.hpp"
#include "glaxnimate/command/shape_commands.hpp"

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
#include "widgets/docks/script_console.hpp"
#include "widgets/docks/timelinedock.h"
#include "widgets/timeline/keyframe_transition_data.hpp"
#include "widgets/lottiefiles/lottiefiles_search_dialog.hpp"
#include "widgets/settings/settings_dialog.hpp"
#include "widgets/tools/shape_tool_widget.hpp"

#include "widgets/view_transform_widget.hpp"
#include "widgets/flow_layout.hpp"
#include "widgets/shape_style/shape_style_preview_widget.hpp"

#include "tools/edit_tool.hpp"
#include "plugin/action.hpp"
#include "glaxnimate_app.hpp"
#include "settings/document_templates.hpp"
#include "emoji/emoji_set_dialog.hpp"

#include "widgets/docks/layersdock.h"
#include "settings/widget_builder.hpp"


using namespace glaxnimate::gui;


void GlaxnimateWindow::Private::setupUi(bool restore_state, bool debug, GlaxnimateWindow* parent)
{
    this->parent = parent;

    // Central Widget
    QWidget* central_widget = new QWidget(parent);
    QVBoxLayout* central_layout = new QVBoxLayout(central_widget);
    central_widget->setLayout(central_layout);
    central_layout->setContentsMargins(0, 0, 0, 0);
    central_layout->setSpacing(0);

    render_widget = RenderWidget(central_widget);
    canvas = new Canvas(render_widget.widget());
    canvas->setAcceptDrops(true);
    parent->setAcceptDrops(true);
    render_widget.set_overlay(canvas);

    message_widget = new WindowMessageWidget(central_widget);
    tab_bar = new CompositionTabBar(central_widget);

    central_layout->addWidget(message_widget);
    central_layout->addWidget(tab_bar);
    central_layout->addWidget(render_widget.widget());

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
    KActionCategory* text_actions = new KActionCategory(i18n("Text"), parent->actionCollection());

    QAction* textOnPath = add_action(text_actions, QStringLiteral("text_put_on_path"), i18n("Put on Path"), QStringLiteral("text-put-on-path"));
    connect(textOnPath, &QAction::triggered, parent, [this]{text_put_on_path();});

    QAction* textRemovePath = add_action(text_actions, QStringLiteral("text_remove_from_path"), i18n("Remove from Path"), QStringLiteral("text-remove-from-path"));
    connect(textRemovePath, &QAction::triggered, parent, [this]{text_remove_from_path();});

    // Load themes
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    KColorSchemeManager* manager = KColorSchemeManager::instance();
#else
    KColorSchemeManager* manager = new KColorSchemeManager(parent);
#endif
    auto* color_selection_menu = KColorSchemeMenu::createMenu(manager, parent);
    parent->actionCollection()->addAction(QStringLiteral("colorscheme_menu"), color_selection_menu);

#if HAVE_STYLE_MANAGER
    // style configure action is only visible on non KDE platform
    parent->actionCollection()->addAction(QStringLiteral("styles_menu"), KStyleManager::createConfigureAction(parent));
#endif

    // Actions: Settings and Help
    QAction* copy_debug = new QAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy Debug Information"), parent);
    parent->actionCollection()->addAction(QStringLiteral("copy_debuginfo"), copy_debug);
    connect(copy_debug, &QAction::triggered, parent, &GlaxnimateWindow::copyDebugInfo);

    QAction* about_env = new QAction(QIcon::fromTheme(QStringLiteral("help-about-symbolic")), i18n("About Environment"), parent);
    parent->actionCollection()->addAction(QStringLiteral("about_env"), about_env);

    KStandardAction::preferences(parent, [this]{
            if (KConfigDialog::showDialog(QStringLiteral("settings"))) {
                return;
            }

            SettingsDialog dialog(this->parent);
            dialog.exec();
            render_widget.update_from_settings();
        }, parent->actionCollection());

    // Get rid of the blasted action that uses F1 shortcut
    auto axef1 = connect(parent->actionCollection(), &KActionCollection::inserted, parent, [parent](QAction* action){
        if ( action->objectName() == KStandardAction::name(KStandardAction::HelpContents) )
        {
            action->setShortcut({});
            parent->actionCollection()->removeAction(action);
        }
    });
    // Main Window
    parent->setupGUI();
    disconnect(axef1);

    parent->setDockOptions(parent->dockOptions() | QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks);

    parent->setCentralWidget(central_widget);

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
    connect(about_env, &QAction::triggered, parent, &GlaxnimateWindow::help_about_env);

    // Docks
    init_docks();

    init_item_views();

    // Initialize tools
    init_tools_ui();
    init_tools(to_activate);

    connect_playback_actions();

    auto preset = LayoutPreset(GlaxnimateSettings::layout());

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
        case LayoutPreset::Mobile:
            layout_mobile();
            break;
        case LayoutPreset::Custom:
            layout_auto();
            GlaxnimateSettings::setLayout(int(LayoutPreset::Custom));
            QAction* layoutCustom = parent->actionCollection()->action(QStringLiteral("layout_custom"));
            layoutCustom->setChecked(true);
            break;
    }

    // Menus
    init_menus();

    // Debug menu
    if ( debug )
        init_debug();

    // Redirect help entry to our own function
    auto help = KStandardAction::helpContents(parent, &GlaxnimateWindow::help_manual, parent->actionCollection());
    help->setShortcut({});
    // Replug it in the Help menu
    QMenu* help_menu = static_cast<QMenu*>(parent->factory()->container(QStringLiteral("help"), parent));
    if (help_menu) {
        QAction* whats_this = parent->actionCollection()->action(KStandardAction::name(KStandardAction::WhatsThis));
        help_menu->insertAction(whats_this, help);
    }

    // Restore state
    // NOTE: keep at the end so we do this once all the widgets are in their default spots
    if ( restore_state )
        init_restore_state();
}

template<class T>
void GlaxnimateWindow::Private::add_modifier_menu_action(KActionCategory* collection)
{
    QAction* action = add_action(collection, "new_" + T::static_class_name().toLower(), T::static_type_name_human());
    action->setIcon(T::static_tree_icon());
    connect(action, &QAction::triggered, [this]{
        auto layer = std::make_unique<T>(current_document.get());
        parent->layer_new_impl(std::move(layer));
    });
}

QAction* GlaxnimateWindow::Private::add_action(KActionCategory* category, const QString &id, const QString &text, const QString &iconName, const QString &toolTip, const QKeySequence &shortcut)
{
    QAction* action = new QAction(parent);
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
    //QAction* clearAction = add_action(QStringLiteral("clear"), i18n("Clear"), QIcon::fromTheme("edit-select-symbolic"), {}, Qt::CTRL | Qt::Key_L)

    KActionCategory* file_actions = new KActionCategory(i18n("File"), parent->actionCollection());

    // File
    file_actions->addAction(KStandardActions::New, parent, &GlaxnimateWindow::document_new);
    file_actions->addAction(KStandardActions::Open, parent, &GlaxnimateWindow::document_open_dialog);
    QAction* import_image = add_action(file_actions, QStringLiteral("import_image"), i18n("Add Image…"), QStringLiteral("insert-image"), {}, Qt::CTRL | Qt::Key_I);
    connect(import_image, &QAction::triggered, parent, [this]{this->import_image();});

    QAction* document_import = add_action(file_actions, QStringLiteral("import"), i18n("Import…"), QStringLiteral("document-import"), {}, Qt::CTRL | Qt::SHIFT | Qt::Key_I);
    connect(document_import, &QAction::triggered, parent, [this]{import_file();});

    QAction* import_lottie = add_action(file_actions, QStringLiteral("open_lottiefiles"), i18n("Import from LottieFiles…"), QStringLiteral("lottiefiles"));
    connect(import_lottie, &QAction::triggered, parent, [this]{import_from_lottiefiles();});

    QAction* open_last = add_action(file_actions, QStringLiteral("open_last"), i18n("Open Most Recent"), QStringLiteral("document-open-recent"), {}, Qt::CTRL | Qt::SHIFT | Qt::Key_O);
    connect(open_last, &QAction::triggered, parent, [this]{
        QList<QUrl> recent_file_urls = m_recentFilesAction->urls();
        if ( !recent_file_urls.isEmpty() )
        {
            // Avoid references to recent_files
            QUrl url = recent_file_urls.first();
            document_open_from_url(url);
        }
    });

    m_recentFilesAction = KStandardAction::openRecent(parent, &GlaxnimateWindow::document_open_recent, file_actions);
    file_actions->addAction(KStandardActions::name(KStandardActions::OpenRecent), m_recentFilesAction);
    open_last->setEnabled(false);
    connect(m_recentFilesAction, &KRecentFilesAction::enabledChanged, open_last, &QAction::setEnabled);

    m_recentFilesAction->loadEntries(KConfigGroup(KSharedConfig::openConfig(), QString()));

    QAction* revert_action = file_actions->addAction(KStandardActions::Revert, parent, &GlaxnimateWindow::document_reload);

    parent->actionCollection()->setDefaultShortcut(revert_action, Qt::CTRL | Qt::Key_F5);
    file_actions->addAction(KStandardActions::Save, parent, &GlaxnimateWindow::document_save);
    file_actions->addAction(KStandardActions::SaveAs, parent, &GlaxnimateWindow::document_save_as);
    QAction* save_as_template = add_action(file_actions, QStringLiteral("save_as_template"), i18n("Save as Template"), QStringLiteral("document-save-as-template"));
    connect(save_as_template, &QAction::triggered, parent, [this]{
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

    QAction* document_export = add_action(file_actions, QStringLiteral("export"), i18n("Export…"), QStringLiteral("document-export"), {}, Qt::CTRL | Qt::Key_E);
    connect(document_export, &QAction::triggered, parent, &GlaxnimateWindow::document_export);

    QAction* export_as = add_action(file_actions, QStringLiteral("export_as"), i18n("Export As…"), QStringLiteral("document-export"), {}, Qt::CTRL | Qt::SHIFT | Qt::Key_E);
    connect(export_as, &QAction::triggered, parent, &GlaxnimateWindow::document_export_as);

    QAction* export_sequence = add_action(file_actions, QStringLiteral("export_sequence"), i18n("Export as Image Sequence…"), QStringLiteral("folder-images"));
    connect(export_sequence, &QAction::triggered, parent, &GlaxnimateWindow::document_export_sequence);

    KStandardAction::close(parent, &GlaxnimateWindow::close, parent->actionCollection());
    KStandardAction::quit(qApp, &QCoreApplication::quit, parent->actionCollection());
}

void GlaxnimateWindow::Private::setup_edit_actions()
{
    KActionCategory* edit_actions = new KActionCategory(i18n("Edit"), parent->actionCollection());

    QAction* undo = edit_actions->addAction(KStandardActions::Undo, &parent->undo_group(), &QUndoGroup::undo);
    QObject::connect(&parent->undo_group(), &QUndoGroup::canUndoChanged, undo, &QAction::setEnabled);
    QObject::connect(&parent->undo_group(), &QUndoGroup::undoTextChanged, undo, [undo](const QString& s){
        undo->setText(i18n("Undo %1", s));
    });

    QAction* redo = edit_actions->addAction(KStandardActions::Redo, &parent->undo_group(), &QUndoGroup::redo);
    QObject::connect(&parent->undo_group(), &QUndoGroup::canRedoChanged, redo, &QAction::setEnabled);
    QObject::connect(&parent->undo_group(), &QUndoGroup::redoTextChanged, redo, [redo](const QString& s){
        redo->setText(i18n("Redo %1", s));
    });
    edit_actions->addAction(KStandardActions::Cut, parent, &GlaxnimateWindow::cut);
    edit_actions->addAction(KStandardActions::Copy, parent, &GlaxnimateWindow::copy);
    edit_actions->addAction(KStandardActions::Paste, parent, &GlaxnimateWindow::paste);

    // edit_paste_as_completion
    QAction* paste_as_composition = add_action(edit_actions, QStringLiteral("edit_paste_as_composition"), i18n("Paste as Composition"), QStringLiteral("special_paste"), {}, Qt::CTRL | Qt::SHIFT | Qt::Key_V);
    connect(paste_as_composition, &QAction::triggered, parent, [this]{parent->paste_as_composition();});

    QAction* duplicate = add_action(edit_actions, QStringLiteral("duplicate"), i18n("Duplicate"), QStringLiteral("edit-duplicate"), {}, Qt::CTRL | Qt::Key_D);
    connect(duplicate, &QAction::triggered, parent, &GlaxnimateWindow::duplicate_selection);

    QAction* select_all = edit_actions->addAction(KStandardActions::SelectAll);
    select_all->setEnabled(false);
    QAction* edit_delete = add_action(edit_actions, QStringLiteral("edit_delete"), i18n("Delete"), QStringLiteral("edit-delete"), {}, Qt::Key_Delete);
    connect(edit_delete, &QAction::triggered, [this](){
        if (active_tool->id() == QStringLiteral("edit")) {
            parent->actionCollection()->action(QStringLiteral("node_remove"))->trigger();
        } else if (active_tool->id() == QStringLiteral("select")) {
            parent->delete_selected();
        }
    });
    this->tool_actions["select"] = {
        edit_delete,
    };

    QAction* grid_enable = add_action(edit_actions, QStringLiteral("snap_grid_enable"), i18n("Grid"), QStringLiteral("grid-rectangular"), {}, Qt::Key_NumberSign);
    grid_enable->setCheckable(true);
    connect(grid_enable, &QAction::triggered, &grid, &SnappingGrid::enable);

}

tools::Tool* GlaxnimateWindow::Private::setup_tools_actions()
{
    KActionCategory* tool_actions_cat = new KActionCategory(i18n("Tools"), parent->actionCollection());

    // Tool Actions
    QActionGroup* tool_actions_group = new QActionGroup(parent);
    tool_actions_group->setExclusive(true);

    tools::Tool* to_activate = nullptr;
    for ( const auto& grp : tools::Registry::instance() )
    {
        for ( const auto& tool : grp.second )
        {
            QAction* action = tool.second->get_action();
            action->setActionGroup(tool_actions_group);
            parent->actionCollection()->setDefaultShortcut(action, tool.second->key_sequence());
            tool_actions_cat->addAction(tool.second->action_name(), action);

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
    KActionCategory* view_actions = new KActionCategory(i18n("View"), parent->actionCollection());

    QActionGroup* layout_actions = new QActionGroup(parent);
    layout_actions->setExclusive(true);

    QAction* layout_custom = add_action(view_actions, QStringLiteral("layout_custom"), i18n("Custom"), {}, QStringLiteral("Customized Layout"));
    layout_custom->setActionGroup(layout_actions);
    layout_custom->setCheckable(true);

    connect(layout_custom, &QAction::triggered, parent, [this]{layout_update();});

    QAction* layout_auto = add_action(view_actions, QStringLiteral("layout_automatic"), i18n("Automatic"), {}, QStringLiteral("Determines the best layout based on screen size"));
    layout_auto->setActionGroup(layout_actions);
    layout_auto->setCheckable(true);
    connect(layout_auto, &QAction::triggered, parent, [this]{layout_update();});

    QAction* layout_wide = add_action(view_actions, QStringLiteral("layout_wide"), i18n("Wide"), {}, QStringLiteral("Layout best suited for larger screens"));
    layout_wide->setActionGroup(layout_actions);
    layout_wide->setCheckable(true);
    connect(layout_wide, &QAction::triggered, parent, [this]{layout_update();});

    QAction* layout_medium = add_action(view_actions, QStringLiteral("layout_medium"), i18n("Medium"), {}, QStringLiteral("More compact than Wide but larger than Compact"));
    layout_medium->setActionGroup(layout_actions);
    layout_medium->setCheckable(true);
    connect(layout_medium, &QAction::triggered, parent, [this]{layout_update();});

    QAction* layout_compact = add_action(view_actions, QStringLiteral("layout_compact"), i18n("Compact"), {}, QStringLiteral("Layout best suited for smaller screens"));
    layout_compact->setActionGroup(layout_actions);
    layout_compact->setCheckable(true);
    connect(layout_compact, &QAction::triggered, parent, [this]{layout_update();});

    QAction* layout_mobile = add_action(view_actions, QStringLiteral("layout_mobile"), i18n("Mobile"), {}, QStringLiteral("Layout best suited for small portrait screens"));
    layout_mobile->setActionGroup(layout_actions);
    layout_mobile->setCheckable(true);
    connect(layout_mobile, &QAction::triggered, parent, [this]{layout_update();});


    view_actions->addAction(KStandardActions::ZoomIn, canvas, &Canvas::zoom_in);
    view_actions->addAction(KStandardActions::ZoomOut, canvas, &Canvas::zoom_out);
    view_actions->addAction(KStandardActions::FitToPage, canvas, &Canvas::view_fit);
    view_actions->addAction(KStandardActions::ActualSize, canvas, &Canvas::reset_zoom);

    QAction* tool_reset_rotation = add_action(view_actions, QStringLiteral("view_reset_rotation"), i18n("Reset Rotation"), QStringLiteral("rotation-allowed"));
    connect(tool_reset_rotation, &QAction::triggered, canvas, &Canvas::reset_rotation);
    QAction* tool_flip_view = add_action(view_actions, QStringLiteral("flip_view"), i18n("Flip View"), QStringLiteral("object-flip-horizontal"));
    connect(tool_flip_view, &QAction::triggered, canvas, &Canvas::flip_horizontal);

    QAction* focus_mode = add_action(view_actions, QStringLiteral("view_focus_mode"), i18n("Focus Mode"), QStringLiteral("window"));
    focus_mode->setCheckable(true);
    connect(focus_mode, &QAction::triggered, canvas, &Canvas::set_focus_mode);
}

void GlaxnimateWindow::Private::setup_document_actions()
{
    KActionCategory* document_actions = new KActionCategory(i18n("Document"), parent->actionCollection());

    QAction* render_frame = add_action(document_actions, QStringLiteral("render_frame"), i18n("Render Single Frame…"), QStringLiteral("image-x-generic"));
    connect(render_frame, &QAction::triggered, parent, [this]{save_frame();});

    QAction* preview_glax = add_action(document_actions, QStringLiteral("glaxnimate_preview"), i18n("Glaxnimate"));
    connect(preview_glax, &QAction::triggered, parent, [this]{preview_glaxnimate();});

    QAction* preview_lottie = add_action(document_actions, QStringLiteral("lottie_preview"), i18n("Lottie (SVG)"));
    connect(preview_lottie, &QAction::triggered, parent, [this]{this->preview_lottie("svg");});

    QAction* preview_lottie_canvas = add_action(document_actions, QStringLiteral("lottie_canvas_preview"), i18n("Lottie (canvas)"));
    connect(preview_lottie_canvas, &QAction::triggered, parent, [this]{this->preview_lottie("canvas");});

    QAction* preview_svg = add_action(document_actions, QStringLiteral("svg_preview"), i18n("SVG (SMIL)"));
    connect(preview_svg, &QAction::triggered, parent, [this]{this->preview_svg();});

    QAction* preview_rive = add_action(document_actions, QStringLiteral("rive_preview"), i18n("RIVE (canvas)"));
    connect(preview_rive, &QAction::triggered, parent, [this]{this->preview_rive();});

    QAction* validate_tgs = add_action(document_actions, QStringLiteral("validate_tgs"), i18n("Validate Telegram Sticker"), QStringLiteral("telegram"));
    connect(validate_tgs, &QAction::triggered, parent, &GlaxnimateWindow::validate_tgs);

    QAction* validate_discord = add_action(document_actions, QStringLiteral("validate_discord"), i18n("Validate Discord Sticker"), QStringLiteral("discord"));
    connect(validate_discord, &QAction::triggered, parent, [this]{this->validate_discord();});

    QAction* document_resize = add_action(document_actions, QStringLiteral("document_resize"), i18n("Resize…"), QStringLiteral("transform-scale"));
    connect(document_resize, &QAction::triggered, parent, [this]{ ResizeDialog(this->parent).resize_composition(comp); });

    QAction* document_cleanup = add_action(document_actions, QStringLiteral("document_cleanup"), i18n("Cleanup"), QStringLiteral("document-cleanup"), QStringLiteral("Remove unused assets"));
    connect(document_cleanup, &QAction::triggered, parent, [this]{cleanup_document();});

    QAction* document_timing = add_action(document_actions, QStringLiteral("document_timing"), i18n("Timing…"), QStringLiteral("player-time"));
    connect(document_timing, &QAction::triggered, parent, [this]{
        TimingDialog(comp, this->parent).exec();
    });

    QAction* document_metadata = add_action(document_actions, QStringLiteral("document_metadata"), i18n("Metadata…"), QStringLiteral("documentinfo"));
    connect(document_metadata, &QAction::triggered, parent, [this]{
        DocumentMetadataDialog(current_document.get(), this->parent).exec();
    });
}

void GlaxnimateWindow::Private::setup_playback_actions()
{
    KActionCategory* playback_actions = new KActionCategory(i18n("Playback"), parent->actionCollection());


    add_action(playback_actions, QStringLiteral("play"), i18n("Play"), QStringLiteral("media-playback-start"), {}, Qt::Key_Space);

    add_action(playback_actions, QStringLiteral("play_loop"), i18n("Loop"), QStringLiteral("media-playlist-repeat"));

    add_action(playback_actions, QStringLiteral("record"), i18n("Record Keyframes"), QStringLiteral("media-record"));

    add_action(playback_actions, QStringLiteral("frame_first"), i18n("Jump to Start"), QStringLiteral("go-first"));

    add_action(playback_actions, QStringLiteral("frame_last"), i18n("Jump to End"), QStringLiteral("go-last"));

    add_action(playback_actions, QStringLiteral("frame_next"), i18n("Next Frame"), QStringLiteral("go-next"), {}, Qt::Key_Right);

    add_action(playback_actions, QStringLiteral("frame_prev"), i18n("Previous Frame"), QStringLiteral("go-previous"), {}, Qt::Key_Left);
}

void GlaxnimateWindow::Private::connect_playback_actions()
{
    QAction* play = parent->actionCollection()->action(QStringLiteral("play"));
    connect(play, &QAction::triggered, timeline_dock->playControls(), &FrameControlsWidget::toggle_play);
    connect(timeline_dock->playControls(), &FrameControlsWidget::play_started, parent, [play]{
        play->setText(i18n("Pause"));
        play->setIcon(QIcon::fromTheme("media-playback-pause"));
    });
    connect(timeline_dock->playControls(), &FrameControlsWidget::play_stopped, parent, [play]{
        play->setText(i18n("Play"));
        play->setIcon(QIcon::fromTheme("media-playback-start"));
    });
    QAction* record = parent->actionCollection()->action(QStringLiteral("record"));
    connect(timeline_dock->playControls(), &FrameControlsWidget::record_toggled, record, &QAction::setChecked);

    QAction* playLoop = parent->actionCollection()->action(QStringLiteral("play_loop"));
    connect(timeline_dock->playControls(), &FrameControlsWidget::loop_changed, playLoop, &QAction::setChecked);
    connect(playLoop, &QAction::triggered, timeline_dock->playControls(), &FrameControlsWidget::set_loop);

    connect(record, &QAction::triggered, timeline_dock->playControls(), &FrameControlsWidget::set_record_enabled);

    QAction* frameFirst = parent->actionCollection()->action(QStringLiteral("frame_first"));
    connect(frameFirst, &QAction::triggered, timeline_dock->playControls(), &FrameControlsWidget::go_first);

    QAction* frameLast = parent->actionCollection()->action(QStringLiteral("frame_last"));
    connect(frameLast, &QAction::triggered, timeline_dock->playControls(), &FrameControlsWidget::go_last);

    QAction* frameNext = parent->actionCollection()->action(QStringLiteral("frame_next"));
    connect(frameNext, &QAction::triggered, timeline_dock->playControls(), &FrameControlsWidget::go_next);

    QAction* framePrev = parent->actionCollection()->action(QStringLiteral("frame_prev"));
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
    KActionCategory* layers_actions = new KActionCategory(i18n("Layers"), parent->actionCollection());

    add_modifier_menu_action<model::Repeater>(layers_actions);
    add_modifier_menu_action<model::Trim>(layers_actions);
    add_modifier_menu_action<model::InflateDeflate>(layers_actions);
    add_modifier_menu_action<model::RoundCorners>(layers_actions);
    add_modifier_menu_action<model::OffsetPath>(layers_actions);
    add_modifier_menu_action<model::ZigZag>(layers_actions);

    QAction* new_precomp_selection = add_action(layers_actions, QStringLiteral("new_precomp_selection"), i18n("Precompose Selection"), QStringLiteral("archive-extract"));
    connect(new_precomp_selection, &QAction::triggered, parent, [this]{
        parent->precompose(comp, cleaned_selection(), &comp->shapes, -1);
    });
    QAction* new_comp = add_action(layers_actions, QStringLiteral("new_comp"), i18n("New Composition"), QStringLiteral("folder-video"));
    connect(new_comp, &QAction::triggered, parent, [this]{add_composition();});

    QAction* new_layer = add_action(layers_actions, QStringLiteral("new_layer"), i18n("Layer"), QStringLiteral("folder"));
    connect(new_layer, &QAction::triggered, parent, [this]{layer_new_layer();});

    QAction* new_group = add_action(layers_actions, QStringLiteral("new_layer_group"), i18n("Group"), QStringLiteral("shapes-symbolic"));
    connect(new_group, &QAction::triggered, parent, [this]{layer_new_group();});

    QAction* new_fill = add_action(layers_actions, QStringLiteral("new_fill"), i18n("Fill"), QStringLiteral("format-fill-color"));
    connect(new_fill, &QAction::triggered, parent, [this]{layer_new_fill();});

    QAction* new_stroke = add_action(layers_actions, QStringLiteral("new_stroke"), i18n("Stroke"), QStringLiteral("object-stroke-style"));
    connect(new_stroke, &QAction::triggered, parent, [this]{layer_new_stroke();});

    QAction* insert_emoji = add_action(layers_actions, QStringLiteral("insert_emoji"), i18n("Emoji…"), QStringLiteral("smiley-shape"));
    connect(insert_emoji, &QAction::triggered, parent, [this]{this->insert_emoji();});
#ifdef Q_OS_WIN32
    // Can't get emoji_data.cpp to compile on windows for qt6 for some reason
    // the compiler errors out without message
    insert_emoji->setEnabled(false);
#endif
}

void GlaxnimateWindow::Private::setup_object_actions()
{
    KActionCategory* object_actions = new KActionCategory(i18n("Object"), parent->actionCollection());

    QAction* raise_to_top = add_action(object_actions, QStringLiteral("object_raise_to_top"), i18n("Raise to Top"), QStringLiteral("layer-top"), {}, Qt::Key_Home);
    connect(raise_to_top, &QAction::triggered, parent, &GlaxnimateWindow::layer_top);

    QAction* raise = add_action(object_actions, QStringLiteral("object_raise"), i18n("Raise"), QStringLiteral("layer-raise"), {}, Qt::Key_PageUp);
    connect(raise, &QAction::triggered, parent, &GlaxnimateWindow::layer_raise);

    QAction* lower = add_action(object_actions, QStringLiteral("object_lower"), i18n("Lower"), QStringLiteral("layer-lower"), {}, Qt::Key_PageDown);
    connect(lower, &QAction::triggered, parent, &GlaxnimateWindow::layer_lower);

    QAction* lower_to_bottom = add_action(object_actions, QStringLiteral("object_lower_to_bottom"), i18n("Lower to Bottom"), QStringLiteral("layer-bottom"), {}, Qt::Key_End);
    connect(lower_to_bottom, &QAction::triggered, parent, &GlaxnimateWindow::layer_bottom);

    QAction* move_to = add_action(object_actions, QStringLiteral("object_move_to"), i18n("Move to…"), QStringLiteral("selection-move-to-layer-above"));
    connect(move_to, &QAction::triggered, parent, &GlaxnimateWindow::move_to);

    QAction* group = add_action(object_actions, QStringLiteral("object_group"), i18n("Group"), QStringLiteral("object-group"), {}, Qt::CTRL | Qt::Key_G);
    connect(group, &QAction::triggered, parent, &GlaxnimateWindow::group_shapes);

    QAction* ungroup = add_action(object_actions, QStringLiteral("object_ungroup"), i18n("Ungroup"), QStringLiteral("object-ungroup"), {}, Qt::CTRL | Qt::SHIFT | Qt::Key_G);
    connect(ungroup, &QAction::triggered, parent, &GlaxnimateWindow::ungroup_shapes);

    KActionCategory* algin_actions = new KActionCategory(i18n("Align"), parent->actionCollection());

    QAction* align_to_selection = add_action(algin_actions, QStringLiteral("align_to_selection"), i18n("Selection"), QStringLiteral("select-rectangular"));
    align_to_selection->setCheckable(true);
    QAction* align_to_canvas = add_action(algin_actions, QStringLiteral("align_to_canvas"), i18n("Canvas"), QStringLiteral("snap-page"));
    align_to_canvas->setCheckable(true);
    QAction* align_to_canvas_group = add_action(algin_actions, QStringLiteral("align_to_canvas_group"), i18n("Canvas (as Group)"), QStringLiteral("object-group"));
    align_to_canvas_group->setCheckable(true);

    QAction* align_hor_left = add_action(algin_actions, QStringLiteral("align_hor_left"), i18n("Left"), QStringLiteral("align-horizontal-left"));
    connect(align_hor_left, &QAction::triggered, parent, [this]{align(AlignDirection::Horizontal, AlignPosition::Begin, false);});

    QAction* align_hor_center = add_action(algin_actions, QStringLiteral("align_hor_center"), i18n("Center"), QStringLiteral("align-horizontal-center"));
    connect(align_hor_center, &QAction::triggered, parent, [this]{align(AlignDirection::Horizontal, AlignPosition::Center, false);});

    QAction* align_hor_right = add_action(algin_actions, QStringLiteral("align_hor_right"), i18n("Right"), QStringLiteral("align-horizontal-right"));
    connect(align_hor_right, &QAction::triggered, parent, [this]{align(AlignDirection::Horizontal, AlignPosition::End, false);});

    QAction* align_vert_top = add_action(algin_actions, QStringLiteral("align_vert_top"), i18n("Top"), QStringLiteral("align-vertical-top"));
    connect(align_vert_top, &QAction::triggered, parent, [this]{align(AlignDirection::Vertical, AlignPosition::Begin, false);});

    QAction* align_vert_center = add_action(algin_actions, QStringLiteral("align_vert_center"), i18n("Center"), QStringLiteral("align-vertical-center"));
    connect(align_vert_center, &QAction::triggered, parent, [this]{align(AlignDirection::Vertical, AlignPosition::Center, false);});

    QAction* align_vert_bottom = add_action(algin_actions, QStringLiteral("align_vert_bottom"), i18n("Bottom"), QStringLiteral("align-vertical-bottom"));
    connect(align_vert_bottom, &QAction::triggered, parent, [this]{align(AlignDirection::Vertical, AlignPosition::End, false);});


    // addAction(QStringLiteral("separator_align_relative_to"), i18n("Relative To"), QStringLiteral(""));
    // addAction(QStringLiteral("separator_align_horizontal"), i18n("Horizontal"), QStringLiteral(""));
    // addAction(QStringLiteral("separator_align_vertical"), i18n("Vertical"), QStringLiteral(""));
    QAction* alignhor_left_out = add_action(algin_actions, QStringLiteral("align_hor_left_out"), i18n("Outside Left"), QStringLiteral("align-horizontal-right-out"));
    connect(alignhor_left_out, &QAction::triggered, parent, [this]{align(AlignDirection::Horizontal, AlignPosition::Begin, true);});

    QAction* align_hor_right_out = add_action(algin_actions, QStringLiteral("align_hor_right_out"), i18n("Outside Right"), QStringLiteral("align-horizontal-left-out"));
    connect(align_hor_right_out, &QAction::triggered, parent, [this]{align(AlignDirection::Horizontal, AlignPosition::End, true);});

    QAction* align_vert_top_out = add_action(algin_actions, QStringLiteral("align_vert_top_out"), i18n("Outside Top"), QStringLiteral("align-vertical-bottom-out"));
    connect(align_vert_top_out, &QAction::triggered, parent, [this]{align(AlignDirection::Vertical, AlignPosition::Begin, true);});

    QAction* align_vert_bottom_out = add_action(algin_actions, QStringLiteral("align_vert_bottom_out"), i18n("Outside Bottom"), QStringLiteral("align-vertical-top-out"));
    connect(align_vert_bottom_out, &QAction::triggered, parent, [this]{align(AlignDirection::Vertical, AlignPosition::End, true);});

    QAction* action_object_add_keyframe = add_action(object_actions, QStringLiteral("object_add_keyframe"), i18n("Add Keyframe"), QStringLiteral("keyframe-add"), {}, Qt::Key_K);
    connect(action_object_add_keyframe, &QAction::triggered, parent, [this]{action_add_keyframe();});

    for ( int i = 0; i < model::KeyframeTransition::max_descriptive; i++ )
    {
        auto desc = model::KeyframeTransition::Descriptive(i);
        if ( desc != model::KeyframeTransition::NoValue )
        {
            auto data = KeyframeTransitionData::data(desc);
            QAction* action = add_action(object_actions, QStringLiteral("kftran_") + data.icon_slug, data.name);
            action->setIcon(data.icon());
            connect(action, &QAction::triggered, parent, [this, desc]{action_keyframe_transition(desc);});
            if ( desc == model::KeyframeTransition::Custom )
            {
                action->setText(i18n("Custom..."));
            }
        }
    }
}

void GlaxnimateWindow::Private::action_add_keyframe()
{
    model::Object* object = current_node;
    model::AnimatableBase* prop = nullptr;

    auto timeline = timeline_dock->timelineWidget();
    QPoint mouse = QCursor::pos();
    auto item = timeline->item_at(mouse);

    if ( !item.property && !item.object )
        item = properties_dock->item_at(mouse);

    if ( item.property )
    {
        object = item.property->object();
        prop = item.animatable;
    }
    else if ( item.object )
    {
        object = item.object;
    }

    if ( prop )
    {
        action_add_keyframe_for(object, prop);
    }
    else
    {
        auto props = model::all_animated_properties(object);
        if ( props.empty() )
            return;
        QMenu menu;
        menu.addSection(QIcon::fromTheme("keyframe-add"), i18n("Add Keyframe"));
        for ( auto sub_prop : props )
        {
            auto action = menu.addAction(sub_prop->localized_name());
            connect(action, &QAction::triggered, parent, [this, sub_prop]{action_add_keyframe_for(sub_prop->object(), sub_prop);});
        }
        menu.exec(mouse);
    }
}

void GlaxnimateWindow::Private::action_add_keyframe_for(model::Object* object, model::AnimatableBase* prop)
{
    current_document->push_command(
        prop->command_add_smooth_keyframe(object->time(), prop->static_value())
    );
}

void GlaxnimateWindow::Private::action_keyframe_transition(model::KeyframeTransition::Descriptive desc)
{
    auto selected = timeline_dock->timelineWidget()->timeline()->selected_keyframes();
    if ( selected.empty() )
        return;

    auto cmd = new QUndoCommand(i18n("Apply %1 keyframe transition", KeyframeTransitionData::data(desc).name));

    if ( desc == model::KeyframeTransition::Custom )
    {
        // Custom transition with dialog
        KeyframeEditorDialog dialog;
        for ( auto kfinfo : selected )
        {
            auto kf = kfinfo.property->keyframe_at(kfinfo.time);
            if ( kf && kf->transition().special() == model::KeyframeTransition::Special::Normal )
            {
                // Set the dialog to use the first normal transition
                dialog.set_transition(kf->transition());
                break;
            }
        }

        if ( dialog.exec() )
        {
            for ( auto kfinfo : selected )
                kfinfo.property->command_set_transition(kfinfo.time, dialog.transition(), cmd);
        }
    }
    else
    {
        // Preset type
        for ( auto kfinfo : selected )
        {
            auto kf = kfinfo.property->keyframe_at(kfinfo.time);
            if ( !kf )
                continue;

            if ( auto kf_before = kfinfo.property->keyframe_before(kfinfo.time) )
            {
                auto left_trans = kf_before->transition();
                left_trans.set_after_descriptive(desc);
                kfinfo.property->command_set_transition(kf_before->time(), left_trans, cmd);
            }

            auto right_trans = kf->transition();
            right_trans.set_before_descriptive(desc);
            kfinfo.property->command_set_transition(kfinfo.time, right_trans, cmd);
        }
    }

    // Push if we did anything
    if ( cmd->childCount() > 0 )
        current_document->push_command(cmd);
}


void GlaxnimateWindow::Private::setup_path_actions()
{
    KActionCategory* path_actions = new KActionCategory(i18n("Path"), parent->actionCollection());

    QAction* object_to_path = add_action(path_actions, QStringLiteral("object_to_path"), i18n("Object to Path"), QStringLiteral("object-to-path"));
    connect(object_to_path, &QAction::triggered, parent, [this]{to_path();});

    QAction* trace_bitmap = add_action(path_actions, QStringLiteral("trace_bitmap"), i18n("Trace Bitmap…"), QStringLiteral("bitmap-trace"));
    connect(trace_bitmap, &QAction::triggered, parent, [this]{
        trace_dialog(parent->current_shape());
    });

    QAction* path_add = add_action(path_actions, QStringLiteral("path_add"), i18n("Union"), QStringLiteral("path-union"));
    path_add->setEnabled(false);
    path_add->setVisible(false);
    QAction* path_substract = add_action(path_actions, QStringLiteral("path_subtract"), i18n("Difference"), QStringLiteral("path-difference"));
    path_substract->setEnabled(false);
    path_substract->setVisible(false);
    QAction* path_intersect = add_action(path_actions, QStringLiteral("path_intersect"), i18n("Intersect"), QStringLiteral("path-intersection"));
    path_intersect->setEnabled(false);
    path_intersect->setVisible(false);
    QAction* path_xor = add_action(path_actions, QStringLiteral("path_xor"), i18n("Exclusion"), QStringLiteral("path-exclusion"));
    path_xor->setEnabled(false);
    path_xor->setVisible(false);

    QAction* path_reverse = add_action(path_actions, QStringLiteral("path_reverse"), i18n("Reverse"), QStringLiteral("path-reverse"));
    path_reverse->setEnabled(true);

    QAction* node_remove = add_action(path_actions, QStringLiteral("node_remove"), i18n("Delete Nodes"), QStringLiteral("format-remove-node"));
    QAction* node_add = add_action(path_actions, QStringLiteral("node_add"), i18n("Add Node…"), QStringLiteral("format-insert-node"));

    QAction* node_join = add_action(path_actions, QStringLiteral("node_join"), i18n("Join Nodes"), QStringLiteral("format-join-node"));
    node_join->setEnabled(false);
    QAction* node_split = add_action(path_actions, QStringLiteral("node_split"), i18n("Split Nodes"), QStringLiteral("format-break-node"));
    node_split->setEnabled(false);

    QAction* node_type_corner = add_action(path_actions, QStringLiteral("node_type_corner"), i18n("Cusp"), QStringLiteral("node-type-cusp"));
    QAction* node_type_smooth = add_action(path_actions, QStringLiteral("node_type_smooth"), i18n("Smooth"), QStringLiteral("node-type-smooth"));
    QAction* node_type_symmetric = add_action(path_actions, QStringLiteral("node_type_symmetric"), i18n("Symmetric"), QStringLiteral("node-type-auto-smooth"));
    QAction* segment_lines = add_action(path_actions, QStringLiteral("segment_lines"), i18n("Make segments straight"), QStringLiteral("node-segment-line"));
    QAction* segment_curve = add_action(path_actions, QStringLiteral("segment_curve"), i18n("Make segments curved"), QStringLiteral("node-segment-curve"));
    QAction* node_dissolve = add_action(path_actions, QStringLiteral("node_dissolve"), i18n("Dissolve Nodes"), QStringLiteral("format-node-curve"));

    this->tool_actions["edit"] = {
        parent->actionCollection()->action(QStringLiteral("edit_delete")),
        node_remove,
        node_type_corner,
        node_type_smooth,
        node_type_symmetric,
        segment_lines,
        segment_curve,
        node_add,
        node_dissolve,
    };

    tools::EditTool* edit_tool = static_cast<tools::EditTool*>(tools::Registry::instance().tool("edit"));
    connect(node_type_corner, &QAction::triggered, parent, [edit_tool]{
        edit_tool->selection_set_vertex_type(math::bezier::Corner);
    });
    connect(node_type_smooth, &QAction::triggered, parent, [edit_tool]{
        edit_tool->selection_set_vertex_type(math::bezier::Smooth);
    });
    connect(node_type_symmetric, &QAction::triggered, parent, [edit_tool]{
        edit_tool->selection_set_vertex_type(math::bezier::Symmetrical);
    });
    connect(node_remove, &QAction::triggered, parent, [edit_tool]{
        edit_tool->selection_delete();
    });
    connect(segment_lines, &QAction::triggered, parent, [edit_tool]{
        edit_tool->selection_straighten();
    });
    connect(segment_curve, &QAction::triggered, parent, [edit_tool]{
        edit_tool->selection_curve();
    });
    connect(node_add, &QAction::triggered, parent, [edit_tool]{
        edit_tool->add_point_mode();
    });
    connect(node_dissolve, &QAction::triggered, parent, [edit_tool]{
        edit_tool->selection_dissolve();
    });
    connect(path_reverse, &QAction::triggered, parent, [this]{

        model::DocumentNode* curr = current_document_node();
        if ( !curr )
            return;

        command::recursive_reverse_path(curr);
    });
}

void GlaxnimateWindow::Private::init_tools_ui()
{
    tools_dock = new QDockWidget(i18n("Tools"), this->parent);
    tools_dock->setWindowIcon(QIcon::fromTheme(QStringLiteral("tools")));
    tools_dock->setObjectName(QStringLiteral("dock_tools"));
    parent->addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, tools_dock);

    QWidget* widget = new QWidget(tools_dock);
    dock_tools_layout = new FlowLayout();
    widget->setLayout(dock_tools_layout);
    tools_dock->setWidget(widget);

    for ( const auto& grp : tools::Registry::instance() )
    {
        for ( const auto& tool : grp.second )
        {
            QAction* action = parent->actionCollection()->action(tool.second->action_name());

            ScalableButton* button = tool.second->get_button();

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

    KToolBar* toolbar_node = parent->toolBar(QStringLiteral("nodeToolBar"));
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
    connect(timeline_dock->timelineWidget()->timeline(), &TimelineWidget::has_keyframe_selected_changed, parent, [this](bool has_kf){
        parent->findChild<QMenu*>("menu_keyframe_type")->setEnabled(has_kf);
    });
}

static QWidget* status_bar_separator()
{
    QFrame* line = new QFrame();
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

    QAction* flipView = parent->actionCollection()->action(QStringLiteral("flip_view"));
    connect(view_trans_widget, &ViewTransformWidget::flip_view, flipView, &QAction::trigger);
}

void GlaxnimateWindow::Private::init_docks()
{
    scriptconsole_dock = new ScriptConsoleDock(this->parent);
    // Scripting
    connect(scriptconsole_dock, &ScriptConsoleDock::error, parent, [this](const QString& plugin, const QString& message){
        show_warning(plugin, message, log::Error);
    });
    scriptconsole_dock->set_global("window", QVariant::fromValue(parent));

    parent->addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, scriptconsole_dock);

    init_plugins();

    // Logs
    log_model.populate(GlaxnimateApp::instance()->log_lines());

    logs_dock = new LogsDock(this->parent, &log_model);
    parent->addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, logs_dock);

    // Swatches
    palette_model.setSearchPaths(utils::data_paths_unchecked("palettes"));
    palette_model.setSavePath(utils::writable_data_path("palettes"));
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

    // Grid
    grid_dock = new DockGrid(this->parent);
    grid.set_angle(math::deg2rad(GlaxnimateSettings::self()->grid_angle()));
    grid.set_origin(QPointF(
        GlaxnimateSettings::self()->grid_origin_x(),
        GlaxnimateSettings::self()->grid_origin_y()
    ));
    grid.set_shape(SnappingGrid::GridShape(GlaxnimateSettings::self()->grid_shape()));
    grid.enable(GlaxnimateSettings::self()->grid_enabled());
    grid.set_size(GlaxnimateSettings::self()->grid_size());
    parent->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, grid_dock);
    grid_dock->set_grid(&grid);
    canvas->set_grid(&grid);
    render_widget.set_grid(&grid);
    connect(&grid, &SnappingGrid::grid_changed, render_widget.widget(), qOverload<>(&QWidget::update));


}

void GlaxnimateWindow::Private::layout_update()
{
    QAction* layoutWide = parent->actionCollection()->action(QStringLiteral("layout_wide"));
    QAction* layoutCompact = parent->actionCollection()->action(QStringLiteral("layout_compact"));
    QAction* layoutCustom = parent->actionCollection()->action(QStringLiteral("layout_custom"));
    QAction* layoutMedium = parent->actionCollection()->action(QStringLiteral("layout_medium"));
    QAction* action_mobile = parent->actionCollection()->action(QStringLiteral("layout_mobile"));

    if ( layoutWide->isChecked() )
        layout_wide();
    else if ( layoutCompact->isChecked() )
        layout_compact();
    else if ( layoutCustom->isChecked() )
        layout_custom();
    else if ( layoutMedium->isChecked() )
        layout_medium();
    else if ( action_mobile->isChecked() )
        layout_mobile();
    else
        layout_auto();
}

static void set_dock_area(QMainWindow* window, Qt::DockWidgetArea area, std::vector<QDockWidget*> docks, std::set<QDockWidget*> hidden)
{
    QDockWidget* prev = nullptr;
    for ( std::size_t i = 0; i < docks.size(); i++ )
    {
        if ( !prev )
            window->addDockWidget(area, docks[i]);
        else
            window->tabifyDockWidget(prev, docks[i]);

        docks[i]->setFloating(false);
        prev = docks[i];
    }
    docks[0]->raise();

    for ( std::size_t i = 0; i < docks.size(); i++ )
        docks[i]->setVisible(hidden.count(docks[i]) == 0);
}

void GlaxnimateWindow::Private::layout_medium()
{
    parent->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    // Bottom
    set_dock_area(
        parent,
        Qt::BottomDockWidgetArea,
        {
            timeline_dock,
            scriptconsole_dock,
            logs_dock,
            time_slider_dock
        },
        {
            scriptconsole_dock,
            logs_dock,
            time_slider_dock
        }
    );


    // Right
    set_dock_area(
        parent,
        Qt::RightDockWidgetArea,
        {
            properties_dock,
            undo_dock,
            assets_dock,
            grid_dock,
            snippets_dock
        },
        {
            assets_dock,
            grid_dock,
            snippets_dock
        }
    );

    // Bottom Right
    set_dock_area(
        parent,
        Qt::RightDockWidgetArea,
        {
            colors_dock,
            stroke_dock,
            layers_dock,
            gradients_dock,
            swatches_dock
        },
        {
            swatches_dock,
        }
    );

    // Left
    parent->addDockWidget(Qt::LeftDockWidgetArea, tools_dock);
    tools_dock->setFloating(false);
    tools_dock->setVisible(false);

    parent->addDockWidget(Qt::LeftDockWidgetArea, tool_options_dock);
    parent->tabifyDockWidget(tool_options_dock, align_dock);
    tool_options_dock->raise();
    tool_options_dock->setFloating(false);
    tool_options_dock->setVisible(true);
    align_dock->setVisible(true);
    align_dock->setFloating(false);

    // Toolbars
    KToolBar* toolbar_tools = parent->toolBar(QStringLiteral("toolsToolBar"));
    parent->addToolBar(Qt::LeftToolBarArea, toolbar_tools);
    toolbar_tools->setVisible(true);

    // Resize

    parent->resizeDocks({properties_dock}, {350}, Qt::Horizontal);
    parent->resizeDocks({properties_dock, colors_dock}, {4000, 1}, Qt::Vertical);
    parent->resizeDocks({tools_dock}, {200}, Qt::Horizontal);
    parent->resizeDocks({tool_options_dock, align_dock, tools_dock}, {1, 1, 4000}, Qt::Vertical);
    parent->resizeDocks({timeline_dock}, {300}, Qt::Vertical);
    scriptconsole_dock->setVisible(false);

    // Resize parent to have a reasonable default size
    parent->resize(1440, 900);

    GlaxnimateSettings::setLayout(int(LayoutPreset::Medium));
    QAction* layoutMedium = parent->actionCollection()->action(QStringLiteral("layout_medium"));
    layoutMedium->setChecked(true);
}

void GlaxnimateWindow::Private::layout_wide()
{
    parent->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    // Bottom
    set_dock_area(
        parent,
        Qt::BottomDockWidgetArea,
        {
            timeline_dock,
            scriptconsole_dock,
            logs_dock,
            time_slider_dock
        },
        {
            scriptconsole_dock,
            logs_dock,
            time_slider_dock
        }
    );

    // Right
    set_dock_area(
        parent,
        Qt::RightDockWidgetArea,
        {
            properties_dock,
            undo_dock,
            assets_dock,
            grid_dock,
            snippets_dock
        },
        {
            assets_dock,
            grid_dock,
            snippets_dock
        }
    );

    // Bottom Right
    set_dock_area(
        parent,
        Qt::RightDockWidgetArea,
        {
            colors_dock,
            stroke_dock,
            layers_dock,
            gradients_dock,
            swatches_dock
        },
        {
            swatches_dock,
        }
    );

    // Left
    parent->addDockWidget(Qt::LeftDockWidgetArea, tools_dock);
    tools_dock->setFloating(false);
    tools_dock->setVisible(false);

    parent->addDockWidget(Qt::LeftDockWidgetArea, tool_options_dock);
    tool_options_dock->setFloating(false);
    tool_options_dock->setVisible(true);

    parent->addDockWidget(Qt::LeftDockWidgetArea, align_dock);
    align_dock->setFloating(false);
    align_dock->setVisible(true);
    // Toolbars
    KToolBar* toolbar_tools = parent->toolBar(QStringLiteral("toolsToolBar"));
    parent->addToolBar(Qt::LeftToolBarArea, toolbar_tools);
    toolbar_tools->setVisible(true);

    // Resize
    parent->resizeDocks({properties_dock}, {400}, Qt::Horizontal);
    parent->resizeDocks({properties_dock, colors_dock}, {4000, 420}, Qt::Vertical);
    parent->resizeDocks({tool_options_dock}, {200}, Qt::Horizontal);
    parent->resizeDocks({tool_options_dock, align_dock, tools_dock}, {1, 1, 4000}, Qt::Vertical);
    parent->resizeDocks({timeline_dock}, {1080/3}, Qt::Vertical);

    // Resize parent to have a reasonable default size
    parent->resize(1920, 1080);

    GlaxnimateSettings::setLayout(int(LayoutPreset::Wide));
    QAction* layoutWide = parent->actionCollection()->action(QStringLiteral("layout_wide"));
    layoutWide->setChecked(true);
}

void GlaxnimateWindow::Private::layout_compact()
{
    parent->setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);

    // Bottom
    set_dock_area(
        parent,
        Qt::BottomDockWidgetArea,
        {
            time_slider_dock,
            timeline_dock,
            scriptconsole_dock,
            logs_dock,
        },
        {
            scriptconsole_dock,
            logs_dock,
            timeline_dock
        }
    );


    // Right
    set_dock_area(
        parent,
        Qt::RightDockWidgetArea,
        {
            properties_dock,
            colors_dock,
            stroke_dock,
            layers_dock,
            undo_dock,
            gradients_dock,
            swatches_dock,
            assets_dock,
            grid_dock,
            snippets_dock,
        },
        {
            undo_dock,
            gradients_dock,
            swatches_dock,
            assets_dock,
            grid_dock,
            snippets_dock,
        }
    );

    // Left
    parent->addDockWidget(Qt::LeftDockWidgetArea, tools_dock);
    tools_dock->setFloating(false);
    tools_dock->setVisible(false);

    parent->addDockWidget(Qt::LeftDockWidgetArea, tool_options_dock);
    parent->tabifyDockWidget(tool_options_dock, align_dock);
    tool_options_dock->raise();
    tool_options_dock->setVisible(true);
    tool_options_dock->setFloating(false);
    align_dock->setVisible(true);
    align_dock->setFloating(false);

    // Toolbars
    KToolBar* toolbar_tools = parent->toolBar(QStringLiteral("toolsToolBar"));
    parent->addToolBar(Qt::LeftToolBarArea, toolbar_tools);
    toolbar_tools->setVisible(true);

    // Resize
    parent->resizeDocks({tool_options_dock, align_dock, tools_dock}, {1, 1, 4000}, Qt::Vertical);
    parent->resizeDocks({time_slider_dock}, {64}, Qt::Vertical);

    // Resize parent to have a reasonable default size
    parent->resize(1366, 768);

    GlaxnimateSettings::setLayout(int(LayoutPreset::Compact));
    QAction* layoutCompact = parent->actionCollection()->action(QStringLiteral("layout_compact"));
    layoutCompact->setChecked(true);
}


void GlaxnimateWindow::Private::layout_mobile()
{
    parent->setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);

    // Bottom
    set_dock_area(
        parent,
        Qt::BottomDockWidgetArea,
        {
            time_slider_dock,
            timeline_dock,
            scriptconsole_dock,
            logs_dock,
        },
        {
            scriptconsole_dock,
            logs_dock,
            timeline_dock
        }
    );


    // Right
    set_dock_area(
        parent,
        Qt::RightDockWidgetArea,
        {
            properties_dock,
            colors_dock,
            stroke_dock,
            layers_dock,
            undo_dock,
            align_dock,
            tool_options_dock,
            gradients_dock,
            swatches_dock,
            assets_dock,
            grid_dock,
            snippets_dock,
        },
        {
            undo_dock,
            gradients_dock,
            swatches_dock,
            assets_dock,
            grid_dock,
            snippets_dock,
            tool_options_dock,
        }
    );

    // Toolbars
    KToolBar* toolbar_tools = parent->toolBar(QStringLiteral("toolsToolBar"));
    parent->addToolBar(Qt::BottomToolBarArea, toolbar_tools);
    toolbar_tools->setVisible(true);

    // Resize
    // parent->resizeDocks({tool_options_dock, align_dock, tools_dock}, {1, 1, 4000}, Qt::Vertical);
    parent->resizeDocks({properties_dock}, {1}, Qt::Horizontal);
    parent->resizeDocks({time_slider_dock}, {64}, Qt::Vertical);

    // Resize parent to have a reasonable default size
    parent->resize(400, 900);

    GlaxnimateSettings::setLayout(int(LayoutPreset::Compact));
    QAction* layoutCompact = parent->actionCollection()->action(QStringLiteral("layout_compact"));
    layoutCompact->setChecked(true);
}

void GlaxnimateWindow::Private::layout_auto()
{
    auto real_estate = qApp->primaryScreen()->availableSize();
    if ( real_estate.width() >= 1920 && real_estate.height() >= 1080 )
        layout_wide();
    else if ( real_estate.width() >= 1440 && real_estate.height() >= 900 )
        layout_medium();
    else if ( real_estate.width() > real_estate.height() )
        layout_compact();
    else
        layout_mobile();

    GlaxnimateSettings::setLayout(int(LayoutPreset::Auto));
    QAction* layoutAuto = parent->actionCollection()->action(QStringLiteral("layout_automatic"));
    layoutAuto->setChecked(true);
}

void GlaxnimateWindow::Private::layout_custom()
{
    init_restore_state();
    GlaxnimateSettings::setLayout(int(LayoutPreset::Custom));

    QAction* layoutCustom = parent->actionCollection()->action(QStringLiteral("layout_custom"));
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
        wid->setWindowTitle(KLocalizedString::removeAcceleratorMarker(wid->windowTitle()));
    }
    parent->unplugActionList( "view_actionlist" );
    parent->plugActionList( "view_actionlist", view_actions );

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
    parent->restoreState(QByteArray::fromBase64(GlaxnimateSettings::window_state().toUtf8()));
    timeline_dock->timelineWidget()->load_state(QByteArray::fromBase64(GlaxnimateSettings::timeline_splitter().toUtf8()));
    // parent->restoreGeometry(GlaxnimateSettings::window_geometry());

    // Hide tool widgets, as they might get shown by restoreState
    KToolBar* toolbar_node = parent->toolBar(QStringLiteral("nodeToolBar"));
    toolbar_node->setVisible(false);
    toolbar_node->setEnabled(false);
}

void GlaxnimateWindow::Private::view_fit()
{
    canvas->view_fit();
}

void GlaxnimateWindow::Private::show_warning(const QString& title, const QString& message, log::Severity icon)
{
    KMessageWidget::MessageType message_type;
    switch ( icon )
    {
        case log::Severity::Info: message_type = KMessageWidget::Information; break;
        case log::Severity::Warning: message_type = KMessageWidget::Warning; break;
        case log::Severity::Error: message_type = KMessageWidget::Error; break;
    }
    message_widget->queue_message({message, message_type});
    log::Log(title).log(message, icon);
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

    QClipboard* clipboard = qApp->clipboard();
    clipboard->setText(debuginfo);
}

void GlaxnimateWindow::Private::shutdown()
{
    KSharedConfigPtr config = KSharedConfig::openConfig();
    m_recentFilesAction->saveEntries(KConfigGroup(config, QString()));

    // GlaxnimateSettings::setWindow_geometry(parent->saveGeometry());
    GlaxnimateSettings::setWindow_state(QString::fromLatin1(parent->saveState().toBase64()));
    GlaxnimateSettings::setTimeline_splitter(QString::fromLatin1(timeline_dock->timelineWidget()->save_state().toBase64()));

    colors_dock->save_settings();

    stroke_dock->save_settings();

    scriptconsole_dock->save_settings();
    scriptconsole_dock->clear_contexts();

    GlaxnimateSettings::self()->save();
}


void GlaxnimateWindow::Private::switch_tool(tools::Tool* tool)
{
    if ( !tool || tool == active_tool )
        return;

    QAction* action = parent->actionCollection()->action(tool->action_name());
    if ( !action->isChecked() )
        action->setChecked(true);

    if ( active_tool )
    {
        if (active_tool->id() == QStringLiteral("edit") )
        {
            KToolBar* toolbar_node = parent->toolBar(QStringLiteral("nodeToolBar"));
            toolbar_node->setVisible(false);
            toolbar_node->setEnabled(false);
        }

        for ( const auto& action : tool_actions[active_tool->id()] )
        {
            action->setEnabled(false);
        }


        if ( auto widget = qobject_cast<ToolWidgetBase*>(active_tool->get_settings_widget()) )
            widget->save_settings();
    }

    if (tool->id() == QStringLiteral("edit") )
    {
        KToolBar* toolbar_node = parent->toolBar(QStringLiteral("nodeToolBar"));
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
                    show_warning(i18n("Trace Bitmap"), i18n("Only select one image"), log::Info);
                    return;
                }
                bmp = image;
            }
        }

        if ( !bmp )
        {
            show_warning(i18n("Trace Bitmap"), i18n("You need to select an image to trace"), log::Info);
            return;
        }
    }

    if ( !bmp->image.get() )
    {
        show_warning(i18n("Trace Bitmap"), i18n("You selected an image with no data"), log::Info);
        return;
    }

    TraceDialog dialog(bmp, parent);
    dialog.exec();
    if ( auto created = dialog.created() )
        layers_dock->layer_view()->set_current_node(created);
}


void GlaxnimateWindow::Private::trigger_plugin_action(plugin::ActionService* service)
{
    QVariantMap settings_value;
    if ( !service->script.settings.empty() )
    {
        if ( !glaxnimate::gui::WidgetBuilder().show_dialog(
            service->script.settings, settings_value, service->plugin()->data().name
        ) )
            return;
    }

    service->trigger(settings_value);
}


void GlaxnimateWindow::Private::init_plugins()
{
    plugin_category = new KActionCategory(i18n("Plugins"), parent->actionCollection());

    auto& par = plugin::PluginActionRegistry::instance();
    for ( plugin::ActionService* service : par.enabled() )
    {
        QAction* qaction = par.make_qaction(service);
        plugin_actions.append(qaction);
        plugin_category->addAction(qaction->objectName(), qaction);
        connect(qaction, &QAction::triggered, service, [service, this]{trigger_plugin_action(service);});
    }

    parent->unplugActionList("plugins_actionlist");
    parent->plugActionList("plugins_actionlist", plugin_actions);

    connect(&par, &plugin::PluginActionRegistry::action_added, parent, [this](plugin::ActionService* service, plugin::ActionService* before) {
        qsizetype index = -1;
        for ( auto act : plugin_actions )
        {
            if ( act->data().value<plugin::ActionService*>() == before )
            {
                index = plugin_actions.indexOf(act);
                break;
            }
        }
        QAction* qaction = plugin::PluginActionRegistry::instance().make_qaction(service);
        connect(qaction, &QAction::triggered, service, [service, this]{trigger_plugin_action(service);});
        plugin_actions.insert(qMax(qsizetype(0), qsizetype(index -1)), qaction);
        plugin_category->addAction(qaction->objectName(), qaction);

        parent->unplugActionList("plugins_actionlist");
        parent->plugActionList("plugins_actionlist", plugin_actions);

    });

    connect(&par, &plugin::PluginActionRegistry::action_removed, parent, [this](plugin::ActionService* action) {
        QString slug = plugin::PluginActionRegistry::instance().qaction_name(action);;
        auto iter = std::find_if(plugin_actions.begin(), plugin_actions.end(), [&slug](QAction* action){ return action->objectName() == slug; });
        if ( iter != plugin_actions.end() )
        {
            parent->actionCollection()->removeAction(*iter);
            plugin_actions.erase(iter);

            parent->unplugActionList("plugins_actionlist");
            parent->plugActionList("plugins_actionlist", plugin_actions);
        }
    });

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
    if ( !GlaxnimateSettings::startup_dialog() )
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
    options.format = io::IoRegistry::instance().from_slug("lottie", io::ImportExport::Import);
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
