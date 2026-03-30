/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
// #include "glaxnimate/qpainter_renderer.hpp"
#include "glaxnimate/renderer/renderer.hpp"

std::unique_ptr<glaxnimate::renderer::Renderer> glaxnimate::renderer::RendererRegistry::default_renderer(int quality)
{
    // if ( quality == 0 )
        // return std::make_unique<glaxnimate::renderer::QPainterRenderer>();
    return (*default_factory)(quality);
}


std::unique_ptr<glaxnimate::renderer::Renderer> glaxnimate::renderer::RendererRegistry::factory_build(const QString &id, int quality)
{
    auto it = factory_map.find(id);
    if ( it == factory_map.end() )
        return {};
    return it->second(quality);
}

void glaxnimate::renderer::RendererRegistry::register_factory(const QString &id, FactoryFunction func, bool make_default)
{
    auto it = factory_map.emplace(id, std::move(func)).first;
    if ( make_default || !default_factory )
        default_factory = &it->second;
}

const std::map<QString, glaxnimate::renderer::RendererRegistry::FactoryFunction> &glaxnimate::renderer::RendererRegistry::factories()
{
    return factory_map;
}

glaxnimate::renderer::RendererRegistry &glaxnimate::renderer::RendererRegistry::instance()
{
    static RendererRegistry registry;
    return registry;
}
