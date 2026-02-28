/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
// #include "qpainter_renderer.hpp"
#include "thorvg_renderer.hpp"

std::unique_ptr<glaxnimate::renderer::Renderer> glaxnimate::renderer::default_renderer(int quality)
{
    Q_UNUSED(quality);
    // if ( quality == 0 )
        // return std::make_unique<glaxnimate::renderer::QPainterRenderer>();
    return std::make_unique<glaxnimate::renderer::ThorvgRenderer>();
}
