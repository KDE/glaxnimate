/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "extraformats_module.hpp"

#include "aep/aep_format.hpp"
#include "rive/rive_format.hpp"
#include "avd/avd_format.hpp"


using namespace glaxnimate::io;

std::vector<glaxnimate::module::ExternalComponent> glaxnimate::extraformats::Module::components() const
{
    return {
    };
}

void glaxnimate::extraformats::Module::initialize()
{
    register_io_classes<
        aep::AepFormat,
        aep::AepxFormat,
        avd::AvdFormat,
        rive::RiveFormat
    >();
}
