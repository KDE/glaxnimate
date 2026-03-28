/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "video_module.hpp"
#include "video_format.hpp"

std::vector<glaxnimate::module::ExternalComponent> glaxnimate::video::Module::components() const
{
    return {
        {i18n("libav"), {}, video::VideoFormat::library_version(), QStringLiteral("https://libav.org/"), "LGPL"}
    };
}

void glaxnimate::video::Module::initialize()
{
    register_io_classes<video::VideoFormat>();
}
