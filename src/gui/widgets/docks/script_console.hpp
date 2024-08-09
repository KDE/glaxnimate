/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SCRIPTCONSOLE_H
#define SCRIPTCONSOLE_H

#include <memory>
#include <QDockWidget>

#include "plugin/executor.hpp"

namespace glaxnimate::gui {

class PluginUiDialog;

class ScriptConsoleDock : public QDockWidget, public plugin::Executor
{
    Q_OBJECT

public:
    ScriptConsoleDock(QWidget* parent = nullptr);
    ~ScriptConsoleDock();

    bool execute(const plugin::Plugin& plugin, const plugin::PluginScript& script, const QVariantList& in_args) override;
    QVariant get_global(const QString& name) override;
    void set_global(const QString& name, const QVariant& value);

    PluginUiDialog* create_dialog(const QString& ui_file) const;

    void clear_contexts();
    void clear_output();
    void save_settings();

protected:
    void changeEvent ( QEvent* e ) override;

public Q_SLOTS:
    void run_snippet(const QString& source);

private Q_SLOTS:
    void console_commit(const QString& command);
    void console_clear();

Q_SIGNALS:
    void error(const QString& plugin, const QString& message);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui

#endif // SCRIPTCONSOLE_H
