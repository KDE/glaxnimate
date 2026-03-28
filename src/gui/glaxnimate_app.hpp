/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>

#include <QtGlobal>
#include <QIcon>
#include <QMimeData>
#include <QApplication>


#ifndef MOBILE_UI
#   include "glaxnimate/log/listener_stderr.hpp"
#   include "glaxnimate/log/listener_store.hpp"
#endif

namespace glaxnimate::gui {

class GlaxnimateApp : public QApplication
{
    Q_OBJECT

public:
    GlaxnimateApp(int &argc, char **argv);

    void initialize();

    void finalize();

    /**
     * \brief Path to get the file from
     * \param name Name of the data files
     */
    QString data_file(const QString& name) const;

    static GlaxnimateApp* instance()
    {
        return static_cast<GlaxnimateApp *>(QCoreApplication::instance());
    }

    bool notify(QObject *receiver, QEvent *e) override;

    QString backup_path() const;

    static QString temp_path();

    static qreal handle_size_multiplier();
    static qreal handle_distance_multiplier();


    void set_clipboard_data(QMimeData* data);
    const QMimeData* get_clipboard_data();

    static void init_about_data();

protected:

    bool event(QEvent *event) override;

private:
    /**
     * \brief Called after construction, before anything else
     * \note set application name and stuff in here
     */
    void on_initialize();

    /**
     * \brief Called after on_initialize() and after translations are loaded
     */
    void on_initialize_settings();


#ifdef MOBILE_UI
private:
    std::unique_ptr<QMimeData> clipboard = std::make_unique<QMimeData>();
#else
public:

    const std::vector<log::LogLine>& log_lines() const
    {
        return store_logger->lines();
    }

private:
    log::ListenerStore* store_logger;
#endif

};

} // namespace glaxnimate::gui
