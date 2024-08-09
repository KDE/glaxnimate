/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "script_console.hpp"
#include "ui_scriptconsole.h"

#include <QEvent>
#include <QRegularExpression>

#include <KCompletion>

#include "app/settings/settings.hpp"
#include "app/scripting/script_engine.hpp"
#include "plugin/plugin.hpp"
#include "widgets/dialogs/plugin_ui_dialog.hpp"

using namespace glaxnimate::gui;
using namespace glaxnimate;

class ScriptConsoleDock::Private
{
public:
    Ui::dock_script_console ui;

    std::vector<app::scripting::ScriptContext> script_contexts;
    const plugin::Plugin* current_plugin = nullptr;
    ScriptConsoleDock* parent;
    std::map<QString, QVariant> globals;
    QRegularExpression re_completion{R"(^[a-zA-Z_0-9.\s\[\]]+$)"};
    KCompletion completion;

    bool ensure_script_contexts()
    {
        if ( script_contexts.empty() )
        {
            create_script_context();
            if ( script_contexts.empty() )
                return false;
        }

        return true;
    }

    bool execute_script ( const plugin::Plugin& plugin, const plugin::PluginScript& script, const QVariantList& args )
    {
        if ( !ensure_script_contexts() )
            return false;

        for ( const auto& ctx : script_contexts )
        {
            if ( ctx->engine() == plugin.data().engine )
            {
                current_plugin = &plugin;
                bool ok = false;
                try {
                    ok = ctx->run_from_module(plugin.data().dir, script.module, script.function, args);
                    if ( !ok )
                        parent->error(plugin.data().name, i18n("Could not run the plugin"));
                } catch ( const app::scripting::ScriptError& err ) {
                    console_error(err);
                    parent->error(plugin.data().name, i18n("Plugin raised an exception"));
                    ok = false;
                }
                current_plugin = nullptr;
                return ok;
            }
        }

        parent->error(plugin.data().name, i18n("Could not find an interpreter"));
        return false;
    }

    void set_completions(const QString& prefix)
    {
        auto match = re_completion.match(prefix);
        if ( !match.hasMatch() )
        {
            ui.console_input->completionObject()->setItems({});
            return;
        }

        if ( !ensure_script_contexts() )
            return;

        int last_dot = prefix.lastIndexOf('.');
        QString evaluated;
        if ( last_dot != -1 )
            evaluated = prefix.left(last_dot);

        auto ctx = script_contexts[ui.console_language->currentIndex()].get();
        auto completions = ctx->eval_completions(evaluated);
        if ( !evaluated.isEmpty() )
        {
            for ( auto& item : completions )
                item = evaluated + "." + item;
        }
        ui.console_input->completionObject()->setItems(completions);
    }

    void run_snippet(const QString& text, bool echo)
    {
        if ( !ensure_script_contexts() )
            return;

        auto c = ui.console_output->textCursor();

        if ( echo )
            console_stdout("> " + text);

        auto ctx = script_contexts[ui.console_language->currentIndex()].get();
        try {
            QString out = ctx->eval_to_string(text);
            if ( !out.isEmpty() )
                console_stdout(out);
        } catch ( const app::scripting::ScriptError& err ) {
            console_error(err);
        }

        c.clearSelection();
        c.movePosition(QTextCursor::End);
        ui.console_output->setTextCursor(c);
    }

    void console_commit(QString text)
    {
        if ( text.isEmpty() )
            return;

        run_snippet(text.replace("\n", " "), true);

        ui.console_input->addToHistory(text);
        ui.console_input->clearEditText();
    }


    void console_stderr(const QString& line)
    {
        ui.console_output->setTextColor(Qt::red);
        ui.console_output->append(line);
    }

    void console_stdout(const QString& line)
    {
        ui.console_output->setTextColor(parent->palette().text().color());
        ui.console_output->append(line);
    }

    void console_error(const app::scripting::ScriptError& err)
    {
        console_stderr(err.message());
    }

    void create_script_context()
    {
        for ( const auto& engine : app::scripting::ScriptEngineFactory::instance().engines() )
        {
            auto ctx = engine->create_context();

            if ( !ctx )
                continue;

            connect(ctx.get(), &app::scripting::ScriptExecutionContext::stdout_line, [this](const QString& s){ console_stdout(s);});
            connect(ctx.get(), &app::scripting::ScriptExecutionContext::stderr_line, [this](const QString& s){ console_stderr(s);});

            try {
                ctx->app_module("glaxnimate");
                ctx->app_module("glaxnimate_gui");
                for ( const auto& p : globals )
                    ctx->expose(p.first, p.second);
            } catch ( const app::scripting::ScriptError& err ) {
                console_error(err);
            }

            script_contexts.push_back(std::move(ctx));
        }
    }

    PluginUiDialog * create_dialog(const QString& ui_file)
    {
        if ( !current_plugin )
            return nullptr;

        if ( !current_plugin->data().dir.exists(ui_file) )
        {
            current_plugin->logger().stream(app::log::Error) << "UI file not found:" << ui_file;
            return nullptr;
        }

        QFile file(current_plugin->data().dir.absoluteFilePath(ui_file));
        if ( !file.open(QIODevice::ReadOnly) )
        {
            current_plugin->logger().stream(app::log::Error) << "Could not open UI file:" << ui_file;
            return nullptr;
        }

        return new PluginUiDialog(file, *current_plugin, parent);
    }
};

ScriptConsoleDock::ScriptConsoleDock(QWidget* parent)
    : QDockWidget(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
    d->parent = this;

    d->ui.console_input->setHistoryItems(app::settings::get<QStringList>("scripting", "history"));
    d->ui.console_input->completionObject()->setCompletionMode(KCompletion::CompletionPopup);
    d->ui.console_input->completionObject()->setOrder(KCompletion::Sorted);
    connect(d->ui.console_input, &KHistoryComboBox::completion, this, [this](const QString& text){ d->set_completions(text); });

    for ( const auto& engine : app::scripting::ScriptEngineFactory::instance().engines() )
    {
        d->ui.console_language->addItem(engine->label());
        if ( engine->slug() == "python" )
            d->ui.console_language->setCurrentIndex(d->ui.console_language->count()-1);
    }

    connect(d->ui.btn_reload, &QAbstractButton::clicked, this, &ScriptConsoleDock::clear_contexts);
}

ScriptConsoleDock::~ScriptConsoleDock() = default;

void ScriptConsoleDock::changeEvent ( QEvent* e )
{
    QDockWidget::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void ScriptConsoleDock::console_clear()
{
    d->ui.console_output->clear();
}

void ScriptConsoleDock::console_commit(const QString& command)
{
    d->console_commit(command);
}

bool ScriptConsoleDock::execute(const plugin::Plugin& plugin, const plugin::PluginScript& script, const QVariantList& args)
{
    return d->execute_script(plugin, script, args);
}

QVariant ScriptConsoleDock::get_global(const QString& name)
{
    auto it = d->globals.find(name);
    if ( it != d->globals.end() )
        return it->second;
    return {};
}

void ScriptConsoleDock::set_global(const QString& name, const QVariant& value)
{
    d->globals[name] = value;
}

void ScriptConsoleDock::clear_contexts()
{
    d->script_contexts.clear();
}

void ScriptConsoleDock::clear_output()
{
    if ( !d->ui.check_persist->isChecked() )
        console_clear();
}

PluginUiDialog* ScriptConsoleDock::create_dialog(const QString& ui_file) const
{
    return d->create_dialog(ui_file);
}

void ScriptConsoleDock::save_settings()
{
    QStringList history = d->ui.console_input->historyItems();
    int max_history = app::settings::get<int>("scripting", "max_history");
    if ( history.size() > max_history )
        history.erase(history.begin(), history.end() - max_history);
    app::settings::set("scripting", "history", history);
}

void ScriptConsoleDock::run_snippet(const QString& source)
{
    d->run_snippet(source, false);
}
