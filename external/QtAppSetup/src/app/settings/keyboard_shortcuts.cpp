/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "keyboard_shortcuts.hpp"
#include "app/widgets/keyboard_settings_widget.hpp"

void app::settings::ShortcutSettings::load(QSettings & settings)
{
    for ( const auto& key : settings.childKeys() )
    {
        auto& action = actions[key];
        action.overwritten = true;
        action.shortcut = QKeySequence(settings.value(key).toString(), QKeySequence::PortableText);
    }
}

void app::settings::ShortcutSettings::save(QSettings& settings)
{
    for ( const auto& p : actions )
    {
        if ( p.second.overwritten )
             settings.setValue(p.first, p.second.shortcut.toString(QKeySequence::PortableText));
        else
            settings.remove(p.first);
    }
}

void app::settings::ShortcutSettings::add_menu(QMenu* menu, const QString& prefix)
{
    auto group = add_group(menu->menuAction()->iconText());
    for ( QAction* act : menu->actions() )
    {
        if ( !act->isSeparator() && !act->menu() && !act->objectName().isEmpty() )
            group->actions.push_back(
                add_action(act, prefix)
            );
    }

    QObject::connect(menu->menuAction(), &QAction::changed, menu, [menu, group]{
        group->label = menu->menuAction()->iconText();
    });
}

app::settings::ShortcutGroup * app::settings::ShortcutSettings::add_group(const QString& label)
{
    groups.push_back(ShortcutGroup{label, {}});
    return &groups.back();
}

app::settings::ShortcutAction * app::settings::ShortcutSettings::action(const QString& slug)
{
    return &actions[slug];
}

app::settings::ShortcutAction * app::settings::ShortcutSettings::add_action(QAction* qaction, const QString& prefix)
{
    Q_EMIT begin_actions_change();
    auto sca = action(prefix + qaction->objectName());

    sca->icon = qaction->icon();
    sca->label = qaction->iconText();
    sca->default_shortcut = qaction->shortcut();
    if ( sca->overwritten )
        qaction->setShortcut(sca->shortcut);
    else
        sca->shortcut = qaction->shortcut();
    sca->action = qaction;

    QObject::connect(qaction, &QAction::changed, qaction, [qaction, sca]{
        sca->icon = qaction->icon();
        sca->label = qaction->iconText();
    });

    Q_EMIT end_actions_change();
    return sca;
}

const QList<app::settings::ShortcutGroup> & app::settings::ShortcutSettings::get_groups() const
{
    return groups;
}

QWidget * app::settings::ShortcutSettings::make_widget(QWidget* parent)
{
    return new KeyboardSettingsWidget(this, parent);
}

const std::unordered_map<QString, app::settings::ShortcutAction> & app::settings::ShortcutSettings::get_actions() const
{
    return actions;
}

const QKeySequence & app::settings::ShortcutSettings::get_shortcut(const QString& action_name) const
{
    return actions.at(action_name).shortcut;
}

app::settings::ShortcutGroup* app::settings::ShortcutSettings::find_group(const QString& label)
{
    for ( app::settings::ShortcutGroup& group : groups )
        if ( group.label == label )
            return &group;
    return {};
}

void app::settings::ShortcutSettings::remove_action(ShortcutAction* action)
{
    Q_EMIT begin_actions_change();
    QString name;
    name = action->action->objectName();
    for ( app::settings::ShortcutGroup& group : groups )
    {
        std::vector<ShortcutAction*>::iterator it = std::find(group.actions.begin(), group.actions.end(), action);
        if(it != group.actions.end())
        {
           group.actions.erase(it);
           break;
        }
    }
    actions.erase(name);
    Q_EMIT end_actions_change();
}
