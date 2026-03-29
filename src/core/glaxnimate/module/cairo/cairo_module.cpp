/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cairo_module.hpp"

#include <cairo/cairo.h>

#include "postscript_format.hpp"
#include "pdf_format.hpp"
#include "cairo_renderer.hpp"

std::vector<glaxnimate::module::ExternalComponent> glaxnimate::cairo::Module::components() const
{
    return {
        {i18n("cairo"), cairo_version_string(), {}, QStringLiteral("https://www.cairographics.org/"), "LGPL"}
    };
}

void glaxnimate::cairo::Module::initialize()
{
    register_io_classes<PostScriptFormat, PdfFormat>();
    renderer::Renderer::register_factory(QStringLiteral("Cairo"), [](int q){ return std::make_unique<CairoRenderer>(q); });
}
