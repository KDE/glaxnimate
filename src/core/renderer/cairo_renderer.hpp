/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <cairo/cairo.h>

#include "utils/maybe_ptr.hpp"
#include "renderer/renderer.hpp"

namespace glaxnimate::renderer {

/**
 * \brief Renderer that uses cairo
 */
class CairoRenderer : public Renderer
{
private:
    int quality;
    int width = 0;
    int height = 0;
    Fill fill;
    Stroke stroke;
    int mode = ShapeMode::NothingMode;
    QImage* target = nullptr;
    cairo_surface_t* surface = nullptr;
    cairo_t* canvas = nullptr;
    std::vector<qreal> layer_alpha;
    cairo_pattern_t *pattern = nullptr;

    void set_brush(const QBrush& brush, qreal opacity)
    {
        auto gradient = brush.gradient();
        if ( !gradient )
        {
            auto color = brush.color();
            cairo_set_source_rgba(canvas, color.redF(), color.greenF(), color.blueF(), color.alphaF() * opacity);
            return;
        }

        if ( gradient->type() == QGradient::RadialGradient )
        {
            const QRadialGradient* rad = static_cast<const QRadialGradient*>(gradient);
            pattern = cairo_pattern_create_radial(
                rad->center().x(),
                rad->center().y(),
                rad->centerRadius(),
                rad->focalPoint().x(),
                rad->focalPoint().y(),
                rad->focalRadius()
            );
        }
        else if ( gradient->type() == QGradient::LinearGradient )
        {
            const QLinearGradient* lin = static_cast<const QLinearGradient*>(gradient);
            pattern = cairo_pattern_create_linear(
                lin->start().x(),
                lin->start().y(),
                lin->finalStop().x(),
                lin->finalStop().y()
            );
        }

        if ( !pattern )
            return;

        for ( const auto& stop : gradient->stops() )
        {
            const auto& color = stop.second;
            cairo_pattern_add_color_stop_rgba(pattern, stop.first, color.redF(), color.greenF(), color.blueF(), color.alphaF() * opacity);
        }
        cairo_set_source(canvas, pattern);
    }

    void draw_path(const QPainterPath& path)
    {
        cairo_new_path(canvas);

        int count = path.elementCount();
        for ( int i = 0; i < count; i++ )
        {
            auto el = path.elementAt(i);
            switch (el.type)
            {
                case QPainterPath::MoveToElement:
                    cairo_move_to(canvas, el.x, el.y);
                    break;

                case QPainterPath::LineToElement:
                    cairo_line_to(canvas, el.x, el.y);
                    break;

                case QPainterPath::CurveToElement:
                {
                    const QPainterPath::Element& cp2 = path.elementAt(i + 1);
                    const QPainterPath::Element& end = path.elementAt(i + 2);
                    cairo_curve_to(canvas, el.x, el.y, cp2.x, cp2.y, end.x, end.y);

                    i += 2;
                    break;
                }

                default:
                    break;
            }
        }
    }

public:
    CairoRenderer() : CairoRenderer(5) {}
    explicit CairoRenderer(int quality) : quality(quality)
    {
    }

    void set_image_surface(QImage * destination) override
    {
        target = destination;
        width = target->width();
        height = target->height();
        surface = cairo_image_surface_create_for_data((unsigned char*)target->bits(), CAIRO_FORMAT_ARGB32, target->width(), target->height(), target->bytesPerLine());
        canvas = cairo_create(surface);
    }

    void render_start() override
    {
        cairo_antialias_t anti;
        if ( quality < 1 ) anti = CAIRO_ANTIALIAS_NONE;
        else if ( quality < 4 ) anti = CAIRO_ANTIALIAS_FAST;
        else if ( quality < 8 ) anti = CAIRO_ANTIALIAS_GOOD;
        else anti = CAIRO_ANTIALIAS_BEST;

        cairo_set_antialias(canvas, anti);
        layer_alpha.push_back(1);
    }

    void render_end() override
    {
        layer_alpha.clear();
        cairo_destroy(canvas);
        cairo_surface_destroy(surface);
    }

    void set_fill(const Fill & fill) override
    {
        this->fill = fill;
        mode |= FillMode;
    }

    void set_stroke(const Stroke & stroke) override
    {
        this->stroke = stroke;
        mode |= StrokeMode;
    }

    void draw_path(const math::bezier::MultiBezier & bez) override
    {
        QPainterPath path = bez.painter_path();
        if ( mode & FillMode )
        {
            set_brush(fill.brush, fill.opacity);
            draw_path(path);
            cairo_set_fill_rule(canvas, fill.rule == Qt::OddEvenFill ? CAIRO_FILL_RULE_EVEN_ODD : CAIRO_FILL_RULE_WINDING);
            cairo_fill(canvas);
            if ( pattern )
            {
                cairo_pattern_destroy(pattern);
                pattern = nullptr;
            }
        }

        if ( mode & StrokeMode )
        {
            set_brush(stroke.pen.brush(), stroke.opacity);
            cairo_set_line_width(canvas, stroke.pen.width());
            auto qcap = stroke.pen.capStyle();
            auto cap = qcap == Qt::RoundCap ?
                CAIRO_LINE_CAP_ROUND :
                (qcap == Qt::SquareCap ?
                    CAIRO_LINE_CAP_SQUARE :
                    CAIRO_LINE_CAP_BUTT
                )
            ;
            cairo_set_line_cap(canvas, cap);
            auto qjoin = stroke.pen.joinStyle();
            auto join = qjoin == Qt::RoundJoin ?
                CAIRO_LINE_JOIN_ROUND :
                (qjoin == Qt::MiterJoin ?
                    CAIRO_LINE_JOIN_MITER :
                    CAIRO_LINE_JOIN_BEVEL
                )
            ;
            cairo_set_line_join(canvas, join);
            cairo_set_miter_limit(canvas, stroke.pen.miterLimit());
            draw_path(path);
            cairo_stroke(canvas);
            if ( pattern )
            {
                cairo_pattern_destroy(pattern);
                pattern = nullptr;
            }
        }

        mode = NothingMode;
    }


    void layer_start() override
    {
        layer_alpha.push_back(1.);
        cairo_push_group(canvas);
    }

    void layer_end() override
    {
        qreal aplha = layer_alpha.back();
        layer_alpha.pop_back();
        cairo_pop_group_to_source(canvas);
        cairo_paint_with_alpha(canvas, aplha);
    }

    void set_opacity(qreal opacity) override
    {
        layer_alpha.back() = opacity;
    }

    qreal opacity() const override
    {
        return layer_alpha.back();
    }

    void clip_rect(const QRectF & rect, Qt::ClipOperation op = Qt::ReplaceClip) override
    {
        if ( op == Qt::ReplaceClip )
            cairo_reset_clip(canvas);
        cairo_rectangle(canvas, rect.left(), rect.top(), rect.width(), rect.height());
        cairo_clip(canvas);
    }

    void clip_path(const QPainterPath & path, Qt::ClipOperation op = Qt::ReplaceClip) override
    {
        if ( op == Qt::ReplaceClip )
            cairo_reset_clip(canvas);
        draw_path(path);
        cairo_clip(canvas);
    }

    void draw_pixmap(const QPixmap & pixmap) override
    {
        QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);

        cairo_surface_t *surface = cairo_image_surface_create_for_data(
            image.bits(),
            CAIRO_FORMAT_ARGB32,
            image.width(),
            image.height(),
            image.bytesPerLine()
        );

        cairo_set_source_surface(canvas, surface, 0, 0);
        cairo_paint(canvas);
        cairo_surface_destroy(surface);
    }

    void scale(qreal x, qreal y) override
    {
        cairo_scale(canvas, x, y);
    }

    void translate(qreal x, qreal y) override
    {
        cairo_translate(canvas, x, y);
    }

    void transform(const QTransform & matrix) override
    {
        cairo_matrix_t cm {
            matrix.m11(), matrix.m12(),
            matrix.m21(), matrix.m22(),
            matrix.dx(), matrix.dy()
        };
        cairo_transform(canvas, &cm);
    }
};

} // namespace glaxnimate::renderer

