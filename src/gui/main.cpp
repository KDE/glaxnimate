/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QSplashScreen>
#include <QtGlobal>

#include <kiconthemes_version.h>

#ifndef Q_OS_ANDROID
#   include <KCrash>
#endif

#define HAVE_STYLE_MANAGER __has_include(<KStyleManager>)
#if HAVE_STYLE_MANAGER
#include <KStyleManager>
#endif
#include <KIconTheme>

#include "app/env.hpp"
#include "app/scripting/python/python_engine.hpp"
#include "app/log/log.hpp"

#include "cli.hpp"
#include "app_info.hpp"
#include "io/io_registry.hpp"
#include "io/lottie/lottie_html_format.hpp"

#include "widgets/dialogs/glaxnimate_window.hpp"
#include "settings/icon_settings.hpp"

using namespace glaxnimate;

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
#if QT_VERSION_MAJOR < 6
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
#ifdef Q_OS_WIN
    // workaround crash bug #408 in Qt/Windows
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

// trigger initialisation of proper icon theme
#if KICONTHEMES_VERSION >= QT_VERSION_CHECK(6, 3, 0)
    KIconTheme::initTheme();
#endif

    gui::GlaxnimateApp app(argc, argv);
    KLocalizedString::setApplicationDomain("glaxnimate");

#if HAVE_STYLE_MANAGER
    // trigger initialisation of proper application style
    KStyleManager::initStyle();
#endif

#ifndef Q_OS_ANDROID
    KCrash::setDrKonqiEnabled(true);
    KCrash::initialize();
#endif

    AppInfo::instance().init_qapplication();

    app::scripting::python::PythonEngine::add_module_search_paths(app.data_paths("lib/"));

    auto args = gui::parse_cli(app.arguments());

    gui::cli_main(app, args);

    if ( args.return_value )
        return *args.return_value;

    gui::settings::IconSettings::instance().initialize();

#ifdef Q_OS_WIN
    auto pyhome = app::Environment::Variable("PYTHONHOME");
    if ( pyhome.empty() )
    {
        QDir binpath(QCoreApplication::applicationDirPath());
        binpath.cdUp();
        pyhome = binpath.absolutePath();
        app::log::Log("Python").log("Setting PYTHONHOME to " + pyhome.get(), app::log::Info);
    }
// #elif defined(Q_OS_MAC)
//     auto pyhome = app::Environment::Variable("PYTHONHOME");
//     if ( pyhome.empty() )
//     {
//         QDir binpath(QCoreApplication::applicationDirPath());
//         binpath.cdUp();
//         pyhome = binpath.absolutePath();
//         app::log::Log("Python").log("Setting PYTHONHOME to " + pyhome.get(), app::log::Info);
//     }
#endif

    qRegisterMetaType<app::log::Severity>();

    QSplashScreen sc;
    sc.setPixmap(QPixmap(":glaxnimate/splash.svg"));
    sc.show();
    app.processEvents();

    app.initialize();

    bool debug = args.has_flag("debug");
    if ( debug )
        io::IoRegistry::instance().register_object(std::make_unique<io::lottie::LottieHtmlFormat>());
    gui::GlaxnimateWindow window(!args.has_flag("default-ui"), debug);
    window.setAttribute(Qt::WA_DeleteOnClose, false);
    sc.finish(&window);
    window.show();

    if ( args.is_defined("ipc") )
        window.ipc_connect(args.value("ipc").toString());

    if ( args.is_defined("window-size") )
        window.resize(args.value("window-size").toSize());

    if ( args.has_flag("window-id") )
        app::cli::show_message(QString::number(window.winId(), 16), false);

    window.check_autosaves();

    if ( args.is_defined("file") )
    {
        QVariantMap open_settings;
        open_settings["trace"] = args.value("trace");
        window.document_open_settings(args.value("file").toString(), open_settings);
    }
    else
    {
        window.show_startup_dialog();
    }

    int ret = app.exec();

    app.finalize();

    return ret;
}
