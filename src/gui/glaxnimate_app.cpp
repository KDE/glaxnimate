/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate_app.hpp"

#include "app_info.hpp"

#include <QDir>
#include <QStandardPaths>
#include "glaxnimate_settings.hpp"

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

#include "glaxnimate_settings.hpp"
#include "app/settings/palette_settings.hpp"
#include "app/settings/keyboard_shortcuts.hpp"
#include "app/log/listener_file.hpp"
#include "settings/plugin_settings_group.hpp"
#include "settings/clipboard_settings.hpp"
#include "settings/toolbar_settings.hpp"
#include "settings/api_credentials.hpp"
#include "settings/icon_theme.hpp"

void GlaxnimateApp::on_initialize()
{

    app::log::Logger::instance().add_listener<app::log::ListenerFile>(writable_data_path("log.txt"));
    app::log::Logger::instance().add_listener<app::log::ListenerStderr>();
    store_logger = app::log::Logger::instance().add_listener<app::log::ListenerStore>();

    setWindowIcon(QIcon(data_file("images/logo.svg")));

    QStringList search_paths = data_paths("icons");
    search_paths += QIcon::themeSearchPaths();
    QIcon::setThemeSearchPaths(search_paths);
    QIcon::setFallbackSearchPaths(data_paths("images/icons"));


    QDir().mkpath(backup_path());
}

void GlaxnimateApp::on_initialize_settings()
{
    app::settings::Settings::instance().add_group(std::make_unique<settings::ToolbarSettingsGroup>());
    app::settings::Settings::instance().add_group(std::make_unique<settings::PluginSettingsGroup>(QStringList{
        "AnimatedRaster", "ReplaceColor", "dotLottie", "FrameByFrame"
    }));
    GlaxnimateSettings::self()->add_group(std::make_unique<settings::ClipboardSettings>());

    // auto palette_settings = std::make_unique<app::settings::PaletteSettings>();
    // load_themes(this, palette_settings.get());
    // app::settings::Settings::instance().add_group(std::move(palette_settings));

    // auto sc_settings = std::make_unique<app::settings::ShortcutSettings>();
    // shortcut_settings = sc_settings.get();
    // app::settings::Settings::instance().add_group(std::move(sc_settings));

    GlaxnimateSettings::self()->add_group(std::make_unique<settings::ApiCredentials>());

    IconThemeManager::instance().set_theme_and_fallback(GlaxnimateSettings::self()->icon_theme());
    connect(GlaxnimateSettings::self(), &GlaxnimateSettings::icon_theme_changed, &IconThemeManager::instance(), &IconThemeManager::set_theme);

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

void glaxnimate::gui::GlaxnimateApp::on_initialize_translations()
{
    app::TranslationService::instance().initialize("en_US");
}

bool GlaxnimateApp::event(QEvent *event)
{
    if ( event->type() == QEvent::ApplicationPaletteChange )
        IconThemeManager::instance().update_from_application_palette();

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
