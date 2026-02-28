/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QPainter>

#include "utils/maybe_ptr.hpp"
#include "renderer/renderer.hpp"

namespace glaxnimate::renderer {


/**
 * \brief Renderer that uses QPainter
 */
class QPainterRenderer : public Renderer
{
private:
    utils::maybe_ptr<QPainter> painter;
    Fill fill;
    Stroke stroke;
    int mode = ShapeMode::NothingMode;


public:
    explicit QPainterRenderer(QPainter* painter)
        : painter(painter, false)
    {}

    QPainterRenderer() = default;

    int supported_surfaces() const override
    {
        return SurfaceType::Painter;
    }

    void set_image_surface(QImage * destination) override
    {
        painter.reset(new QPainter(destination), true);
        painter->setRenderHint(QPainter::Antialiasing);
    }

    bool set_gl_surface(void *, int, int, int) override
    {
        return false;
    }

    bool set_painter_surface(QPainter * painter, int, int) override
    {
        this->painter.reset(painter, false);
        return true;
    }

    void render_start() override
    {
        if ( !painter )
        {
            painter.reset(new QPainter(), true);
        }
    }

    void render_end() override
    {
        if ( painter.owns_pointer() )
            painter->end();
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
            painter->setBrush(fill.brush);
            painter->setOpacity(painter->opacity() * fill.opacity);
            painter->setPen(Qt::NoPen);
            path.setFillRule(Qt::FillRule(fill.rule));
            painter->drawPath(path);
        }

        if ( mode & StrokeMode )
        {
            painter->setBrush(Qt::NoBrush);
            painter->setPen(stroke.pen);
            painter->setOpacity(painter->opacity() * stroke.opacity);
            painter->drawPath(bez.painter_path());
        }

        mode = NothingMode;
    }

    void fill_rect(const QRectF & rect, const QBrush & brush) override
    {
        painter->fillRect(rect, brush);
    }

    void fill_pattern(const QRectF & rect, const QImage & pattern) override
    {
        QBrush brush;
        brush.setTexture(QPixmap::fromImage(pattern));
        painter->fillRect(rect, brush);
    }

    void layer_start() override
    {
        painter->save();
    }

    void layer_end() override
    {
        painter->restore();
    }

    void set_opacity(qreal opacity) override
    {
        painter->setOpacity(opacity * painter->opacity());
    }

    qreal opacity() const override
    {
        return painter->opacity();
    }

    void clip_rect(const QRectF & rect, Qt::ClipOperation op = Qt::ReplaceClip) override
    {
        painter->setClipRect(rect, op);
    }

    void clip_path(const math::bezier::MultiBezier & path, Qt::ClipOperation op = Qt::ReplaceClip) override
    {
        painter->setClipPath(path.painter_path(), op);
    }

    void draw_image(const QImage & image) override
    {
        painter->drawImage(0, 0, image);
    }

    void scale(qreal x, qreal y) override
    {
        painter->scale(x, y);
    }

    void translate(qreal x, qreal y) override
    {
        painter->translate(x, y);
    }

    void transform(const QTransform & matrix) override
    {
        painter->setTransform(matrix, true);
    }


};

} // namespace glaxnimate::renderer
