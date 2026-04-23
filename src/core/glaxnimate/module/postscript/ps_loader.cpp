/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/module/postscript/ps_loader.hpp"
#include "glaxnimate/model/shapes/shapes/path.hpp"

#include <glaxnimate/model/shapes/style/fill.hpp>

using namespace glaxnimate::ps;

Loader::Loader(io::ImportExport *importer, model::Document *document, const QVariantMap &)
    : importer(importer), document(document)
{
    comp = document->assets()->add_comp_no_undo();
}

bool Loader::success() const
{
    return !has_error;
}

void Loader::on_print(const QString& text)
{
    importer->information(text);
}

void Loader::on_warning(const QString &text)
{
    importer->warning(text);
}

void Loader::on_error(const QString &text)
{
    importer->error(text);
    has_error = true;
}

void Loader::on_comment(const QString &)
{
}

void Loader::on_fill(const GraphicsState &gstate)
{
    auto group = comp->shapes.create<model::Group>();

    for ( const auto& bez : gstate.path )
    {
        auto shape = group->shapes.create<model::Path>();
        shape->shape.set(convert(bez));
    }

    auto fill = group->shapes.create<model::Fill>();
    fill->color.set(gstate.color);
}

QPointF Loader::convert(const QPointF &p) const
{
    return QPointF(p.x(), comp->height.get() - p.y());
}

glaxnimate::math::bezier::Point Loader::convert(const math::bezier::Point &pt) const
{
    return glaxnimate::math::bezier::Point(
        convert(pt.pos),
        convert(pt.tan_in),
        convert(pt.tan_out)
    );
}

glaxnimate::math::bezier::Bezier Loader::convert(const math::bezier::Bezier &bez) const
{
    auto copy = bez;
    for ( auto& pt : copy )
        pt = convert(pt);
    return copy;
}


