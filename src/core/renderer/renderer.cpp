/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "cairo_renderer.hpp"

std::unique_ptr<glaxnimate::renderer::Renderer> glaxnimate::renderer::default_renderer(int quality)
{
    return std::make_unique<glaxnimate::renderer::CairoRenderer>(quality);
}

