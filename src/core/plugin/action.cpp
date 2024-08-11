/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "action.hpp"
#include "plugin.hpp"

#include "app/settings/widget_builder.hpp"

using namespace glaxnimate;

const std::vector<plugin::ActionService *> & plugin::PluginActionRegistry::enabled() const
{
    return enabled_actions;
}


QAction * plugin::PluginActionRegistry::make_qaction ( plugin::ActionService* action )
{
    QAction* act = new QAction;
    act->setIcon(action->plugin()->make_icon(action->icon));
    if ( action->label.isEmpty() )
        act->setText(action->plugin()->data().name);
    else
        act->setText(action->label);
    act->setToolTip(action->tooltip);
    connect(act, &QAction::triggered, action, &ActionService::trigger);
    connect(action, &ActionService::disabled, act, &QAction::deleteLater);
    act->setData(QVariant::fromValue(action));
    act->setObjectName(qaction_name(action));
    return act;
}

QString glaxnimate::plugin::PluginActionRegistry::qaction_name(ActionService* action)
{
    return QStringLiteral("action_plugin_") + action->plugin()->data().name.toLower() + QStringLiteral("_") + action->label.toLower();
}


void plugin::PluginActionRegistry::add_action ( plugin::ActionService* action )
{
    auto it = find(action);
    if ( it != enabled_actions.end() && *it == action )
        return;

    ActionService* sibling_before = nullptr;
    if ( it != enabled_actions.end() )
        sibling_before = *it;
    enabled_actions.insert(it, action);
    Q_EMIT action_added(action, sibling_before);
}

void plugin::PluginActionRegistry::remove_action ( plugin::ActionService* action )
{
    auto it = find(action);
    if ( it == enabled_actions.end() || *it != action )
        return;

    enabled_actions.erase(it);
    Q_EMIT action_removed(action);
}

bool plugin::PluginActionRegistry::compare(plugin::ActionService* a, plugin::ActionService* b)
{
    if ( a->plugin()->data().id == b->plugin()->data().id )
    {
        if ( a->label == b->label )
            return a < b;
        return a->label < b->label;
    }
    return a->plugin()->data().id < b->plugin()->data().id;
}

std::vector<plugin::ActionService *>::iterator plugin::PluginActionRegistry::find(plugin::ActionService* as)
{
    auto it = std::lower_bound(enabled_actions.begin(), enabled_actions.end(), as, &PluginActionRegistry::compare);
    return it;
}

QIcon plugin::ActionService::service_icon() const
{
    return plugin()->make_icon(icon);
}


void plugin::ActionService::trigger() const
{
    QVariantMap settings_value;
    if ( !script.settings.empty() )
    {
        if ( !app::settings::WidgetBuilder().show_dialog(
            script.settings, settings_value, plugin()->data().name
        ) )
            return;
    }

    plugin()->run_script(script, {
        PluginRegistry::instance().global_parameter("window"),
        PluginRegistry::instance().global_parameter("document"),
        settings_value
    });
}

