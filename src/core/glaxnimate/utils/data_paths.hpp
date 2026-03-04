/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once
#include <QDir>

namespace glaxnimate::utils {


    /**
     * \brief A path to write user preferences into
     * \param name Name of the data subdirectory
     */
    QString writable_data_path(const QString& name);

    /**
     * \brief Get all available directories to search data from
     * \param name Name of the data directory
     */
    QStringList data_paths(const QString& name);

    /**
     * \brief Get all directories to search data from
     *
     * This function may include directories that don't exist but that will be
     * checked if they existed
     *
     * \param name Name of the data directory
     */
    QStringList data_paths_unchecked(const QString& name);

    /**
     * \brief Get all possible directories to search data from
     */
    QList<QDir> data_roots();

} // namespace glaxnimate::utils
