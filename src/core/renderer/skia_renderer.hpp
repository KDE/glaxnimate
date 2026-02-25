/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <stack>

#include <QImage>

#include <SkCanvas.h>
#include <SkSurface.h>
#include <SkPath.h>
#include <SkPaint.h>

#include "renderer/renderer.hpp"

namespace glaxnimate::renderer {

template<class T>
class maybe_ptr
{
private:
    T* data;
    bool owns;

    void delete_owned() noexcept
    {
        delete data;
        owns = false;
    }

    void clear() noexcept
    {
        data = nullptr;
        owns = false;
    }
public:
    using pointer = T*;

    maybe_ptr(T* data, bool owns) noexcept : data(data), owns(owns) {}
    maybe_ptr() noexcept : data(nullptr), owns(false) {}
    maybe_ptr(const maybe_ptr&) = delete;
    maybe_ptr(maybe_ptr&& o) noexcept : data(o.data), owns(o.owns)
    {
        if ( o.owns )
            o.clear();
    }
    maybe_ptr& operator=(const maybe_ptr&) = delete;
    maybe_ptr& operator=(maybe_ptr&& o) noexcept
    {
        std::swap(o.data, data);
        std::swap(o.owns, owns);
        return *this;

    }

    ~maybe_ptr() noexcept { delete_owned(); }

    void reset(T* data, bool owns)
    {
        delete_owned();
        this->data = data;
        this->owns = owns;
    }

    T* operator->() noexcept { return data; }

    explicit operator bool() noexcept { return data; }
};

/**
 * \brief Renderer that uses Skia
 * \see https://api.skia.org/ https://skia.org/docs/
 */
class SkiaRenderer : public Renderer
{
private:
    enum Mode { NothingMode = 0, FillMode = 1, StrokeMode = 2 };

    maybe_ptr<QImage> image = {};
    sk_sp<SkSurface> surface;
    SkCanvas* canvas = nullptr;
    Fill fill;
    Stroke stroke;
    int mode = NothingMode;
    std::vector<std::unique_ptr<SkPaint>> layers;
    SkPaint* current_layer = nullptr;

public:
    void set_image_surface(QImage * destination) override
    {
        image.reset(destination, false);
        SkImageInfo info = SkImageInfo::MakeN32Premul(destination->width(), destination->height());
        surface = SkSurfaces::WrapPixels(
            info,
            destination->bits(),
            destination->bytesPerLine()
        );
        canvas = surface->getCanvas();
    }

    void render_start() override
    {
        layer_start();
    }

    void render_end() override
    {
        layer_end();
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

    SkColor4f convert_color(const QColor& c)
    {
        return {c.redF(), c.greenF(), c.blueF(), c.alphaF()};
    }

    void draw_path(const math::bezier::MultiBezier & bez) override
    {
        auto path = convert_path(bez.painter_path());

        if ( mode & FillMode )
        {
            SkPaint skfill(convert_color(fill.brush.color()));
            skfill.setAntiAlias(true);
            skfill.setAlphaf(skfill.getAlphaf() * fill.opacity);
            path.setFillType(fill.rule == Qt::WindingFill ? SkPathFillType::kWinding : SkPathFillType::kEvenOdd);
            canvas->drawPath(path, skfill);
        }

        if ( mode & StrokeMode )
        {
            SkPaint skstroke(convert_color(stroke.pen.brush().color()));
            skstroke.setAntiAlias(true);
            skstroke.setStyle(SkPaint::kStroke_Style);
            skstroke.setAlphaf(skstroke.getAlphaf() * fill.opacity);
            skstroke.setStrokeWidth(stroke.pen.width());
            auto qjoin = stroke.pen.joinStyle();
            skstroke.setStrokeJoin(
                qjoin == Qt::RoundJoin ?
                    SkPaint::kRound_Join :
                    (qjoin == Qt::MiterJoin ?
                        SkPaint::kMiter_Join :
                        SkPaint::kBevel_Join
                    )
            );
            auto qcap = stroke.pen.capStyle();
            skstroke.setStrokeCap(
                qcap == Qt::RoundCap ?
                SkPaint::kRound_Cap :
                (qcap == Qt::SquareCap ?
                SkPaint::kSquare_Cap :
                SkPaint::kButt_Cap
                )
            );
            skstroke.setStrokeMiter(stroke.pen.miterLimit());
            canvas->drawPath(path, skstroke);
        }

        mode = NothingMode;
    }

    void layer_start() override
    {
        std::unique_ptr<SkPaint> layer = std::make_unique<SkPaint>(SkColors::kTransparent);
        current_layer = layer.get();
        layer->setAntiAlias(true);
        canvas->saveLayer(nullptr, layer.get());
        layers.push_back(std::move(layer));
    }

    void layer_end() override
    {
        canvas->restore();
        layers.pop_back();
        if ( layers.size() )
            current_layer = layers.back().get();
        else
            current_layer = nullptr;
    }

    void set_opacity(qreal opacity) override
    {
        current_layer->setAlphaf(opacity);
    }

    qreal opacity() const override
    {
        return current_layer->getAlphaf();
    }

    void clip_rect(const QRectF & rect, Qt::ClipOperation = Qt::ReplaceClip) override
    {
        SkPath path;
        path.addRect(SkRect::MakeXYWH(rect.left(), rect.top(), rect.width(), rect.height()));
        canvas->clipPath(path, true);
    }

    SkPath convert_path(const QPainterPath & path) const
    {
        SkPath skpath;
        int count = path.elementCount();
        for ( int i = 0; i < count; i++ )
        {
            auto el = path.elementAt(i);
            switch (el.type)
            {
                case QPainterPath::MoveToElement:
                    skpath.moveTo(el.x, el.y);
                    break;

                case QPainterPath::LineToElement:
                    skpath.lineTo(el.x, el.y);
                    break;

                case QPainterPath::CurveToElement:
                {
                    const QPainterPath::Element& cp2 = path.elementAt(i + 1);
                    const QPainterPath::Element& end = path.elementAt(i + 2);

                    skpath.cubicTo(el.x, el.y, cp2.x, cp2.y, end.x, end.y);

                    i += 2;
                    break;
                }

                default:
                    break;
            }
        }
        return skpath;
    }

    void clip_path(const QPainterPath & path, Qt::ClipOperation = Qt::ReplaceClip) override
    {
        canvas->clipPath(convert_path(path), true);
    }

    void draw_pixmap(const QPixmap & pixmap) override
    {
        QImage img = pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
        SkImageInfo info = SkImageInfo::MakeN32Premul(img.width(), img.height());
        sk_sp<SkImage> skImage = SkImages::RasterFromPixmap(
            SkPixmap(info, img.bits(), img.bytesPerLine()),
            nullptr,
            nullptr
        );

        if ( skImage )
            canvas->drawImage(skImage, 0, 0);
    }

    void scale(qreal x, qreal y) override
    {
        canvas->scale(x, y);
    }

    void translate(qreal x, qreal y) override
    {
        canvas->translate(x, y);
    }

    void transform(const QTransform & matrix) override
    {
        auto existing = canvas->getLocalToDevice();
        SkM44 conv(
            matrix.m11(), matrix.m12(), matrix.m13(), 0,
            matrix.m21(), matrix.m22(), matrix.m23(), 0,
            matrix.m31(), matrix.m32(), matrix.m33(), 0,
            0, 0, 0, 1
        );
        canvas->setMatrix(existing.postConcat(conv));
    }


};

} // namespace glaxnimate::renderer

