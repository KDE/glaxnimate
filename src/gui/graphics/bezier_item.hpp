/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <set>

#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include "handle.hpp"
#include "model/shapes/path.hpp"
#include "utils/pseudo_mutex.hpp"
#include "typed_item.hpp"

namespace glaxnimate::gui::graphics {

class PointItem : public QGraphicsObject
{
    Q_OBJECT
public:
    PointItem(int index, const math::bezier::Point& point, QGraphicsItem* parent, model::AnimatableBase* property);

    QRectF boundingRect() const override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *) override;

    int index() const;

    void set_point_type(math::bezier::PointType type);

    void show_tan_in(bool show);

    void show_tan_out(bool show);

    void remove_tangent(MoveHandle* handle);

    const math::bezier::Point& point() const;
    void modify(const math::bezier::Point& pt, const QString& undo_name);

    class BezierItem* parent_editor() const;

    bool tan_in_empty() const;
    bool tan_out_empty() const;

    void set_selected(bool selected);

Q_SIGNALS:
    void modified(int index, const math::bezier::Point& point, bool commit, const QString& name);

private Q_SLOTS:
    void tan_in_dragged(const QPointF& p, Qt::KeyboardModifiers mods);

    void tan_out_dragged(const QPointF& p, Qt::KeyboardModifiers mods);

    void pos_dragged(const QPointF& p);

    void on_modified(bool commit, const QString& name="");
    void on_commit();

private:
    void drag_preserve_angle(QPointF& dragged, QPointF& other, const QPointF& dragged_new);

    void set_point(const math::bezier::Point& p);
    void set_index(int index);
    void set_has_tan_in(bool show);
    void set_has_tan_out(bool show);


    MoveHandle pos{nullptr, MoveHandle::Any, MoveHandle::Diamond, 8};
    MoveHandle tan_in{nullptr, MoveHandle::Any, MoveHandle::Circle, 6};
    MoveHandle tan_out{nullptr, MoveHandle::Any, MoveHandle::Circle, 6};

    int index_;
    math::bezier::Point point_;
    bool has_tan_in = true;
    bool has_tan_out = true;
    bool shows_tan_in = true;
    bool shows_tan_out = true;
    friend class BezierItem;
};


class BezierItem : public TypedItem<Types::BezierItem>
{
    Q_OBJECT

public:
    BezierItem(model::AnimatedProperty<math::bezier::Bezier>* property, QGraphicsItem* parent=nullptr);
    BezierItem(model::AnimatedProperty<QPointF>* property, model::VisualNode* target_object = nullptr, QGraphicsItem* parent=nullptr);

    QRectF boundingRect() const override;

    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget*) override;

    void set_type(int index, math::bezier::PointType type);

    model::AnimatableBase* target_property() const;
    model::AnimatedProperty<math::bezier::Bezier>* target_bezier_property() const;
    model::AnimatedProperty<QPointF>* target_position_property() const;
    model::VisualNode* target_object() const;

    const std::set<int>& selected_indices();
    void clear_selected_indices();
    void select_index(int i);
    void deselect_index(int i);
    void toggle_index(int i);

    const math::bezier::Bezier& bezier() const;

    void split_segment(int index, qreal factor);

public Q_SLOTS:
    /**
     * \brief Updates the bezier without updating the property
     */
    void update_bezier(const math::bezier::Bezier& bez);

    /**
     * \brief Updates the bezier and updates the property
     */
    void set_bezier(const math::bezier::Bezier& bez, bool commit = true);

    void remove_point(int index);

    void make_first(int index);

private Q_SLOTS:
    void on_dragged(int index, const math::bezier::Point& point, bool commit, const QString& name);
    void refresh_from_position_property();

private:
    void do_update(bool commit, const QString& name);
    void do_add_point(int index);

    math::bezier::Bezier bezier_;
    std::vector<std::unique_ptr<PointItem>> items;
    model::AnimatedProperty<math::bezier::Bezier>* property_bezier = nullptr;
    model::AnimatedProperty<QPointF>* property_pos = nullptr;
    utils::PseudoMutex updating;
    std::set<int> selected_indices_;
    model::VisualNode* target_object_;
};


} // namespace glaxnimate::gui::graphics
