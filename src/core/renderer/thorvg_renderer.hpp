/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <thorvg.h>

#include "utils/maybe_ptr.hpp"
#include "renderer/renderer.hpp"

namespace glaxnimate::renderer {

/**
 * \brief Renderer that uses ThorVG
 */
class ThorvgRenderer : public Renderer
{
private:
    Fill fill;
    Stroke stroke;
    int mode = ShapeMode::NothingMode;

    std::unique_ptr<tvg::Canvas> canvas;
    std::vector<tvg::Scene*> layers;

    /*void set_brush(const QBrush& brush, qreal opacity)
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
    }*/

    void draw_path(const QPainterPath& path, tvg::Shape* shape)
    {
        int count = path.elementCount();
        for ( int i = 0; i < count; i++ )
        {
            auto el = path.elementAt(i);
            switch (el.type)
            {
                case QPainterPath::MoveToElement:
                    shape->moveTo(el.x, el.y);
                    break;

                case QPainterPath::LineToElement:
                    shape->lineTo(el.x, el.y);
                    break;

                case QPainterPath::CurveToElement:
                {
                    const QPainterPath::Element& cp2 = path.elementAt(i + 1);
                    const QPainterPath::Element& end = path.elementAt(i + 2);
                    shape->cubicTo(el.x, el.y, cp2.x, cp2.y, end.x, end.y);

                    i += 2;
                    break;
                }

                default:
                    break;
            }
        }
    }

public:
    ThorvgRenderer()
    {
        // 4 threads
        tvg::Initializer::init(4);
    }

    ~ThorvgRenderer()
    {
        tvg::Initializer::term();
    }

    void set_image_surface(QImage * destination) override
    {
        auto sw_canvas = tvg::SwCanvas::gen();
        sw_canvas->target(
            reinterpret_cast<uint32_t*>(destination->bits()),
            // destination->bytesPerLine(),
            destination->width(),
            destination->width(),
            destination->height(),
            tvg::ColorSpace::ABGR8888
        );
        canvas.reset(sw_canvas);
    }

    bool set_gl_surface(void * context, int framebuffer, int width, int height) override
    {
        auto gl_canvas = tvg::GlCanvas::gen();
        if ( !gl_canvas )
            return false;
        canvas.reset(gl_canvas);
        return gl_canvas->target(nullptr, nullptr, context, framebuffer, width, height, tvg::ColorSpace::ABGR8888S) == tvg::Result::Success;
    }

    void render_start() override
    {
        layer_start();
    }

    void render_end() override
    {
        canvas->add(layers[0]);
        layers.clear();
        canvas->draw(true);
        canvas->sync();
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
        if ( !mode )
            return;

        QPainterPath path = bez.painter_path();
        auto shape = tvg::Shape::gen();
        draw_path(path, shape);

        if ( mode & FillMode )
        {
            // set_brush(fill.brush, fill.opacity);
            QColor col = fill.brush.color();
            shape->fill(col.red(), col.green(), col.blue(), col.alpha() * fill.opacity);
            shape->fillRule(fill.rule == Qt::OddEvenFill ? tvg::FillRule::EvenOdd : tvg::FillRule::NonZero);
        }

        if ( mode & StrokeMode )
        {
            // set_brush(stroke.pen.brush(), stroke.opacity);
            QColor col = stroke.pen.brush().color();
            shape->strokeFill(col.red(), col.green(), col.blue(), col.alpha() * stroke.opacity);

            shape->strokeWidth(stroke.pen.width());
            auto qcap = stroke.pen.capStyle();
            auto cap = qcap == Qt::RoundCap ?
                tvg::StrokeCap::Round :
                (qcap == Qt::SquareCap ?
                    tvg::StrokeCap::Square :
                    tvg::StrokeCap::Butt
                )
            ;
            shape->strokeCap(cap);
            auto qjoin = stroke.pen.joinStyle();
            auto join = qjoin == Qt::RoundJoin ?
                tvg::StrokeJoin::Round :
                (qjoin == Qt::MiterJoin ?
                    tvg::StrokeJoin::Miter :
                    tvg::StrokeJoin::Bevel
                )
            ;
            shape->strokeJoin(join);
            shape->strokeMiterlimit(stroke.pen.miterLimit());
        }

        layers.back()->add(shape);

        mode = NothingMode;
    }


    void layer_start() override
    {
        layers.push_back(tvg::Scene::gen());
    }

    void layer_end() override
    {
        auto scene = layers.back();
        layers.pop_back();
        layers.back()->add(scene);
    }

    void set_opacity(qreal opacity) override
    {
        layers.back()->opacity(opacity * 255);
    }

    qreal opacity() const override
    {
        return layers.back()->opacity() / 255.;
    }

    void clip_rect(const QRectF & rect, Qt::ClipOperation op = Qt::ReplaceClip) override
    {
        auto shape = tvg::Shape::gen();
        shape->appendRect(rect.left(), rect.top(), rect.width(), rect.height(), 0, 0);
        // TODO if ( op == Qt::ReplaceClip )
        layers.back()->clip(shape);
    }

    void clip_path(const QPainterPath & path, Qt::ClipOperation op = Qt::ReplaceClip) override
    {
        // TODO if ( op == Qt::ReplaceClip )
        auto shape = tvg::Shape::gen();
        draw_path(path, shape);
        layers.back()->clip(shape);
    }

    void draw_pixmap(const QPixmap & pixmap) override
    {
        /*QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);

        cairo_surface_t *surface = cairo_image_surface_create_for_data(
            image.bits(),
            CAIRO_FORMAT_ARGB32,
            image.width(),
            image.height(),
            image.bytesPerLine()
        );

        cairo_set_source_surface(canvas, surface, 0, 0);
        cairo_paint(canvas);
        cairo_surface_destroy(surface);*/
    }

    void scale(qreal x, qreal y) override
    {
        layers.back()->scale(x);
    }

    void translate(qreal x, qreal y) override
    {
        layers.back()->translate(x, y);
    }

    void transform(const QTransform & matrix) override
    {
        tvg::Matrix cm {
            float(matrix.m11()), float(matrix.m21()), float(matrix.m31()),
            float(matrix.m12()), float(matrix.m22()), float(matrix.m32()),
            float(matrix.m13()), float(matrix.m23()), float(matrix.m33()),
        };
        layers.back()->transform(cm);
    }
};

} // namespace glaxnimate::renderer


