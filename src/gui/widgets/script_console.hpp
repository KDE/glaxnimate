#ifndef SCRIPTCONSOLE_H
#define SCRIPTCONSOLE_H

#include <memory>
#include <QWidget>

#include "plugin/executor.hpp"

class PluginUiDialog;

class ScriptConsole : public QWidget, public plugin::Executor
{
    Q_OBJECT

public:
    ScriptConsole(QWidget* parent = nullptr);
    ~ScriptConsole();

    bool execute(const plugin::Plugin& plugin, const plugin::PluginScript& script, const QVariantList& in_args) override;
    QVariant get_global(const QString& name) override;
    void set_global(const QString& name, const QVariant& value);

    PluginUiDialog* create_dialog(const QString& ui_file) const;

    void clear_contexts();
    void save_settings();

protected:
    void changeEvent ( QEvent* e ) override;


private slots:
    void console_commit(const QString& command);
    void console_clear();

signals:
    void error(const QString& plugin, const QString& message);

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // SCRIPTCONSOLE_H
