/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/utils/data_paths.hpp"

#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>


QString glaxnimate::utils::writable_data_path(const QString& name)
{
    QString search = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    if ( !search.isEmpty() )
    {
        return QDir::cleanPath(QDir(search).absoluteFilePath(name));
    }

    return QString();
}

QStringList glaxnimate::utils::data_paths(const QString& name)
{
    QStringList found;

    for ( const QDir& d: data_roots() )
    {
        if ( d.exists(name) )
            found << QDir::cleanPath(d.absoluteFilePath(name));
    }
    found.removeDuplicates();

    return found;
}

QStringList glaxnimate::utils::data_paths_unchecked(const QString& name)
{
    QStringList filter;
    for ( const QDir& d: data_roots() )
    {
        filter << QDir::cleanPath(d.absoluteFilePath(name));
    }
    filter.removeDuplicates();

    return filter;
}

QList<QDir> glaxnimate::utils::data_roots()
{
    QList<QDir> search;
    // std paths
    for ( const QString& str : QStandardPaths::standardLocations(QStandardPaths::AppDataLocation) )
        search.push_back(QDir(str));
    // executable dir
    QDir binpath(QCoreApplication::applicationDirPath());
    auto app = QCoreApplication::instance();
#ifdef Q_OS_WIN
    // some Windows apps do not use a bin subfolder
    search.push_back(binpath.filePath(QString("share/%1/%2").arg(app->organizationName()).arg(app->applicationName())));
#endif
    binpath.cdUp();
    search.push_back(binpath.filePath(QString("share/%1/%2").arg(app->organizationName()).arg(app->applicationName())));
#ifdef Q_OS_MAC
    // some macOS app bundles use a Resources subfolder
    search.push_back(binpath.filePath(QString("Resources/%1/%2").arg(app->organizationName()).arg(app->applicationName())));
#endif

    return search;
}

