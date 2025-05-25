/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "settings/custom_settings_group.hpp"
#include "widgets/settings/plugin_settings_widget.hpp"
#include "plugin/plugin.hpp"

namespace glaxnimate::gui::settings {

class PluginSettingsGroup : public CustomSettingsGroup
{
public:
    PluginSettingsGroup(QStringList default_enabled)
    : enabled(std::move(default_enabled)) {}

    QString slug() const override { return "plugins"; }
    QString icon() const override { return QStringLiteral("system-software-install"); }
    KLazyLocalizedString label() const override { return kli18n("Plugins"); }
    void load ( KConfig & settings ) override
    {
        plugin::PluginRegistry::instance().load();

        auto group = settings.group(slug());
        enabled = group.readEntry("enabled", enabled);

        for ( const auto& plugin : plugin::PluginRegistry::instance().plugins() )
            if ( enabled.contains(plugin->data().id) )
                plugin->enable();
    }

    void save ( KConfig & settings ) override
    {
        enabled.clear();

        for ( const auto& plugin : plugin::PluginRegistry::instance().plugins() )
            if ( plugin->enabled() )
                enabled.push_back(plugin->data().id);

        auto group = settings.group(slug());
        group.writeEntry("enabled", enabled);
    }

    QWidget * make_widget ( QWidget * parent ) override { return new PluginSettingsWidget(parent); }

private:
    QStringList enabled;
};

} // namespace glaxnimate::gui::settings
