/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/module/postscript/ps_loader.hpp"
#include "glaxnimate/model/shapes/shapes/path.hpp"
#include "glaxnimate/model/shapes/shapes/text.hpp"
#include "glaxnimate/model/shapes/style/fill.hpp"
#include "glaxnimate/model/shapes/composable/image.hpp"


using namespace glaxnimate::ps;


PsToGlaxnimate::PsToGlaxnimate(model::Document *document)
    : document(document)
{
    new_comp();
}

void PsToGlaxnimate::apply_metadata() const
{
    if ( !last_comp_used && document->assets()->compositions->values.size() > 1 )
        document->assets()->compositions->values.remove(document->assets()->compositions->values.index_of(comp));
}

glaxnimate::model::Composition *PsToGlaxnimate::current_page()
{
    use_page();
    return comp;
}

void PsToGlaxnimate::on_comment(const QByteArray &text)
{
    if ( text == "%EndPageSetup" )
        use_page();
}

void PsToGlaxnimate::on_meta_comment(const QByteArray& key, const QByteArray& value)
{
    if ( key == "GlaxnimateObjectName" )
    {
        object_name = QString::fromUtf8(value);
    }
}

void PsToGlaxnimate::on_fill(const GraphicsState &gstate, bool evenodd)
{
    use_page();

    auto group = comp->shapes.create<model::Group>();
    if ( !object_name.isEmpty() )
        group->name.set(object_name);

    auto fill = group->shapes.create<model::Fill>();
    fill->color.set(gstate.color);
    fill->fill_rule.set(evenodd ? model::Fill::EvenOdd : model::Fill::NonZero);

    for ( const auto& bez : gstate.path )
    {
        auto shape = group->shapes.create<model::Path>();
        shape->shape.set(convert(bez));
    }
}

void PsToGlaxnimate::on_stroke(const GraphicsState &gstate)
{
    use_page();

    auto group = comp->shapes.create<model::Group>();
    if ( !object_name.isEmpty() )
        group->name.set(object_name);

    auto stroke = group->shapes.create<model::Stroke>();
    stroke->color.set(gstate.color);
    stroke->width.set(gstate.line_width);
    stroke->cap.set(gstate.line_cap);
    stroke->join.set(gstate.line_join);
    stroke->miter_limit.set(gstate.miter_limit);

    for ( const auto& bez : gstate.path )
    {
        auto shape = group->shapes.create<model::Path>();
        shape->shape.set(convert(bez));
    }
}

void PsToGlaxnimate::on_show_page(bool copy)
{
    if ( copy )
    {
        use_page();
        auto comp_clone = comp->clone_covariant();
        on_comp_finished(comp);
        comp = comp_clone.get();
        last_comp_used = false;
        document->assets()->compositions->values.insert(std::move(comp_clone));
    }
    else
    {
        use_page();
        on_comp_finished(comp);
        new_comp();
    }
}

void PsToGlaxnimate::on_comp_finished(model::Composition *)
{

}

void PsToGlaxnimate::on_image(const ImageData &image, const GraphicsState &gstate)
{
    use_page();

    auto bitmap = std::make_unique<glaxnimate::model::Bitmap>(document);
    bitmap->set_pixmap(image.image, "png");
    auto asset = bitmap.get();
    document->assets()->images->values.insert(std::move(bitmap));
    auto shape = comp->shapes.create<model::Image>();
    shape->image.set(asset);

    // postscript coordinates
    // TODO apply to the whole comp instead of converting coords?
    QTransform device_tf;
    device_tf.scale(1, -1);
    device_tf.translate(0, -comp->height.get());

    shape->transform->set_transform_matrix(convert(image.matrix.inverted() * gstate.transform));
}

void PsToGlaxnimate::on_draw_text(const TextDrawOptions &options, const GraphicsState &gstate)
{
    use_page();

    auto group = comp->shapes.create<model::Group>();
    if ( !object_name.isEmpty() )
        group->name.set(object_name);

    auto fill = group->shapes.create<model::Fill>();
    fill->color.set(gstate.color);

    auto text = group->shapes.create<model::TextShape>();
    text->text.set(options.text);
    text->position.set(convert(gstate.position()));
    text->font->family.set(gstate.font.family());
    text->font->size.set(gstate.font.size());
    text->font->style.set(gstate.font.style());

    group->transform->set_transform_matrix(convert(gstate.font.transform()));
}

QPointF PsToGlaxnimate::convert(const QPointF &p) const
{
    // return p;
    return QPointF(p.x(), comp->height.get() - p.y());
}

QTransform PsToGlaxnimate::convert(const QTransform &tf) const
{
    // postscript coordinates
    // TODO apply to the whole comp instead of converting coords?
    QTransform device_tf;
    device_tf.scale(1, -1);
    device_tf.translate(0, -comp->height.get());
    return tf * device_tf;
}

void PsToGlaxnimate::apply_page_metadata() const
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
        auto sizev = page_device().get("PageSize");
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

QRectF PsToGlaxnimate::parse_bounding_box(QStringView box) const
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

std::optional<QString> PsToGlaxnimate::get_page_meta(const QByteArray &page, const QByteArray &doc) const
{
    auto it = page_metadata().find(page);
    if ( it != page_metadata().end() )
        return it->second.cast<String>().bytes();

    it = document_metadata().find(doc);
    if ( it != document_metadata().end() )
        return it->second.cast<String>().bytes();

    return {};
}

void PsToGlaxnimate::new_comp()
{
    comp = document->assets()->add_comp_no_undo();
    comp->animation->last_frame.set(180);
    last_comp_used = false;
}

void PsToGlaxnimate::use_page()
{
    if ( !last_comp_used )
    {
        last_comp_used = true;
        apply_page_metadata();
    }
}

glaxnimate::math::bezier::Point PsToGlaxnimate::convert(const math::bezier::Point &pt) const
{
    return glaxnimate::math::bezier::Point(
        convert(pt.pos),
        convert(pt.tan_in),
        convert(pt.tan_out)
    );
}

glaxnimate::math::bezier::Bezier PsToGlaxnimate::convert(const math::bezier::Bezier &bez) const
{
    auto copy = bez;
    for ( auto& pt : copy )
        pt = convert(pt);
    return copy;
}

Loader::Loader(io::ImportExport *importer, model::Document *document, const QVariantMap &)
    : PsToGlaxnimate(document), importer(importer)
{

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
