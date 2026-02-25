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

/**
 * Abstracted renderer interface
 */
class Renderer
{
public:
    virtual ~Renderer() noexcept = default;

// Surface setup
    /**
     * \brief Sets \p destination as the destination surface
     */
    virtual void set_image_surface(QImage* destination) = 0;

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
    virtual void clip_path(const QPainterPath& path, Qt::ClipOperation op = Qt::ReplaceClip) = 0;

    virtual void draw_pixmap(const QPixmap& image) = 0;

// Transform
    virtual void scale(qreal x, qreal y) = 0;
    virtual void translate(qreal x, qreal y) = 0;
    virtual void transform(const QTransform& matrix) = 0;
};

} // namespace glaxnimate::renderer
