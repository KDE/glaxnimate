/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
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

#include "cli_utils/env.hpp"
#include "gui_python/python_engine.hpp"
#include "glaxnimate/log/log.hpp"

#include "cli.hpp"
#include "glaxnimate/app_info.hpp"
#include "glaxnimate/io/io_registry.hpp"
#include "glaxnimate/io/lottie/lottie_html_format.hpp"
#include "glaxnimate/utils/data_paths.hpp"

#include "widgets/dialogs/glaxnimate_window.hpp"
#include "settings/icon_settings.hpp"

// Uncomment the following line to add some minimal tracing of qDebug() messages
// #define GLAXNIMATE_DETAILED_QDEBUG
#ifdef GLAXNIMATE_DETAILED_QDEBUG

#include <stdio.h>
#include <stdlib.h>

void detailed_qdebug(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    const char* type_name = "Unknown";
    switch ( type )
    {
        case QtDebugMsg:
            type_name = "Debug";
            break;
        case QtInfoMsg:
            type_name = "Info";
            break;
        case QtWarningMsg:
            type_name = "Warning";
            break;
        case QtCriticalMsg:
            type_name = "Critical";
            break;
        case QtFatalMsg:
            type_name = "Fatal";
            break;
    }

    if ( context.file )
    {
        fprintf(
            stderr,
            "%s: %s:%u (%s): %s\n",
            type_name,
            context.file,
            context.line,
            context.function,
            localMsg.constData()
        );
    }
    else
    {
        fprintf(
            stderr,
            "%s: %s\n",
            type_name,
            localMsg.constData()
        );
    }

    if ( type == QtFatalMsg )
        abort();
}
#endif

using namespace glaxnimate;

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

#ifdef GLAXNIMATE_DETAILED_QDEBUG
    qInstallMessageHandler(detailed_qdebug);
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

    gui::GlaxnimateApp::init_qapplication();

    plugin::python::PythonEngine::add_module_search_paths(utils::data_paths("lib/"));

    io::IoRegistry::load_formats();

    auto args = gui::parse_cli(app.arguments());

    gui::cli_main(app, args);

    if ( args.return_value )
        return *args.return_value;

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
