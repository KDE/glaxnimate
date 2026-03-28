/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "glaxnimate/module/module.hpp"

namespace glaxnimate::video {

class Module : public module::Module
{
public:
    Module() : module::Module(i18n("Video Format Support")) {}

    std::vector<module::ExternalComponent> components() const override;

protected:
    void initialize() override;
};

} // namespace glaxnimate::video
