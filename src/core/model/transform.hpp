/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "model/animation/animatable.hpp"
#include "model/object.hpp"

#include <QTransform>


namespace glaxnimate::model {


class Transform : public Object
{
    GLAXNIMATE_OBJECT(Transform)
    GLAXNIMATE_ANIMATABLE(QPointF, anchor_point, QPointF(0, 0))
    GLAXNIMATE_ANIMATABLE(QPointF, position, QPointF(0, 0))
    GLAXNIMATE_ANIMATABLE(QVector2D, scale, QVector2D(1, 1))
    GLAXNIMATE_ANIMATABLE(float, rotation, 0, {})

public:
    using Object::Object;

    virtual QString type_name_human() const override { return tr("Transform"); }

    QTransform transform_matrix(FrameTime f, bool auto_orient = false) const;
    void set_transform_matrix(const QTransform& t);
    void copy(Transform* other);
};



} // namespace glaxnimate::model
