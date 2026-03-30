/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "thorvg_module.hpp"
#include "thorvg_renderer.hpp"

using namespace glaxnimate::thorvg;

std::vector<glaxnimate::module::ExternalComponent> glaxnimate::thorvg::Module::components() const
{
    return {
        {QStringLiteral("thorvg"), {},
            QStringLiteral("%1.%2.%3")
                .arg(TVG_VERSION_MAJOR)
                .arg(TVG_VERSION_MINOR)
                .arg(TVG_VERSION_MICRO),
        {}, "MIT"}
    };
}

void glaxnimate::thorvg::Module::initialize()
{
    renderer::RendererRegistry::instance().register_factory(QStringLiteral("ThorVG"), [](int q){ return std::make_unique<ThorvgRenderer>(q); }, true);
}
