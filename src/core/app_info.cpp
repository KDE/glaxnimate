/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "app_info.hpp"

#include "utils/i18n.hpp"
#include "application_info_generated.hpp"

QString glaxnimate::AppInfo::name() const
{
    return i18n("Glaxnimate");
}

QString glaxnimate::AppInfo::slug() const
{
    return QStringLiteral(PROJECT_SLUG);
}

QString glaxnimate::AppInfo::version() const
{
    return QStringLiteral(PROJECT_VERSION);
}

QString glaxnimate::AppInfo::organization() const
{
    return QStringLiteral(PROJECT_SLUG);
}

QUrl glaxnimate::AppInfo::url_docs() const
{
    return QUrl(URL_DOCS);
}

QString glaxnimate::AppInfo::description() const
{
    return QStringLiteral(PROJECT_DESCRIPTION);
}

QString glaxnimate::AppInfo::project_id() const
{
    return QStringLiteral(PROJECT_ID);
}
