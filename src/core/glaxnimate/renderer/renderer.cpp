/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
// #include "glaxnimate/qpainter_renderer.hpp"
#include "glaxnimate/renderer/thorvg_renderer.hpp"

std::unique_ptr<glaxnimate::renderer::Renderer> glaxnimate::renderer::default_renderer(int quality)
{
    // if ( quality == 0 )
        // return std::make_unique<glaxnimate::renderer::QPainterRenderer>();
    return std::make_unique<glaxnimate::renderer::ThorvgRenderer>(quality);
}


std::map<QString, glaxnimate::renderer::Renderer::FactoryFunction> &glaxnimate::renderer::Renderer::factory()
{
    static std::map<QString, glaxnimate::renderer::Renderer::FactoryFunction> factory;
    return factory;
}

std::unique_ptr<glaxnimate::renderer::Renderer> glaxnimate::renderer::Renderer::factory_build(const QString &id, int quality)
{
    auto it = factory().find(id);
    if ( it == factory().end() )
        return {};
    return it->second(quality);
}

void glaxnimate::renderer::Renderer::register_factory(const QString &id, FactoryFunction func)
{
    factory().emplace(id, std::move(func));
}

const std::map<QString, glaxnimate::renderer::Renderer::FactoryFunction> &glaxnimate::renderer::Renderer::factory_registry()
{
    return factory();
}
