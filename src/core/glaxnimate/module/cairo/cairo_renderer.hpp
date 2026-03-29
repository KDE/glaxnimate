/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <cairo/cairo.h>

#include "glaxnimate/renderer/renderer.hpp"

namespace glaxnimate::cairo {
using namespace glaxnimate::renderer;

/**
 * \brief Renderer that uses cairo
 */
class CairoRenderer : public renderer::Renderer
{
private:
    int quality;
    int width = 0;
    int height = 0;
    renderer::Fill fill;
    renderer::Stroke stroke;
    int mode = renderer::ShapeMode::NothingMode;
    QImage* target = nullptr;
    cairo_surface_t* surface = nullptr;
    cairo_t* canvas = nullptr;

    struct LayerData
    {
        qreal alpha = 1;
        cairo_operator_t blend_mode = CAIRO_OPERATOR_OVER;

        cairo_surface_t* mask_surface = nullptr;
        cairo_t* mask_canvas = nullptr;
    };

    std::vector<LayerData> layer_data;
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

    void create_path(const math::bezier::MultiBezier& path)
    {
        cairo_new_path(canvas);

        for ( const auto& bez : path.beziers() )
        {
            if ( bez.size() == 0 )
                continue;

            cairo_move_to(canvas, bez[0].pos.x(), bez[0].pos.y());
            for ( int i = 0; i < bez.size() - 1; i++ )
            {
                const auto& before = bez[i];
                const auto& after = bez[i+1];
                cairo_curve_to(
                    canvas,
                    before.tan_out.x(), before.tan_out.y(),
                    after.tan_in.x(), after.tan_in.y(),
                    after.pos.x(), after.pos.y()
                );
            }

            if ( bez.closed() && bez.size() > 1 )
            {
                const auto& before = bez.back();
                const auto& after = bez[0];
                cairo_curve_to(
                    canvas,
                    before.tan_out.x(), before.tan_out.y(),
                    after.tan_in.x(), after.tan_in.y(),
                    after.pos.x(), after.pos.y()
                );
                cairo_close_path(canvas);
            }
        }
    }

    cairo_operator_t convert_blend_mode(BlendMode mode) const
    {
        switch ( mode )
        {
            case renderer::BlendMode::Normal: return CAIRO_OPERATOR_OVER;
            case renderer::BlendMode::Add: return CAIRO_OPERATOR_ADD;
            case renderer::BlendMode::Color: return CAIRO_OPERATOR_HSL_COLOR;
            case renderer::BlendMode::ColorBurn: return CAIRO_OPERATOR_COLOR_BURN;
            case renderer::BlendMode::ColorDodge: return CAIRO_OPERATOR_COLOR_DODGE;
            case renderer::BlendMode::Darken: return CAIRO_OPERATOR_DARKEN;
            case renderer::BlendMode::Difference: return CAIRO_OPERATOR_DIFFERENCE;
            case renderer::BlendMode::Exclusion: return CAIRO_OPERATOR_EXCLUSION;
            case renderer::BlendMode::HardLight: return CAIRO_OPERATOR_HARD_LIGHT;
            case renderer::BlendMode::Hue: return CAIRO_OPERATOR_HSL_HUE;
            case renderer::BlendMode::Luminosity: return CAIRO_OPERATOR_HSL_LUMINOSITY;
            case renderer::BlendMode::Lighten: return CAIRO_OPERATOR_LIGHTEN;
            case renderer::BlendMode::Multiply: return CAIRO_OPERATOR_MULTIPLY;
            case renderer::BlendMode::Overlay: return CAIRO_OPERATOR_OVERLAY;
            case renderer::BlendMode::Saturation: return CAIRO_OPERATOR_HSL_SATURATION;
            case renderer::BlendMode::Screen: return CAIRO_OPERATOR_SCREEN;
            case renderer::BlendMode::SoftLight: return CAIRO_OPERATOR_SOFT_LIGHT;
        }
        return CAIRO_OPERATOR_OVER;
    }

public:
    CairoRenderer() : CairoRenderer(5) {}
    explicit CairoRenderer(int quality) : quality(quality)
    {
    }

    int supported_surfaces() const override { return 0; }

    void set_cairo_surface(cairo_surface_t* surface, int width, int height)
    {
        this->width = width;
        this->height = height;
        this->surface = surface;
        canvas = cairo_create(surface);
    }

    void set_image_surface(QImage * destination) override
    {
        target = destination;
        width = target->width();
        height = target->height();
        surface = cairo_image_surface_create_for_data((unsigned char*)target->bits(), CAIRO_FORMAT_ARGB32, target->width(), target->height(), target->bytesPerLine());
        canvas = cairo_create(surface);
    }

    bool set_gl_surface(void *, int, int, int) override { return false; }
    bool set_painter_surface(QPainter *, int, int) override  { return false; }

    void render_start() override
    {
        cairo_antialias_t anti;
        if ( quality < 1 ) anti = CAIRO_ANTIALIAS_NONE;
        else if ( quality < 4 ) anti = CAIRO_ANTIALIAS_FAST;
        else if ( quality < 8 ) anti = CAIRO_ANTIALIAS_GOOD;
        else anti = CAIRO_ANTIALIAS_BEST;

        cairo_set_antialias(canvas, anti);
        layer_data.push_back({});
    }

    void render_end() override
    {
        layer_data.clear();
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

    void draw_path(const math::bezier::MultiBezier & path) override
    {
        if ( mode & FillMode )
        {
            set_brush(fill.brush, fill.opacity);
            create_path(path);
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
            create_path(path);
            cairo_stroke(canvas);
            if ( pattern )
            {
                cairo_pattern_destroy(pattern);
                pattern = nullptr;
            }
        }

        mode = NothingMode;
    }

    // used by gl stuff which we don't support
    void fill_rect(const QRectF&, const QBrush&) override {}
    void fill_pattern(const QRectF&, const QImage&) override {}

    void layer_start() override
    {
        layer_data.push_back({});
        cairo_push_group(canvas);
    }

    void layer_end() override
    {
        auto data = layer_data.back();
        layer_data.pop_back();

        cairo_pop_group_to_source(canvas);
        cairo_set_operator(canvas, data.blend_mode);
        cairo_paint_with_alpha(canvas, data.alpha);

        if ( data.mask_surface )
        {
            cairo_mask_surface(canvas, data.mask_surface, 0, 0);
            cairo_destroy(data.mask_canvas);
            cairo_surface_destroy(data.mask_surface);
        }
    }

    void set_blend_mode(BlendMode mode) override
    {
        layer_data.back().blend_mode = convert_blend_mode(mode);
    }

    void mask_start(int) override
    {
        layer_data.back().mask_surface = surface;
        layer_data.back().mask_canvas = canvas;
        surface = cairo_surface_create_similar(surface, CAIRO_CONTENT_COLOR_ALPHA, width, height);
        canvas = cairo_create(surface);
    }

    void mask_end() override
    {
        // cairo_surface_flush(surface);
        std::swap(canvas, layer_data.back().mask_canvas);
        std::swap(surface, layer_data.back().mask_surface);
    }

    void set_opacity(qreal opacity) override
    {
        layer_data.back().alpha = opacity;
    }

    qreal opacity() const override
    {
        return layer_data.back().alpha;
    }

    void clip_rect(const QRectF & rect) override
    {
        cairo_rectangle(canvas, rect.left(), rect.top(), rect.width(), rect.height());
        cairo_clip(canvas);
    }


    void draw_image(const QImage & image) override
    {
        // QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);

        cairo_surface_t *surface = cairo_image_surface_create_for_data(
            const_cast<uchar*>(image.bits()),
            CAIRO_FORMAT_ARGB32,
            image.width(),
            image.height(),
            image.bytesPerLine()
            );

        cairo_set_source_surface(canvas, surface, 0, 0);
        cairo_paint(canvas);
        cairo_surface_destroy(surface);
    }

    void set_quality(int quality) override
    {
        this->quality = quality;
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

} // namespace glaxnimate::cairo
