#pragma once

#include <vector>
#include <unordered_map>

#include <QKeySequence>
#include <QAction>
#include <QMenu>
#include <QPointer>

#include "app/utils/qstring_hash.hpp"
#include "custom_settings_group.hpp"

namespace app::settings {

struct ShortcutAction
{
    QIcon icon;
    QString label;
    QKeySequence shortcut;
    QKeySequence default_shortcut;
    bool overwritten = false;
    QPointer<QAction> action;
};

struct ShortcutGroup
{
    QString label;
    std::vector<ShortcutAction*> actions;
};


class ShortcutSettings : public CustomSettingsGroupBase
{
public:
    QString slug() const override { return "shortcuts"; }
    QString label() const override { return QObject::tr("Keyboard Shortcuts"); }
    QIcon icon() const override { return QIcon::fromTheme("input-keyboard"); }
    QWidget * make_widget(QWidget * parent) override;

    void load(QSettings & settings) override;
    void save(QSettings & settings) override;

    ShortcutGroup* add_group(const QString& label);
    void add_menu(QMenu* menu, const QString& prefix = {});
    ShortcutAction* action(const QString& slug);
    ShortcutAction* add_action(QAction* action, const QString& prefix = {});

    const QList<ShortcutGroup>& get_groups() const;
    const std::unordered_map<QString, ShortcutAction>& get_actions() const;

private:
    QList<ShortcutGroup> groups;
    std::unordered_map<QString, ShortcutAction> actions;
};


} // namespace app::settings
