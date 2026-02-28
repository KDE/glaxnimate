/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QPen>
#include "math/bezier/bezier.hpp"

namespace glaxnimate::renderer {

struct Fill
{
    QBrush brush = {};
    qreal opacity = 1;
    Qt::FillRule rule = Qt::OddEvenFill;
};

struct Stroke
{
    QPen pen = {};
    qreal opacity = 1;
};

enum ShapeMode { NothingMode = 0, FillMode = 1, StrokeMode = 2 };

enum SurfaceType
{
    OpenGL  = 0x10,
    Painter = 0x20,
};

/**
 * Abstracted renderer interface
 */
class Renderer
{
public:
    Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    virtual ~Renderer() noexcept = default;

// Query
    virtual int supported_surfaces() const = 0;

    bool supports_surface(int type) const { return (supported_surfaces() & type) == type; }

// Surface setup
    /**
     * \brief Sets \p destination as the destination surface
     */
    virtual void set_image_surface(QImage* destination) = 0;

    /**
     * \brief Sets the given OpenGL context as the destination surface
     * \returns \b true on success
     */
    virtual bool set_gl_surface(void* context, int framebuffer, int width, int height) = 0;

    /**
     * \brief Sets a QPainter as the render target
     */
    virtual bool set_painter_surface(QPainter* painter, int width, int height) = 0;

    /**
     * \brief Performs any operation needed to begin the rendering
     */
    virtual void render_start() = 0;
    /**
     * \brief Cleans up and applies any pending drawings to the surface
     */
    virtual void render_end() = 0;

// Drawing ops
    /**
     * \brief Sets the fill for the following drawing operation
     */
    virtual void set_fill(const Fill& fill) = 0;
    /**
     * \brief Sets the stroke for the following drawing operation
     */
    virtual void set_stroke(const Stroke& stroke) = 0;
    /**
     * \brief Draws the path based on the last specified fill or stroke
     */
    virtual void draw_path(const math::bezier::MultiBezier& bez) = 0;

    /**
     * \brief Fills a rectangle with a brusj
     */
    virtual void fill_rect(const QRectF& rect, const QBrush& brush)
    {
        set_fill({brush});
        math::bezier::MultiBezier bez;
        bez.move_to(QPointF(rect.left(), rect.top()));
        bez.line_to(QPointF(rect.right(), rect.top()));
        bez.line_to(QPointF(rect.right(), rect.bottom()));
        bez.line_to(QPointF(rect.left(), rect.bottom()));
        bez.close();
        draw_path(bez);
    }

// Compositing ops
    /**
     * \brief Starts a new layer for compositing
     */
    virtual void layer_start() = 0;
    /**
     * \brief Applies the layer to the current surface
     */
    virtual void layer_end() = 0;

    /**
     * \brief Sets layer opacity
     */
    virtual void set_opacity(qreal opacity) = 0;
    /**
     * \brief Layer opacity
     */
    virtual qreal opacity() const = 0;
    /**
     * \brief Sets a clipping rect
     */
    virtual void clip_rect(const QRectF& rect, Qt::ClipOperation op = Qt::ReplaceClip) = 0;
    virtual void clip_path(const math::bezier::MultiBezier& path, Qt::ClipOperation op = Qt::ReplaceClip) = 0;

    virtual void draw_image(const QImage& image) = 0;

// Transform
    virtual void scale(qreal x, qreal y) = 0;
    virtual void translate(qreal x, qreal y) = 0;
    virtual void transform(const QTransform& matrix) = 0;
};

/**
 * \brief Creates the default renderer for the given quality
 * \param quality number from 0 to 10
 */
std::unique_ptr<Renderer> default_renderer(int quality);

QString gl_version();

} // namespace glaxnimate::renderer
