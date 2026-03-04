/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate_app.hpp"


#include <QDir>
#include <QMetaEnum>
#include <QStandardPaths>

#include <KAboutData>
#include <KLocalizedString>

#include "glaxnimate_settings.hpp"
#include "settings/icon_settings.hpp"
#include "glaxnimate/utils/data_paths.hpp"
#include "glaxnimate/log/log.hpp"
#include "glaxnimate/app_info.hpp"

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

void GlaxnimateApp::on_initialize()
{
}

void GlaxnimateApp::on_initialize_settings()
{
}

#else

#include <QPalette>
#include <QClipboard>

#include <KLocalizedString>

#include "glaxnimate_settings.hpp"
#include "glaxnimate/log/listener_file.hpp"
#include "settings/plugin_settings_group.hpp"
#include "settings/clipboard_settings.hpp"
#include "settings/api_credentials.hpp"
#include "trace/trace.hpp"


void GlaxnimateApp::on_initialize()
{
    log::Logger::instance().add_listener<log::ListenerFile>(utils::writable_data_path("log.txt"));
    log::Logger::instance().add_listener<log::ListenerStderr>();
    store_logger = log::Logger::instance().add_listener<log::ListenerStore>();

    setWindowIcon(QIcon(data_file("images/logo.svg")));

    QDir().mkpath(backup_path());
}

void GlaxnimateApp::on_initialize_settings()
{
    GlaxnimateSettings::self()->add_group(std::make_unique<settings::PluginSettingsGroup>(QStringList{
        "AnimatedRaster", "ReplaceColor", "dotLottie", "FrameByFrame"
    }));
    GlaxnimateSettings::self()->add_group(std::make_unique<settings::ClipboardSettings>());

    GlaxnimateSettings::self()->add_group(std::make_unique<settings::ApiCredentials>());

}

void GlaxnimateApp::set_clipboard_data(QMimeData *data)
{
    return QGuiApplication::clipboard()->setMimeData(data);
}

const QMimeData *GlaxnimateApp::get_clipboard_data()
{
    return QGuiApplication::clipboard()->mimeData();
}

QString GlaxnimateApp::data_file(const QString& name) const
{
    QStringList found;

    for ( const QDir& d: utils::data_roots() )
    {
        if ( d.exists(name) )
            return QDir::cleanPath(d.absoluteFilePath(name));
    }

    return {};
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

bool GlaxnimateApp::event(QEvent *event)
{
    if ( event->type() == QEvent::ApplicationPaletteChange )
        gui::settings::IconSettings::instance().palette_change();

    return QApplication::event(event);
}

#ifdef OPENGL_ENABLED

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
static void gl_version(KAboutData& aboutData)
{
    QOffscreenSurface surface;
    surface.create();

    QOpenGLContext context;
    if ( !context.create() )
        return;
    context.makeCurrent(&surface);

    QOpenGLFunctions *f = context.functions();
    f->initializeOpenGLFunctions();

    aboutData.addComponent(
        i18n("OpenGL"),
        i18n("Hardware-accelerated graphics."),
        (const char*)f->glGetString(GL_VERSION),
        QStringLiteral("https://www.opengl.org/")
    );

    aboutData.addComponent(
        i18n("OpenGL Renderer"),
        {},
        (const char*)f->glGetString(GL_RENDERER),
        QStringLiteral("https://www.opengl.org/")
    );

    context.doneCurrent();
}
#endif

#ifdef GLAXNIMATE_VIDEO_ENABLED
#   include "glaxnimate/io/video/video_format.hpp"
#endif

#ifdef GLAXNIMATE_PYTHON_ENABLED
#   include "plugin/python/python_engine.hpp"
#endif

void GlaxnimateApp::init_qapplication()
{
    const auto& info = glaxnimate::AppInfo::instance();
    KAboutData aboutData(
        info.slug(),
        info.name(),
        info.version(),
        info.description(),
        KAboutLicense::GPL,
        i18n("(c) 2019-2026"),
        // Optional text shown in the About box.
        QStringLiteral(""),
        info.url_docs().toString()
    );

    aboutData.setOrganizationDomain(QByteArray("kde.org"));
    aboutData.setDesktopFileName(info.project_id());

    aboutData.addAuthor(i18n("Mattia \"Glax\" Basaglia"), i18n("Maintainer"), QStringLiteral("glax@dragon.best"));


#ifdef GLAXNIMATE_VIDEO_ENABLED
    aboutData.addComponent(i18n("libav"), {}, io::video::VideoFormat::library_version(), QStringLiteral("https://libav.org/"), KAboutLicense::LGPL);
#endif

#ifndef Q_OS_ANDROID
    aboutData.addComponent(i18n("potrace"), i18n("Used by the bitmap tracing feature."), utils::trace::Tracer::potrace_version(), QStringLiteral("http://potrace.sourceforge.net/"), KAboutLicense::GPL_V2);
    aboutData.addComponent(i18n("Inkscape"), {}, {}, QStringLiteral("https://inkscape.org/"), KAboutLicense::GPL_V2);
#endif
#ifdef GLAXNIMATE_PYTHON_ENABLED
    aboutData.addComponent(i18n("pybind11"), i18n("Used by the plugin system."), plugin::python::PythonEngine::pybind_version(), QStringLiteral("https://pybind11.readthedocs.io/en/stable/"));
    aboutData.addComponent(i18n("CPython"), i18n("Used by the plugin system."), plugin::python::PythonEngine::python_version(), QStringLiteral("hhttps://python.org/"));
#endif
#ifdef OPENGL_ENABLED
    gl_version(aboutData);
#endif

    KAboutData::setApplicationData(aboutData);
    if ( qApp )
        qApp->setOrganizationName(info.organization());
}


void GlaxnimateApp::initialize()
{
    on_initialize();
    on_initialize_settings();
}

void GlaxnimateApp::finalize()
{
}


bool GlaxnimateApp::notify(QObject* receiver, QEvent* e)
{
    try {
        return QApplication::notify(receiver, e);
    } catch ( const std::exception& exc ) {
        log::Log("Event", QMetaEnum::fromType<QEvent::Type>().valueToKey(e->type())).stream(log::Error) << "Exception:" << exc.what();
        return false;
    }
}
