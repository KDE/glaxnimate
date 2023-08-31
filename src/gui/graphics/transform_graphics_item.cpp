/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "transform_graphics_item.hpp"
#include <QPainter>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QStyleOptionGraphicsItem>

#include "model/document.hpp"
#include "command/animation_commands.hpp"
#include "math/math.hpp"
#include "glaxnimate_app.hpp"
#include "widgets/canvas.hpp"

using namespace glaxnimate::gui;

class graphics::TransformGraphicsItem::Private
{
public:
    enum Index
    {
        Anchor,
        Rot,
        Position,

        TL,
        TR,
        BR,
        BL,
        Top,
        Bottom,
        Left,
        Right,

        Count
    };

    struct Handle
    {
        using Getter = QPointF (Private::*)()const;
        using Signal = void (TransformGraphicsItem::*)(const QPointF&, Qt::KeyboardModifiers);

        MoveHandle* handle;
        Getter get_p;
        Signal signal;

        Handle(
            MoveHandle* handle,
            Getter get_p,
            Signal signal,
            const std::vector<model::AnimatableBase*>& props
        ) : handle(handle), get_p(get_p), signal(signal)
        {
            handle->set_associated_properties(props);
        }
    };

    model::Transform* transform;
    model::VisualNode* target;
    std::array<Handle, Count> handles;
    QRectF cache;
    QTransform transform_matrix;
    QTransform transform_matrix_inv;

    QPointF pos_drag_start;
    QTransform pos_trans;
    QPointF pos_start_value;
    TransformGraphicsItem* parent;
    Canvas* canvas = nullptr;

    QPointF get_tl() const { return cache.topLeft(); }
    QPointF get_tr() const { return cache.topRight(); }
    QPointF get_br() const { return cache.bottomRight(); }
    QPointF get_bl() const { return cache.bottomLeft(); }
    QPointF get_t() const { return {cache.center().x(), cache.top()}; }
    QPointF get_b() const { return {cache.center().x(), cache.bottom()}; }
    QPointF get_l() const { return {cache.left(), cache.center().y()}; }
    QPointF get_r() const { return {cache.right(), cache.center().y()}; }
    QPointF get_a() const
    {
        return transform->anchor_point.get();
    }
    QPointF get_rot() const
    {
        return get_t();
    }
    qreal offshoot_size() const
    {
        return 48 * GlaxnimateApp::handle_distance_multiplier();
    }
    QPointF get_pos() const
    {
        return get_l();
    }

    void set_pos(const Handle& h) const
    {
        h.handle->setPos((this->*h.get_p)());
    }

    Private(TransformGraphicsItem* parent, model::Transform* transform, model::VisualNode* target)
    : transform(transform),
        target(target),
        handles{
            Handle{
                new MoveHandle(parent, MoveHandle::Any, MoveHandle::Saltire, 16, true),
                &TransformGraphicsItem::Private::get_a,
                &TransformGraphicsItem::drag_a,
                {&transform->anchor_point, &transform->position}
            },
            Handle{
                new MoveHandle(parent, MoveHandle::Rotate, MoveHandle::Circle, 6, true),
                &TransformGraphicsItem::Private::get_rot,
                &TransformGraphicsItem::drag_rot,
                {&transform->rotation}
            },
            Handle{
                new MoveHandle(parent, MoveHandle::Any, MoveHandle::Cross, 6, true),
                &TransformGraphicsItem::Private::get_pos,
                &TransformGraphicsItem::drag_pos,
                {&transform->position}
            },
            Handle{
                new MoveHandle(parent, MoveHandle::DiagonalDown, MoveHandle::Square, 6, true),
                &TransformGraphicsItem::Private::get_tl,
                &TransformGraphicsItem::drag_tl,
                {&transform->position, &transform->scale}
            },
            Handle{
                new MoveHandle(parent, MoveHandle::DiagonalUp, MoveHandle::Square, 6, true),
                &TransformGraphicsItem::Private::get_tr,
                &TransformGraphicsItem::drag_tr,
                {&transform->position, &transform->scale}
            },
            Handle{
                new MoveHandle(parent, MoveHandle::DiagonalDown, MoveHandle::Square, 6, true),
                &TransformGraphicsItem::Private::get_br,
                &TransformGraphicsItem::drag_br,
                {&transform->position, &transform->scale}
            },
            Handle{
                new MoveHandle(parent, MoveHandle::DiagonalUp, MoveHandle::Square, 6, true),
                &TransformGraphicsItem::Private::get_bl,
                &TransformGraphicsItem::drag_bl,
                {&transform->position, &transform->scale}
            },
            Handle{
                new MoveHandle(parent, MoveHandle::Vertical, MoveHandle::Diamond, 8, true),
                &TransformGraphicsItem::Private::get_t,
                &TransformGraphicsItem::drag_t,
                {&transform->position, &transform->scale}
            },
            Handle{
                new MoveHandle(parent, MoveHandle::Vertical, MoveHandle::Diamond, 8, true),
                &TransformGraphicsItem::Private::get_b,
                &TransformGraphicsItem::drag_b,
                {&transform->position, &transform->scale}
            },
            Handle{
                new MoveHandle(parent, MoveHandle::Horizontal, MoveHandle::Diamond, 8, true),
                &TransformGraphicsItem::Private::get_l,
                &TransformGraphicsItem::drag_l,
                {&transform->position, &transform->scale}
            },
            Handle{
                new MoveHandle(parent, MoveHandle::Horizontal, MoveHandle::Diamond, 8, true),
                &TransformGraphicsItem::Private::get_r,
                &TransformGraphicsItem::drag_r,
                {&transform->position, &transform->scale}
            },
        },
        parent(parent)
    {

        handles[Position].handle->set_offset({-offshoot_size(), 0});
        handles[Rot].handle->set_offset({0, -offshoot_size()});
    }

    qreal find_scale(QPointF target_local, qreal size_to_anchor, qreal anchor_lin, qreal target_local_lin, qreal old_scale)
    {

        QPointF ap = transform_matrix.map(transform->anchor_point.get());
        QPointF target = transform_matrix.map(target_local);
        qreal target_length = math::length(target - ap);
        qreal new_scale = target_length / size_to_anchor;
        auto sign = math::sign(target_local_lin - anchor_lin);
        if ( sign != math::sign(old_scale) )
            new_scale *= -1;
        if ( qFuzzyCompare(new_scale, 0) )
            new_scale = 0.01 * sign;

        return new_scale;
    }

    void push_command(model::AnimatableBase& prop, const QVariant& value, bool commit)
    {
        target->document()->undo_stack().push(new command::SetMultipleAnimated(
            prop.name(), {&prop}, {prop.value()}, {value}, commit
        ));
    }

    bool find_scale_y(const QPointF& p, qreal y, qreal& scale)
    {
        qreal size_to_anchor_y = y - transform->anchor_point.get().y();
        if ( size_to_anchor_y == 0 )
        {
            scale = transform->scale.get().y();
            return false;
        }

        scale = find_scale(
            QPointF(transform->anchor_point.get().x(), p.y()),
            size_to_anchor_y,
            transform->anchor_point.get().y(),
            p.y(),
            transform->scale.get().y()
        );
        return true;
    }

    bool find_scale_x(const QPointF& p, qreal x, qreal& scale)
    {
        qreal size_to_anchor_x = x - transform->anchor_point.get().x();
        if ( size_to_anchor_x == 0 )
        {
            scale = transform->scale.get().x();
            return false;
        }

        scale = find_scale(
            QPointF(p.x(), transform->anchor_point.get().y()),
            size_to_anchor_x,
            transform->anchor_point.get().x(),
            p.x(),
            transform->scale.get().x()
        );
        return true;
    }

};

graphics::TransformGraphicsItem::TransformGraphicsItem(
    model::Transform* transform, model::VisualNode* target, QGraphicsItem* parent
)
    : QGraphicsObject(parent), d(std::make_unique<Private>(this, transform, target))
{
    connect(target, &model::VisualNode::bounding_rect_changed, this, &TransformGraphicsItem::update_handles);
    connect(target, &model::VisualNode::transform_matrix_changed, this, &TransformGraphicsItem::update_offshoots);
    connect(transform, &model::Object::property_changed, this, &TransformGraphicsItem::update_transform);

    update_transform();
    update_handles();
    for ( const auto& h : d->handles )
    {
        connect(h.handle, &MoveHandle::dragged, this, h.signal);
        if ( &h == &d->handles[Private::Anchor] )
            connect(h.handle, &MoveHandle::drag_finished, this, &TransformGraphicsItem::commit_anchor);
        else if ( &h == &d->handles[Private::Rot] )
            connect(h.handle, &MoveHandle::drag_finished, this, &TransformGraphicsItem::commit_rot);
        else if ( &h == &d->handles[Private::Position] )
        {
            connect(h.handle, &MoveHandle::drag_starting, this, &TransformGraphicsItem::drag_pos_start);
            connect(h.handle, &MoveHandle::drag_finished, this, &TransformGraphicsItem::commit_pos);
        }
        else
            connect(h.handle, &MoveHandle::drag_finished, this, &TransformGraphicsItem::commit_scale);
    }

#ifdef Q_OS_ANDROID
    d->handles[Private::Anchor].handle->setVisible(false);
#endif
}

graphics::TransformGraphicsItem::~TransformGraphicsItem() = default;

void glaxnimate::gui::graphics::TransformGraphicsItem::update_offshoots()
{
    prepareGeometryChange();
    d->set_pos(d->handles[Private::Anchor]);


    auto length = d->offshoot_size();

    // Evaluate effective rotation
    auto transform = sceneTransform();

    // Hack but idk how else to do this...
    if ( d->canvas )
        transform = transform * d->canvas->transform();

    QPointF p1 = transform.map(QPointF(0, 0));
    QPointF p2 = transform.map(QPointF(10, 0));
    qreal angle = math::atan2(p2.y() - p1.y(), p2.x() - p1.x());

    d->handles[Private::Position].handle->set_offset({length * math::cos(angle - math::pi), length * math::sin(angle - math::pi)});
    d->handles[Private::Rot].handle->set_offset({length * math::cos(angle - math::pi / 2), length * math::sin(angle - math::pi / 2)});
}


void graphics::TransformGraphicsItem::update_handles()
{
    prepareGeometryChange();
    d->cache = d->target->local_bounding_rect(d->target->time());
    for ( const auto& h : d->handles )
    {
        d->set_pos(h);
    }
    update_offshoots();
}

void graphics::TransformGraphicsItem::update_transform()
{
    d->transform_matrix = d->transform->transform_matrix(d->transform->time());
    d->transform_matrix_inv = d->transform_matrix.inverted();
    update_offshoots();
}


void graphics::TransformGraphicsItem::drag_tl(const QPointF& p, Qt::KeyboardModifiers modifiers)
{
    qreal scale_y;
    bool has_y = d->find_scale_y(p, d->cache.top(), scale_y);
    qreal scale_x;
    bool has_x = d->find_scale_x(p, d->cache.left(), scale_x);

    if ( modifiers & Qt::ControlModifier )
        scale_x = scale_y = (scale_x + scale_y) / 2;

    if ( has_x || has_y )
        d->push_command(d->transform->scale, QVector2D(scale_x, scale_y), false);
}

void graphics::TransformGraphicsItem::drag_tr(const QPointF& p, Qt::KeyboardModifiers modifiers)
{
    qreal scale_y;
    bool has_y = d->find_scale_y(p, d->cache.top(), scale_y);
    qreal scale_x;
    bool has_x = d->find_scale_x(p, d->cache.right(), scale_x);

    if ( modifiers & Qt::ControlModifier )
        scale_x = scale_y = (scale_x + scale_y) / 2;

    if ( has_x || has_y )
        d->push_command(d->transform->scale, QVector2D(scale_x, scale_y), false);
}

void graphics::TransformGraphicsItem::drag_br(const QPointF& p, Qt::KeyboardModifiers modifiers)
{
    qreal scale_y;
    bool has_y = d->find_scale_y(p, d->cache.bottom(), scale_y);
    qreal scale_x;
    bool has_x = d->find_scale_x(p, d->cache.right(), scale_x);

    if ( modifiers & Qt::ControlModifier )
        scale_x = scale_y = (scale_x + scale_y) / 2;

    if ( has_x || has_y )
        d->push_command(d->transform->scale, QVector2D(scale_x, scale_y), false);
}

void graphics::TransformGraphicsItem::drag_bl(const QPointF& p, Qt::KeyboardModifiers modifiers)
{
    qreal scale_y;
    bool has_y = d->find_scale_y(p, d->cache.bottom(), scale_y);
    qreal scale_x;
    bool has_x = d->find_scale_x(p, d->cache.left(), scale_x);

    if ( modifiers & Qt::ControlModifier )
        scale_x = scale_y = (scale_x + scale_y) / 2;

    if ( has_x || has_y )
        d->push_command(d->transform->scale, QVector2D(scale_x, scale_y), false);
}

void graphics::TransformGraphicsItem::drag_t(const QPointF& p, Qt::KeyboardModifiers)
{
    qreal scale;
    if ( d->find_scale_y(p, d->cache.top(), scale) )
        d->push_command(d->transform->scale, QVector2D(d->transform->scale.get().x(), scale), false);
}

void graphics::TransformGraphicsItem::drag_b(const QPointF& p, Qt::KeyboardModifiers)
{
    qreal scale;
    if ( d->find_scale_y(p, d->cache.bottom(), scale) )
        d->push_command(d->transform->scale, QVector2D(d->transform->scale.get().x(), scale), false);
}

void graphics::TransformGraphicsItem::drag_l(const QPointF& p, Qt::KeyboardModifiers)
{
    qreal scale;
    if ( d->find_scale_x(p, d->cache.left(), scale) )
        d->push_command(d->transform->scale, QVector2D(scale, d->transform->scale.get().y()), false);
}

void graphics::TransformGraphicsItem::drag_r(const QPointF& p, Qt::KeyboardModifiers)
{
    qreal scale;
    if ( d->find_scale_x(p, d->cache.right(), scale) )
        d->push_command(d->transform->scale, QVector2D(scale, d->transform->scale.get().y()), false);
}

void graphics::TransformGraphicsItem::drag_a(const QPointF& p, Qt::KeyboardModifiers)
{
    QPointF anchor = p;
    QPointF anchor_old = d->transform->anchor_point.get();


    QPointF p1 = d->transform_matrix.map(QPointF(0, 0));
    d->transform->anchor_point.set(anchor);
    QPointF p2 = d->transform_matrix.map(QPointF(0, 0));

    QPointF pos = d->transform->position.get() - p2 + p1;
    d->transform->anchor_point.set(anchor_old);
    d->target->document()->undo_stack().push(new command::SetMultipleAnimated(
        tr("Drag anchor point"),
        false,
        {&d->transform->anchor_point, &d->transform->position},
        anchor,
        pos
    ));
}

void graphics::TransformGraphicsItem::drag_rot(const QPointF& p, Qt::KeyboardModifiers modifiers)
{
    QPointF diff_old = d->get_rot() - d->transform->anchor_point.get();
    QVector2D scale = d->transform->scale.get();
    qreal angle_to_rot_handle = std::atan2(diff_old.y() * scale.y(), diff_old.x() * scale.x());

    QPointF p_new = d->transform_matrix.map(p);
    QPointF ap = d->transform_matrix.map(d->transform->anchor_point.get());
    QPointF diff_new = p_new - ap;
    qreal angle_new = std::atan2(diff_new.y(), diff_new.x());
    qreal angle = qRadiansToDegrees(angle_new - angle_to_rot_handle);

    if ( modifiers & Qt::ControlModifier )
        angle = qRound(angle/15) * 15;

    d->push_command(d->transform->rotation, angle, false);
}

void graphics::TransformGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* opt, QWidget*)
{
    // Hack needed to keep offshoots straight
    if ( !d->canvas )
    {
        auto views = scene()->views();
        if ( !views.empty() )
        {
            d->canvas = qobject_cast<Canvas*>(views[0]);
            connect(d->canvas, &Canvas::rotated, this, &TransformGraphicsItem::update_offshoots);
            update_offshoots();
        }
    }


    painter->save();
    QPen pen(opt->palette.color(QPalette::Highlight), 1);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->drawRect(d->cache);
    painter->restore();
}

QRectF graphics::TransformGraphicsItem::boundingRect() const
{
    qreal margin_left = d->handles[Private::Position].handle->isVisible() ? -d->offshoot_size() : 0;
    return d->cache.adjusted(margin_left, -d->offshoot_size(), 0, 0);
}

void graphics::TransformGraphicsItem::commit_anchor()
{
    d->target->document()->undo_stack().push(new command::SetMultipleAnimated(
        tr("Drag anchor point"),
        true,
        {&d->transform->anchor_point, &d->transform->position},
        d->transform->anchor_point.get(),
        d->transform->position.get()
    ));
}

void graphics::TransformGraphicsItem::commit_rot()
{
    d->push_command(d->transform->rotation, d->transform->rotation.value(), true);
}

void graphics::TransformGraphicsItem::commit_scale()
{
    d->push_command(d->transform->scale, d->transform->scale.value(), true);
}


void graphics::TransformGraphicsItem::set_transform_matrix(const QTransform& t)
{
    setTransform(t);
}

void graphics::TransformGraphicsItem::drag_pos_start(const QPointF &p)
{
    d->pos_trans = d->target->docnode_fuzzy_parent()->transform_matrix(d->target->time()).inverted();
    d->pos_drag_start = d->pos_trans.map(mapToScene(p));
    d->pos_start_value = d->transform->position.get();
}

void graphics::TransformGraphicsItem::drag_pos(const QPointF &p, Qt::KeyboardModifiers mod)
{
    auto sp = mapToScene(p);
    QPointF delta = d->pos_trans.map(sp) - d->pos_drag_start;
    if ( mod & Qt::ControlModifier )
    {
        if ( math::abs(delta.x()) < math::abs(delta.y()) )
            delta.setX(0);
        else
            delta.setY(0);
    }

    d->push_command(d->transform->position, d->pos_start_value + delta, false);
}

void graphics::TransformGraphicsItem::commit_pos()
{
    d->push_command(d->transform->position, d->transform->position.value(), true);
}

QVariant graphics::TransformGraphicsItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
    if ( change == ItemSceneHasChanged )
        update_offshoots();

    return QGraphicsItem::itemChange(change, value);
}
