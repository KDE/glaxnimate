/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "glaxnimate/model/animation/animatable.hpp"
#include "glaxnimate/model/object.hpp"

#include <QTransform>
#include <QIcon>


namespace glaxnimate::model {


class Transform : public Object
{
    GLAXNIMATE_OBJECT(Transform)
    GLAXNIMATE_ANIMATABLE(QPointF, anchor_point, QPointF(0, 0))
    GLAXNIMATE_ANIMATABLE(QPointF, position, QPointF(0, 0))
    GLAXNIMATE_ANIMATABLE(QVector2D, scale, QVector2D(1, 1))
    GLAXNIMATE_ANIMATABLE(float, rotation, 0, {})
    GLAXNIMATE_PROPERTY(bool, auto_orient, false, {}, {}, PropertyTraits::Visual|PropertyTraits::Hidden)

public:
    using Object::Object;

    virtual QIcon tree_icon() const override { return QIcon::fromTheme("node-transform"); }
    virtual QString type_name_human() const override { return i18n("Transform"); }

    QTransform transform_matrix(FrameTime f) const;
    void set_transform_matrix(const QTransform& t);
    /**
     * \brief Returns the transform at the given time using the given anchor point instead of the transform's
     */
    QTransform transform_matrix_with_anchor(FrameTime f, const QPointF& anchor) const;
    void copy(Transform* other);


};



} // namespace glaxnimate::model
