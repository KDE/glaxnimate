/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate_app.hpp"

#include "app_info.hpp"

#include <QDir>
#include <QStandardPaths>

using namespace glaxnimate::gui;
using namespace glaxnimate;

#ifdef MOBILE_UI

#include <QScreen>

static qreal get_mult()
{
#ifndef Q_OS_ANDROID
    return 1;
}
#else
    auto sz = QApplication::primaryScreen()->size();
    return qMin(sz.width(), sz.height()) / 240.;

}

QString GlaxnimateApp::data_file(const QString &name) const
{
    return "assets:/share/glaxnimate/glaxnimate/" + name;
}

#endif

qreal GlaxnimateApp::handle_size_multiplier()
{
    static qreal mult = get_mult();
    return mult;
}

qreal GlaxnimateApp::handle_distance_multiplier()
{
    return handle_size_multiplier() / 2.;
}

void GlaxnimateApp::set_clipboard_data(QMimeData *data)
{
    clipboard.reset(data);
}

const QMimeData *GlaxnimateApp::get_clipboard_data()
{
    return clipboard.get();
}

#else

#include <QPalette>
#include <QClipboard>

#include <KLocalizedString>

#include "app/settings/settings.hpp"
#include "app/settings/palette_settings.hpp"
#include "app/log/listener_file.hpp"
#include "settings/plugin_settings_group.hpp"
#include "settings/clipboard_settings.hpp"
#include "settings/toolbar_settings.hpp"
#include "settings/api_credentials.hpp"

static QVariantMap avail_icon_themes()
{
    QVariantMap avail_icon_themes;
    avail_icon_themes[i18n("Glaxnimate Default")] = "";
    for ( QDir search : QIcon::themeSearchPaths() )
    {
        for ( const auto& avail : search.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot) )
        {
            QDir subdir(avail.filePath());
            if ( subdir.exists("index.theme") )
                avail_icon_themes[avail.baseName()] = avail.baseName();
        }
    }

    return avail_icon_themes;
}

static QString default_icon_theme()
{
    QPalette palette = QGuiApplication::palette();
    if ( palette.color(QPalette::Button).value() < 100 )
        return "breeze-dark";
    else
        return "breeze";
}

static void set_icon_theme(const QVariant& v)
{
    QString theme_name = v.toString();

    if ( theme_name.isEmpty() )
        theme_name = default_icon_theme();

    QIcon::setThemeName(theme_name);
}

static void icon_theme_fixup()
{
    QString default_theme = default_icon_theme();
    QIcon::setFallbackThemeName(default_theme);

    QString old = QIcon::themeName();
    if ( old == "breeze" || old == "breeze-dark" )
        QIcon::setThemeName(default_theme);
    else
        set_icon_theme(old);
}

static void load_themes(GlaxnimateApp* app, app::settings::PaletteSettings* settings)
{
    for ( QDir themedir : app->data_paths("themes") )
    {
        for ( const auto& theme : themedir.entryList({"*.ini"}, QDir::Files|QDir::Readable, QDir::Name|QDir::IgnoreCase) )
        {
            QSettings ini_parser(themedir.absoluteFilePath(theme), QSettings::IniFormat);
            settings->load_palette(ini_parser, true);
        }
    }
}

void GlaxnimateApp::on_initialize()
{

    app::log::Logger::instance().add_listener<app::log::ListenerFile>(writable_data_path("log.txt"));
    app::log::Logger::instance().add_listener<app::log::ListenerStderr>();
    store_logger = app::log::Logger::instance().add_listener<app::log::ListenerStore>();

    setWindowIcon(QIcon(data_file("images/logo.svg")));

    QDir().mkpath(backup_path());
}

void GlaxnimateApp::on_initialize_settings()
{
    using namespace app::settings;

    Settings::instance().add_group("ui", i18nc("@title settings group", "User Interface"), "preferences-desktop-theme", {
        //      slug            Label/Tooltip                                               Type                default     choices             side effects
        Setting("icon_theme", i18nc("@label:listbox", "Icon Theme"),  {},             Setting::String,    "",         avail_icon_themes(), ::set_icon_theme),
        // Setting("startup_dialog", i18nc("@option:check", "Show startup dialog"), {},  Setting::Bool,      true),
        Setting("timeline_scroll_horizontal",
                i18nc("@option:check", "Horizontal Timeline Scroll"),
                i18nc("@info:tooltip", "If enabled, the timeline will scroll horizontally by default and vertically with Shift or Alt"),
                                                                                            Setting::Bool,      false),
        Setting("layout",            {}, {},                                                Setting::Internal,  0),
        Setting("window_state",      {}, {},                                                Setting::Internal,  QByteArray{}),
        Setting("window_geometry",   {}, {},                                                Setting::Internal,  QByteArray{}),
        Setting("timeline_splitter", {}, {},                                                Setting::Internal,  QByteArray{}),
    });
    Settings::instance().add_group("defaults", i18nc("@title settings group", "New Animation Defaults"), "video-webm", {
        Setting("width",
            i18nc("@label:spinbox", "Width"),    "",
            512,   0, 1000000),
        Setting("height",
            i18nc("@label:spinbox", "Height"),   "",
            512,   0, 1000000),
        Setting("fps",
            i18nc("@label:spinbox", "FPS"),
            i18nc("@info:tooltip", "Frames per second"),
            60.f, 0.f, 1000.f),
        Setting("duration",
            i18nc("@label:spinbox", "Duration"),
            i18nc("@info:tooltip", "Duration in seconds"),
            3.f, 0.f, 90000.f),
    });
    Settings::instance().add_group("open_save", i18nc("@title settings group", "Open / Save"), "kfloppy", {
        Setting("max_recent_files", i18nc("@label:spinbox", "Max Recent Files"), {},      5, 0, 16),
        Setting("path",             {},         {},                        Setting::Internal,  QString{}),
        Setting("recent_files",     {},         {},                        Setting::Internal,  QStringList{}),
        Setting("backup_frequency",
                i18nc("@label:spinbox", "Backup Frequency"),
                i18nc("@info:tooltip", "How often to save a backup copy (in minutes)"),  5, 0, 60),
        Setting("render_path",      {},         {},                        Setting::Internal,  QString{}),
        Setting("import_path",      {},         {},                        Setting::Internal,  QString{}),
        Setting("native_dialog",    i18nc("@option:check", "Use system file dialog"), {}, Setting::Bool, true),
    });
    Settings::instance().add_group("scripting", i18nc("@title settings group", "Scripting"), "utilities-terminal", {
        //      slug                Label       Tooltip                    Type                default
        Setting("history",          {},         {},                        Setting::Internal,  QStringList{}),
        Setting("max_history",      {},         {},                        Setting::Internal,  100),
    });
    Settings::instance().add_group("tools", i18nc("@title settings group", "Tools"), "tools", {
        //      slug                Label       Tooltip                    Type                default
        Setting("shape_group",      {},         {},                        Setting::Internal,  true),
        Setting("shape_fill",       {},         {},                        Setting::Internal,  true),
        Setting("shape_stroke",     {},         {},                        Setting::Internal,  true),
        Setting("edit_mask",        {},         {},                        Setting::Internal,  false),
        Setting("color_main",       {},         {},                        Setting::Internal,  "#ffffff"),
        Setting("color_secondary",  {},         {},                        Setting::Internal,  "#000000"),
        Setting("stroke_width",     {},         {},                        Setting::Internal,  1.),
        Setting("stroke_cap",       {},         {},                        Setting::Internal,  int(Qt::RoundCap)),
        Setting("stroke_join",      {},         {},                        Setting::Internal,  int(Qt::RoundJoin)),
        Setting("stroke_miter",     {},         {},                        Setting::Internal,  4.),
        Setting("star_type",        {},         {},                        Setting::Internal,  1),
        Setting("star_ratio",       {},         {},                        Setting::Internal,  0.5),
        Setting("star_points",      {},         {},                        Setting::Internal,  5),
    });
    // catch all
    Settings::instance().add_group("internal", "", "", {});

    app::settings::Settings::instance().add_group(std::make_unique<settings::ToolbarSettingsGroup>());
    app::settings::Settings::instance().add_group(std::make_unique<settings::PluginSettingsGroup>(QStringList{
        "AnimatedRaster", "ReplaceColor", "dotLottie", "FrameByFrame"
    }));
    app::settings::Settings::instance().add_group(std::make_unique<settings::ClipboardSettings>());

    auto palette_settings = std::make_unique<app::settings::PaletteSettings>();
    load_themes(this, palette_settings.get());
    app::settings::Settings::instance().add_group(std::move(palette_settings));

    app::settings::Settings::instance().add_group(std::make_unique<settings::ApiCredentials>());

}

app::settings::ShortcutSettings * GlaxnimateApp::shortcuts() const
{
    return shortcut_settings;
}

void GlaxnimateApp::set_clipboard_data(QMimeData *data)
{
    return QGuiApplication::clipboard()->setMimeData(data);
}

const QMimeData *GlaxnimateApp::get_clipboard_data()
{
    return QGuiApplication::clipboard()->mimeData();
}

bool GlaxnimateApp::event(QEvent *event)
{
    if ( event->type() == QEvent::ApplicationPaletteChange )
        icon_theme_fixup();

    return app::Application::event(event);
}

#endif



QString GlaxnimateApp::temp_path()
{
    QDir tempdir = QDir::temp();
    QString subdir = AppInfo::instance().slug();

    if ( !tempdir.exists(subdir) )
        if ( !tempdir.mkpath(subdir) )
            return "";

    return tempdir.filePath(subdir);
}

QString GlaxnimateApp::backup_path() const
{

    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QLatin1String("/stalefiles/")
        + QCoreApplication::instance()->applicationName()
    ;
}
