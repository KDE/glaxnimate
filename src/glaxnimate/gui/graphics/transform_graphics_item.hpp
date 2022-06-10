#pragma once

#include "graphics/handle.hpp"
#include "glaxnimate/core/model/transform.hpp"
#include "glaxnimate/core/model/document_node.hpp"


namespace glaxnimate::gui::graphics {

class TransformGraphicsItem : public QGraphicsObject
{
    Q_OBJECT

public:
    TransformGraphicsItem(glaxnimate::model::Transform* transform, glaxnimate::model::VisualNode* target, QGraphicsItem* parent);
    ~TransformGraphicsItem();

    QRectF boundingRect() const override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;

public slots:
    void set_transform_matrix(const QTransform& t);

private slots:
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
