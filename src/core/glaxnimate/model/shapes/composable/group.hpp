/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "glaxnimate/model/shapes/composable/composable.hpp"

namespace glaxnimate::model {


class Group : public StaticOverrides<Group, Composable>
{
    GLAXNIMATE_OBJECT(Group)

public:

    GLAXNIMATE_PROPERTY_LIST(ShapeElement, shapes)

public:
    using Ctor::Ctor;

    int docnode_child_count() const override { return shapes.size(); }
    DocumentNode* docnode_child(int index) const override { return shapes[index]; }
    int docnode_child_index(DocumentNode* obj) const override { return shapes.index_of(static_cast<ShapeElement*>(obj)); }

    static QIcon static_tree_icon()
    {
        return QIcon::fromTheme("object-group");
    }

    static QString static_type_name_human()
    {
        return i18n("Group");
    }

    void add_shapes(model::FrameTime t, math::bezier::MultiBezier & bez, const QTransform& transform) const override;

    QRectF local_bounding_rect(FrameTime t) const override;

    QTransform local_transform_matrix(model::FrameTime t) const override;

    glaxnimate::math::bezier::MultiBezier to_clip(model::FrameTime t) const override;

    std::unique_ptr<ShapeElement> to_path() const override;

protected:
    glaxnimate::math::bezier::MultiBezier to_painter_path_impl(model::FrameTime t) const override;
    void on_graphics_changed() override;
    void on_composition_changed(model::Composition* old_comp, model::Composition* new_comp) override;
};

} // namespace glaxnimate::model
