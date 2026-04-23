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
    comp->name.set({});
}

bool Loader::success() const
{
    return !has_error;
}

void Loader::apply_metadata() const
{
    apply_page_metadata();
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

void Loader::apply_page_metadata() const
{
    if ( auto bbstr = get_page_meta("PageBoundingBox", "BoundingBox") )
    {
        QRectF bbox = parse_bounding_box(*bbstr);
        if ( bbox.isValid() )
        {
            comp->width.set(bbox.width());
            comp->height.set(bbox.height());
        }
    }
    else
    {
        auto sizev = page_metadata().get("PageSize");
        if ( sizev.type() == Value::Array )
        {
            auto size = sizev.cast<ValueArray>();

            if ( size.size() >= 2 )
            {
                comp->width.set(size[0].cast<float>());
                comp->height.set(size[1].cast<float>());
            }
        }
    }

    if ( auto title = get_page_meta("Page", "Title") )
    {
        comp->name.set(*title);
    }

}

QRectF Loader::parse_bounding_box(QStringView box) const
{
    auto chunks = box.split(' ');
    if ( chunks.size() != 4 )
        return {};

    static constexpr const int left = 0;
    static constexpr const int bottom = 1;
    static constexpr const int right = 2;
    static constexpr const int top = 3;

    // left bottom right top
    std::array<float, 4> coords;

    for ( int i = 0; i < 4; i++ )
    {
        bool ok;
        coords[i] = chunks[i].toFloat(&ok);
        if ( !ok )
            return {};
    }

    return QRectF(QPointF(coords[left], coords[top]), QPointF(coords[right], coords[bottom]));
}

std::optional<QString> Loader::get_page_meta(const QByteArray &page, const QByteArray &doc) const
{
    auto it = page_metadata().find(page);
    if ( it != page_metadata().end() )
        return it->second.cast<String>().bytes();

    it = document_metadata().find(doc);
    if ( it != document_metadata().end() )
        return it->second.cast<String>().bytes();

    return {};
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


