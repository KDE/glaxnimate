/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "graphics/handle.hpp"
#include "model/transform.hpp"
#include "model/document_node.hpp"


namespace glaxnimate::gui::graphics {

class TransformGraphicsItem : public QGraphicsObject
{
    Q_OBJECT

public:
    TransformGraphicsItem(glaxnimate::model::Transform* transform, glaxnimate::model::VisualNode* target, QGraphicsItem* parent);
    ~TransformGraphicsItem();

    QRectF boundingRect() const override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;

public Q_SLOTS:
    void set_transform_matrix(const QTransform& t);

private Q_SLOTS:
    void drag_tl(const QPointF& p, Qt::KeyboardModifiers modifiers);
    void drag_tr(const QPointF& p, Qt::KeyboardModifiers modifiers);
    void drag_br(const QPointF& p, Qt::KeyboardModifiers modifiers);
    void drag_bl(const QPointF& p, Qt::KeyboardModifiers modifiers);
    void drag_t(const QPointF& p, Qt::KeyboardModifiers modifiers);
    void drag_b(const QPointF& p, Qt::KeyboardModifiers modifiers);
    void drag_l(const QPointF& p, Qt::KeyboardModifiers modifiers);
    void drag_r(const QPointF& p, Qt::KeyboardModifiers modifiers);
    void drag_a(const QPointF& p, Qt::KeyboardModifiers modifiers);
    void drag_rot(const QPointF& p, Qt::KeyboardModifiers modifiers);
    void drag_rot_start(const QPointF& p, Qt::KeyboardModifiers modifiers);
    void drag_pos(const QPointF& p, Qt::KeyboardModifiers modifiers);
    void drag_pos_start(const QPointF& p);

    void commit_scale();
    void commit_anchor();
    void commit_rot();
    void commit_pos();

    void update_handles();
    void update_transform();
    void update_offshoots();

protected:
    QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui::graphics
