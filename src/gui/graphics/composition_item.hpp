/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#pragma once

#include "model/document.hpp"
#include "model/assets/composition.hpp"
#include "command/property_commands.hpp"

#include "graphics/handle.hpp"
#include "graphics/document_node_graphics_item.hpp"
#include "graphics/transform_graphics_item.hpp"
#include "renderer/qpainter_renderer.hpp"
#include "renderer/skia_renderer.hpp"

namespace glaxnimate::gui::graphics {

class CompositionItem : public DocumentNodeGraphicsItem
{
    model::Composition* comp;
public:
    explicit CompositionItem (model::Composition* animation)
        : DocumentNodeGraphicsItem(animation), comp(animation)
    {
        setFlag(QGraphicsItem::ItemIsSelectable, false);
        setFlag(QGraphicsItem::ItemHasNoContents, false);
        set_selection_mode(None);
        connect(animation->document(), &model::Document::graphics_invalidated, this, [this]{refresh();});
        connect(animation, &model::Composition::width_changed, this, &CompositionItem::size_changed);
        connect(animation, &model::Composition::height_changed, this, &CompositionItem::size_changed);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override
    {
        QImage img(comp->width.get(), comp->height.get(), QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::transparent);
        renderer::SkiaRenderer renderer;
        renderer.set_image_surface(&img);

        // renderer::QPainterRenderer renderer(painter);

        renderer.render_start();
        node()->paint(&renderer, node()->time(), model::VisualNode::Canvas);
        renderer.render_end();

        painter->drawImage(0, 0, img);
    }

    void refresh()
    {
        update();
    }

private Q_SLOTS:
    void size_changed()
    {
        prepareGeometryChange();
    }
};



class CompositionTransformItem : public QGraphicsObject
{
public:
    explicit CompositionTransformItem(model::Composition* animation)
        : animation(animation)
    {
        connect(animation, &model::Composition::width_changed, this, &CompositionTransformItem::size_changed);
        connect(animation, &model::Composition::height_changed, this, &CompositionTransformItem::size_changed);

        handle_h = new MoveHandle(this, MoveHandle::Horizontal, MoveHandle::Diamond, 8);
        handle_v = new MoveHandle(this, MoveHandle::Vertical, MoveHandle::Diamond, 8);
        handle_hv = new MoveHandle(this, MoveHandle::DiagonalDown, MoveHandle::Square);
        connect(handle_h, &MoveHandle::dragged_x, this, &CompositionTransformItem::dragged_x);
        connect(handle_v, &MoveHandle::dragged_y, this, &CompositionTransformItem::dragged_y);
        connect(handle_hv, &MoveHandle::dragged, this, &CompositionTransformItem::dragged_xy);
        connect(handle_h,  &MoveHandle::drag_finished, this, &CompositionTransformItem::drag_finished);
        connect(handle_v,  &MoveHandle::drag_finished, this, &CompositionTransformItem::drag_finished);
        connect(handle_hv, &MoveHandle::drag_finished, this, &CompositionTransformItem::drag_finished);

        update_handles();
    }

    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = nullptr) override {}
    QRectF boundingRect() const override { return {}; }

private Q_SLOTS:
    void size_changed()
    {
        update_handles();
    }

    void dragged_xy(const QPointF& pos)
    {
        update_size(pos.x(), pos.y(), false);
    }

    void dragged_x(qreal x)
    {
        update_size(x, animation->height.get(), false);
    }

    void dragged_y(qreal y)
    {
        update_size(animation->width.get(), y, false);
    }

    void drag_finished()
    {
        update_size(animation->width.get(), animation->height.get(), true);
    }

private:
    void update_size(qreal x, qreal y, bool commit)
    {
        int w = std::max(1, qRound(x));
        int h = std::max(1, qRound(y));
        if ( w != animation->width.get() || h != animation->height.get() )
        {
            animation->push_command(new command::SetMultipleProperties(
                i18n("Resize Canvas"),
                commit,
                {&animation->width, &animation->height},
                w, h
            ));
        };
        update_handles();
    }

    void update_handles()
    {
        handle_h->setPos(QPointF(animation->width.get(), animation->height.get() / 2.0));
        handle_v->setPos(QPointF(animation->width.get() / 2.0, animation->height.get()));
        handle_hv->setPos(QPointF(animation->width.get(), animation->height.get()));
    }

private:
    model::Composition* animation;
    MoveHandle* handle_h;
    MoveHandle* handle_v;
    MoveHandle* handle_hv;
};

} // namespace glaxnimate::gui::graphics
