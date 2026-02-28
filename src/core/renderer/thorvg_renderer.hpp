/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <thorvg.h>

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

    tvg::Fill* make_fill(const QBrush& brush, qreal opacity)
    {
        auto gradient = brush.gradient();
        if ( !gradient )
            return nullptr;

        tvg::Fill* fill = nullptr;

        if ( gradient->type() == QGradient::RadialGradient )
        {
            const QRadialGradient* rad = static_cast<const QRadialGradient*>(gradient);
            auto radial = tvg::RadialGradient::gen();
            fill = radial;
            radial->radial(
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
            auto linear = tvg::LinearGradient::gen();
            fill = linear;
            linear->linear(
                lin->start().x(),
                lin->start().y(),
                lin->finalStop().x(),
                lin->finalStop().y()
            );
        }
        else if ( gradient->type() == QGradient::ConicalGradient )
        {
            // TODO
            const QConicalGradient* conic = static_cast<const QConicalGradient*>(gradient);
            auto radial = tvg::RadialGradient::gen();
            fill = radial;
            radial->radial(
                conic->center().x(),
                conic->center().y(),
                1000,
                conic->center().x(),
                conic->center().y(),
                0
            );
        }

        if ( !fill )
            return nullptr;

        std::vector<tvg::Fill::ColorStop> stops;
        for ( const auto& stop : gradient->stops() )
        {
            const auto& color = stop.second;
            stops.push_back({float(stop.first), uint8_t(color.red()), uint8_t(color.green()), uint8_t(color.blue()), uint8_t(color.alpha() * opacity)});
        }

        fill->colorStops(stops.data(), stops.size());

        fill->spread(
            gradient->spread() == QGradient::PadSpread ?
                tvg::FillSpread::Pad :
                (gradient->spread() == QGradient::ReflectSpread ?
                    tvg::FillSpread::Reflect :
                    tvg::FillSpread::Repeat
                )
        );

        fill->transform(convert_transform(brush.transform()));

        return fill;

    }

    tvg::Matrix convert_transform(const QTransform& matrix)
    {
        return tvg::Matrix {
            float(matrix.m11()), float(matrix.m21()), float(matrix.m31()),
            float(matrix.m12()), float(matrix.m22()), float(matrix.m32()),
            float(matrix.m13()), float(matrix.m23()), float(matrix.m33()),
        };
    }

    void draw_path(const math::bezier::MultiBezier& path, tvg::Shape* shape)
    {

        for ( const auto& bez : path.beziers() )
        {
            if ( bez.size() == 0 )
                continue;

            shape->moveTo(bez[0].pos.x(), bez[0].pos.y());
            for ( int i = 0; i < bez.size() - 1; i++ )
            {
                const auto& before = bez[i];
                const auto& after = bez[i+1];
                shape->cubicTo(
                    before.tan_out.x(), before.tan_out.y(),
                    after.tan_in.x(), after.tan_in.y(),
                    after.pos.x(), after.pos.y()
                );
            }

            if ( bez.closed() )
            {
                const auto& before = bez.back();
                const auto& after = bez[0];
                shape->cubicTo(
                    before.tan_out.x(), before.tan_out.y(),
                    after.tan_in.x(), after.tan_in.y(),
                    after.pos.x(), after.pos.y()
                );
                shape->close();
            }
        }
    }

    tvg::ColorSpace convert_image_format(QImage::Format fmt) const
    {
        switch ( fmt )
        {
            case QImage::Format_ARGB32:
                return tvg::ColorSpace::ARGB8888S;
            case QImage::Format_ARGB32_Premultiplied:
                return tvg::ColorSpace::ARGB8888;
            case QImage::Format_Grayscale8:
                return tvg::ColorSpace::Grayscale8;
            default:
                return tvg::ColorSpace::Unknown;
        }
    }

    tvg::Picture* convert_image(const QImage & image)
    {
        auto picture = tvg::Picture::gen();

        auto result = picture->load(
            reinterpret_cast<const uint32_t*>(image.constBits()),
            image.width(),
            image.height(),
            convert_image_format(image.format()),
            false
        );
        if ( result != tvg::Result::Success )
        {
            picture->unref();
            return nullptr;
        }

        return picture;
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

    int supported_surfaces() const override
    {
        return SurfaceType::OpenGL;
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
            convert_image_format(destination->format())
        );
        canvas.reset(sw_canvas);
    }

    bool set_gl_surface(void * context, int framebuffer, int width, int height) override
    {
#ifdef OPENGL_ENABLED
        auto gl_canvas = tvg::GlCanvas::gen();
        if ( !gl_canvas )
            return false;
        canvas.reset(gl_canvas);
        return gl_canvas->target(nullptr, nullptr, context, framebuffer, width, height, tvg::ColorSpace::ABGR8888S) == tvg::Result::Success;
#else
        Q_UNUSED(context);
        Q_UNUSED(framebuffer);
        Q_UNUSED(width);
        Q_UNUSED(height);
        return false;
#endif
    }

    bool set_painter_surface(QPainter*, int, int) override
    {
        return false;
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

    void fill_rect(const QRectF & rect, const QBrush & brush) override
    {
        auto shape = tvg::Shape::gen();
        shape->appendRect(rect.left(), rect.top(), rect.width(), rect.height(), 0, 0);
        if ( auto fill = make_fill(brush, 1) )
        {
            shape->fill(fill);
        }
        else
        {
            QColor col = brush.color();
            shape->fill(col.red(), col.green(), col.blue(), col.alpha());
        }
        layers.back()->add(shape);

    }

    void draw_path(const math::bezier::MultiBezier & path) override
    {
        if ( !mode )
            return;

        auto shape = tvg::Shape::gen();
        draw_path(path, shape);

        if ( mode & FillMode )
        {
            if ( auto tfill = make_fill(fill.brush, fill.opacity) )
            {
                shape->fill(tfill);
            }
            else
            {
                QColor col = fill.brush.color();
                shape->fill(col.red(), col.green(), col.blue(), col.alpha() * fill.opacity);
            }
            shape->fillRule(fill.rule == Qt::OddEvenFill ? tvg::FillRule::EvenOdd : tvg::FillRule::NonZero);
        }

        if ( mode & StrokeMode )
        {
            if ( auto tfill = make_fill(stroke.pen.brush(), stroke.opacity) )
            {
                shape->strokeFill(tfill);
            }
            else
            {
                QColor col = stroke.pen.brush().color();
                shape->strokeFill(col.red(), col.green(), col.blue(), col.alpha() * stroke.opacity);
            }

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

    void clip_path(const math::bezier::MultiBezier & path, Qt::ClipOperation op = Qt::ReplaceClip) override
    {
        // TODO if ( op == Qt::ReplaceClip )
        auto shape = tvg::Shape::gen();
        draw_path(path, shape);
        layers.back()->clip(shape);
    }

    void draw_image(const QImage & image) override
    {
        if ( auto picture = convert_image(image) )
            layers.back()->add(picture);
    }

    void fill_pattern(const QRectF& rect, const QImage& pattern) override
    {
        if ( auto picture = convert_image(pattern) )
        {
            layer_start();
            clip_rect(rect);
            for ( int y = rect.top(); y < rect.bottom(); y += pattern.height() )
            {
                for ( int x = rect.left(); x < rect.right(); x += pattern.width() )
                {
                    auto copy = picture->duplicate();
                    copy->translate(x, y);
                    layers.back()->add(copy);
                }
            }
            layer_end();

            picture->unref();
        }
    }


    void scale(qreal x, qreal) override
    {
        layers.back()->scale(x);
    }

    void translate(qreal x, qreal y) override
    {
        layers.back()->translate(x, y);
    }

    void transform(const QTransform & matrix) override
    {
        layers.back()->transform(convert_transform(matrix));
    }
};

} // namespace glaxnimate::renderer


