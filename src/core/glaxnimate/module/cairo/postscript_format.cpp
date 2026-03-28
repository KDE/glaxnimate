/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/module/cairo/postscript_format.hpp"

#include <cairo/cairo-ps.h>

#include "glaxnimate/model/assets/composition.hpp"
#include "cairo_renderer.hpp"
#include "glaxnimate/app_info.hpp"


QStringList glaxnimate::cairo::PostScriptFormat::extensions(Direction) const
{
    return {QStringLiteral("ps"), QStringLiteral("eps")};
}

cairo_status_t write_to_io(void *closure, const unsigned char *data, unsigned int length)
{
    ((QIODevice*)closure)->write((const char*)data, length);
    return CAIRO_STATUS_SUCCESS;
}

static void add_comment(cairo_surface_t *surface, const QString& string)
{
    cairo_ps_surface_dsc_comment(surface, string.toStdString().c_str());
}

bool glaxnimate::cairo::PostScriptFormat::on_save_static(QIODevice &dev, const QString &, model::Composition *comp, model::FrameTime time, const QVariantMap &)
{
    CairoRenderer renderer(10);
    auto surface = cairo_ps_surface_create_for_stream(&write_to_io, &dev, comp->width.get(), comp->height.get());
    if ( cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS )
    {
        error(i18n("Could not create PostScript surface: %1", QString(cairo_status_to_string(cairo_surface_status(surface)))));
        return false;
    }

    // EPS
    cairo_ps_surface_set_eps(surface, true);
    add_comment(surface, QStringLiteral("%%BoundingBox: 0 0 %1 %2").arg(comp->width.get()).arg(comp->height.get()));
    auto& info = glaxnimate::AppInfo::instance();
    add_comment(surface, QStringLiteral("%%Creator: %1 %2").arg(info.name()).arg(info.version()));
    add_comment(surface, QStringLiteral("%%Title: %1").arg(comp->name.get()));

    renderer.set_cairo_surface(surface, comp->width.get(), comp->height.get());
    renderer.render_start();
    comp->paint(&renderer, time, model::VisualNode::Render);
    renderer.render_end();
    return true;
}

std::unique_ptr<glaxnimate::settings::SettingsGroup> glaxnimate::cairo::PostScriptFormat::save_settings(model::Composition*) const
{
    // TODO level, embedded?
    return {};
}
