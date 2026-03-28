/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/module/cairo/pdf_format.hpp"

#include <cairo/cairo-pdf.h>

#include "glaxnimate/model/assets/composition.hpp"
#include "glaxnimate/app_info.hpp"
#include "cairo_renderer.hpp"


QStringList glaxnimate::cairo::PdfFormat::extensions(Direction) const
{
    return {QStringLiteral("pdf")};
}

static cairo_status_t write_to_io(void *closure, const unsigned char *data, unsigned int length)
{
    ((QIODevice*)closure)->write((const char*)data, length);
    return CAIRO_STATUS_SUCCESS;
}

static void set_metadata(cairo_surface_t *surface, cairo_pdf_metadata_t metadata, const QString& string)
{
    if ( !string.isEmpty() )
        cairo_pdf_surface_set_metadata(surface, metadata, string.toStdString().c_str());
}


bool glaxnimate::cairo::PdfFormat::on_save_static(QIODevice &dev, const QString &, model::Composition *comp, model::FrameTime time, const QVariantMap &)
{
    CairoRenderer renderer(10);
    auto surface = cairo_pdf_surface_create_for_stream(&write_to_io, &dev, comp->width.get(), comp->height.get());
    if ( cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS )
    {
        error(i18n("Could not create PDF surface: %1", QString(cairo_status_to_string(cairo_surface_status(surface)))));
        return false;
    }

    // Metadata
    set_metadata(surface, CAIRO_PDF_METADATA_TITLE, comp->name.get());
    set_metadata(surface, CAIRO_PDF_METADATA_AUTHOR, comp->document()->info().author);
    set_metadata(surface, CAIRO_PDF_METADATA_SUBJECT, comp->document()->info().description);
    set_metadata(surface, CAIRO_PDF_METADATA_KEYWORDS, comp->document()->info().keywords.join(' '));
    auto& info = glaxnimate::AppInfo::instance();
    set_metadata(surface, CAIRO_PDF_METADATA_CREATOR, QStringLiteral("%1 %2").arg(info.name()).arg(info.version()));


    renderer.set_cairo_surface(surface, comp->width.get(), comp->height.get());
    renderer.render_start();
    comp->paint(&renderer, time, model::VisualNode::Render);
    renderer.render_end();
    return true;
}

std::unique_ptr<glaxnimate::settings::SettingsGroup> glaxnimate::cairo::PdfFormat::save_settings(model::Composition*) const
{
    return {};
}
