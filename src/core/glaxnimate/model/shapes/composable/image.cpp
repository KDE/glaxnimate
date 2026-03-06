/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/model/shapes/composable/image.hpp"
#include "glaxnimate/model/document.hpp"
#include "glaxnimate/model/assets/assets.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::Image)


bool glaxnimate::model::Image::is_valid_image(glaxnimate::model::DocumentNode* node) const
{
    return document()->assets()->images->values.is_valid_reference_value(node, false);
}

std::vector<glaxnimate::model::DocumentNode *> glaxnimate::model::Image::valid_images() const
{
    return document()->assets()->images->values.valid_reference_values(false);
}

QRectF glaxnimate::model::Image::local_bounding_rect(glaxnimate::model::FrameTime) const
{
    if ( !image.get() )
        return {};
    return QRectF(0, 0, image->width.get(), image->height.get());
}

void glaxnimate::model::Image::on_paint(renderer::Renderer* painter, glaxnimate::model::FrameTime time, glaxnimate::model::VisualNode::PaintMode mode, glaxnimate::model::Modifier* mod) const
{
    if ( image.get() )
    {
        Composable::on_paint(painter, time, mode, mod);
        image->paint(painter);
    }
}

void glaxnimate::model::Image::on_image_changed(glaxnimate::model::Bitmap* new_use, glaxnimate::model::Bitmap* old_use)
{
    if ( old_use )
    {
        disconnect(old_use, &Bitmap::loaded, this, &Image::on_update_image);
    }

    if ( new_use )
    {
        connect(new_use, &Bitmap::loaded, this, &Image::on_update_image);
    }
}

void glaxnimate::model::Image::on_update_image()
{
    Q_EMIT property_changed(&image, {});
}

QTransform glaxnimate::model::Image::local_transform_matrix(glaxnimate::model::FrameTime t) const
{
    return transform->transform_matrix(t);
}

void glaxnimate::model::Image::add_shapes(FrameTime, math::bezier::MultiBezier&, const QTransform&) const
{
}

QIcon glaxnimate::model::Image::tree_icon() const
{
    return QIcon::fromTheme("x-shape-image");
}

QString glaxnimate::model::Image::type_name_human() const
{
    return i18n("Image");
}

glaxnimate::math::bezier::MultiBezier glaxnimate::model::Image::to_painter_path_impl(FrameTime time) const
{
    auto trans = transform.get()->transform_matrix(time);
    glaxnimate::math::bezier::MultiBezier p;
    p.append(trans.map(QRectF(QPointF(0, 0), image.get() ? image->pixmap().size() : QSize(0, 0))));
    return p;
}
