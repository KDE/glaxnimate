/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "app_info.hpp"

#include <QGuiApplication>
#include <KAboutData>
#include <KLocalizedString>

#include "application_info_generated.hpp"
#include "utils/trace.hpp"
#include "utils/tar.hpp"
#include "io/video/video_format.hpp"
#include "app/scripting/python/python_engine.hpp"

QString glaxnimate::AppInfo::name() const
{
    return i18n("Glaxnimate");
}

QString glaxnimate::AppInfo::slug() const
{
    return PROJECT_SLUG;
}

QString glaxnimate::AppInfo::version() const
{
    return PROJECT_VERSION;
}

QString glaxnimate::AppInfo::organization() const
{
    return PROJECT_SLUG;
}

QUrl glaxnimate::AppInfo::url_docs() const
{
    return QUrl(URL_DOCS);
}

QString glaxnimate::AppInfo::description() const
{
    return PROJECT_DESCRIPTION;
}

void glaxnimate::AppInfo::init_qapplication() const
{
    KAboutData aboutData(
        slug(),
        name(),
        version(),
        QStringLiteral(PROJECT_DESCRIPTION),
        KAboutLicense::GPL,
        i18n("(c) 2019-2023"),
        // Optional text shown in the About box.
        QStringLiteral(""),
        QStringLiteral(URL_DOCS)
    );

    aboutData.setOrganizationDomain(QByteArray("kde.org"));
    aboutData.setDesktopFileName(QStringLiteral(PROJECT_ID));

    aboutData.addAuthor(i18n("Mattia \"Glax\" Basaglia"), i18n("Maintainer"), QStringLiteral("glax@dragon.best"));

    aboutData.addComponent(i18n("potrace"), i18n("Used by the bitmap tracing feature."), utils::trace::Tracer::potrace_version(), QStringLiteral("http://potrace.sourceforge.net/"), KAboutLicense::GPL_V2);
#ifndef Q_OS_ANDROID
    aboutData.addComponent(i18n("libav"), {}, io::video::VideoFormat::library_version(), QStringLiteral("https://libav.org/"), KAboutLicense::LGPL);
    aboutData.addComponent(i18n("libarchive"), {}, utils::tar::libarchive_version());
    aboutData.addComponent(i18n("zlib"), {}, {}, QStringLiteral("https://www.zlib.net/"));

    aboutData.addComponent(i18n("pybind11"), i18n("Used by the plugin system."), app::scripting::python::PythonEngine::pybind_version(), QStringLiteral("https://pybind11.readthedocs.io/en/stable/"));
    aboutData.addComponent(i18n("CPython"), i18n("Used by the plugin system."), app::scripting::python::PythonEngine::python_version(), QStringLiteral("https://python.org/"));

    aboutData.addComponent(i18n("Inkscape"), {}, {}, QStringLiteral("https://inkscape.org/"), KAboutLicense::GPL_V2);
#endif

    KAboutData::setApplicationData(aboutData);
    qApp->setOrganizationName(organization());
}
