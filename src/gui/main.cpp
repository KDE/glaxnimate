/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QSplashScreen>
#include <QtGlobal>
#include <QWindow>

#include <kiconthemes_version.h>

#ifndef Q_OS_ANDROID
#   include <KCrash>
#endif

#define HAVE_STYLE_MANAGER __has_include(<KStyleManager>)
#if HAVE_STYLE_MANAGER
#   include <KStyleManager>
#endif
#include <KIconTheme>

#include "gui_python/python_engine.hpp"

#include "cli.hpp"
#include "cli_utils/env.hpp"
#include "glaxnimate/log/log.hpp"
#include "glaxnimate/module/module.hpp"
#include "glaxnimate/module/video/video_module.hpp"
#include "glaxnimate/utils/data_paths.hpp"

#include "widgets/dialogs/glaxnimate_window.hpp"
#include "settings/icon_settings.hpp"

#ifdef GLAXNIMATE_CAIRO_ENABLED
#   include "glaxnimate/module/cairo/cairo_module.hpp"
#endif

using namespace glaxnimate;

void glaxnimate::gui::initialize_core()
{
    // This loads the build-in modules
    glaxnimate::module::initialize();
#ifdef GLAXNIMATE_VIDEO_ENABLED
    glaxnimate::module::registry().install<glaxnimate::video::Module>();
#endif
#ifdef GLAXNIMATE_CAIRO_ENABLED
    glaxnimate::module::registry().install<glaxnimate::cairo::Module>();
#endif
}

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
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

#ifndef Q_OS_ANDROID
    KCrash::setDrKonqiEnabled(true);
    KCrash::initialize();
#endif

    auto args = gui::parse_cli(app.arguments());

    QSplashScreen sc;
    if ( !gui::cli_no_gui(args) )
    {
        sc.setPixmap(QPixmap(":glaxnimate/splash.svg"));
        sc.show();

        // Force the splash screen to show
        for ( int i = 0; i < 200 && !sc.windowHandle()->isExposed(); i++)
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
            QCoreApplication::sendPostedEvents();
        }
    }

    gui::initialize_core();

    gui::GlaxnimateApp::init_about_data();

    plugin::python::PythonEngine::add_module_search_paths(utils::data_paths("lib/"));

    gui::cli_main(app, args);

    if ( args.return_value )
        return *args.return_value;

#if HAVE_STYLE_MANAGER
    // trigger initialisation of proper application style
    KStyleManager::initStyle();
#endif


    gui::settings::IconSettings::instance().initialize();

#ifdef Q_OS_WIN
    auto pyhome = cli::Environment::Variable("PYTHONHOME");
    if ( pyhome.empty() )
    {
        QDir binpath(QCoreApplication::applicationDirPath());
        binpath.cdUp();
        pyhome = binpath.absolutePath();
        log::Log("Python").log("Setting PYTHONHOME to " + pyhome.get(), log::Info);
    }
// #elif defined(Q_OS_MAC)
//     auto pyhome = app::Environment::Variable("PYTHONHOME");
//     if ( pyhome.empty() )
//     {
//         QDir binpath(QCoreApplication::applicationDirPath());
//         binpath.cdUp();
//         pyhome = binpath.absolutePath();
//         log::Log("Python").log("Setting PYTHONHOME to " + pyhome.get(), log::Info);
//     }
#endif

    qRegisterMetaType<log::Severity>();

    app.initialize();

    bool debug = args.has_flag("debug");
    gui::GlaxnimateWindow window(!args.has_flag("default-ui"), debug);
    window.setAttribute(Qt::WA_DeleteOnClose, false);
    sc.finish(&window);
    window.show();

    if ( args.is_defined("ipc") )
        window.ipc_connect(args.value("ipc").toString());

    if ( args.is_defined("window-size") )
        window.resize(args.value("window-size").toSize());

    if ( args.has_flag("window-id") )
        glaxnimate::cli::show_message(QString::number(window.winId(), 16), false);

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
