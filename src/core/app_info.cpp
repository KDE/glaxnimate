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

QUrl glaxnimate::AppInfo::url_issues() const
{
    return QUrl(URL_ISSUES);
}

QString glaxnimate::AppInfo::description() const
{
    return PROJECT_DESCRIPTION;
}

QUrl glaxnimate::AppInfo::url_donate() const
{
    return QUrl(URL_DONATE);
}

void glaxnimate::AppInfo::init_qapplication() const
{
    qApp->setApplicationName(slug());
    qApp->setApplicationVersion(version());
    qApp->setOrganizationName(organization());
    qApp->setApplicationDisplayName(name());

    KAboutData aboutData(
        QStringLiteral(PROJECT_SLUG),
        name(),
        QStringLiteral(PROJECT_VERSION),
        QStringLiteral(PROJECT_DESCRIPTION),
        KAboutLicense::GPL,
        i18n("(c) 2019-2023"),
        // Optional text shown in the About box.
        QStringLiteral(""),
        QStringLiteral(URL_DOCS),
        QStringLiteral(URL_ISSUES)
    );

    KAboutData::setApplicationData(aboutData);
}
