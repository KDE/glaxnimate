/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <KConfigSkeleton>
#include "settings/custom_settings_group.hpp"

namespace glaxnimate::gui::settings {

class GlaxnimateSettingsBase : public KConfigSkeleton
{
public:
    const std::vector<std::unique_ptr<CustomSettingsGroup>>& custom_groups() const { return groups_; }

    void add_group(std::unique_ptr<CustomSettingsGroup> group)
    {
        auto slug = group->slug();
        group->load(*config());
        groups_.push_back(std::move(group));
    }

private:
    std::vector<std::unique_ptr<CustomSettingsGroup>> groups_;
};

} // namespace glaxnimate::gui::settings
