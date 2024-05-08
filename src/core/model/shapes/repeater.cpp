/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "repeater.hpp"

#include <QPainter>

#include "model/shapes/group.hpp"
#include "model/animation/join_animatables.hpp"

using namespace glaxnimate;

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::Repeater)

QIcon glaxnimate::model::Repeater::static_tree_icon()
{
    return QIcon::fromTheme("table");
}

QString glaxnimate::model::Repeater::static_type_name_human()
{
    return i18n("Repeater");
}


math::bezier::MultiBezier glaxnimate::model::Repeater::process(FrameTime t, const math::bezier::MultiBezier& mbez) const
{
    QTransform matrix = transform->transform_matrix(t);
    math::bezier::MultiBezier out;
    math::bezier::MultiBezier copy = mbez;
    for ( int i = 0; i < copies.get_at(t); i++ )
    {
        out.append(copy);
        copy.transform(matrix);
    }
    return out;

}

bool glaxnimate::model::Repeater::process_collected() const
{
    return true;
}

void glaxnimate::model::Repeater::on_paint(QPainter* painter, glaxnimate::model::FrameTime t, glaxnimate::model::VisualNode::PaintMode mode, glaxnimate::model::Modifier*) const
{
    QTransform matrix = transform->transform_matrix(t);
    auto alpha_s = start_opacity.get_at(t);
    auto alpha_e = end_opacity.get_at(t);
    int n_copies = copies.get_at(t);

    auto initial_opacity = painter->opacity();

    for ( int i = 0; i < n_copies; i++ )
    {
        float alpha_lerp = float(i) / (n_copies == 1 ? 1 : n_copies - 1);
        auto alpha = math::lerp(alpha_s, alpha_e, alpha_lerp);
        painter->setOpacity(alpha * initial_opacity);

        for ( auto sib : affected() )
        {
            if ( sib->visible.get() )
                sib->paint(painter, t, mode);
        }

        painter->setTransform(matrix, true);
    }
}


template<class T, class Func = std::plus<T>>
static void increase_transform(glaxnimate::model::AnimatedProperty<T>& into, const glaxnimate::model::AnimatedProperty<T>& from, Func func = {})
{
    using Keyframe = glaxnimate::model::Keyframe<T>;

    for ( int i = 0, e = from.keyframe_count(); i < e; i++ )
    {
        auto into_kf = static_cast<Keyframe*>(into.keyframe(i));

        into_kf->set(func(into_kf->get(), static_cast<const Keyframe*>(from.keyframe(i))->get()));
    }

    into.set(func(into.get(), from.get()));
}

static void increase_transform(glaxnimate::model::Transform* into, const glaxnimate::model::Transform* from)
{
    increase_transform(into->position, from->position);
    increase_transform(into->anchor_point, from->anchor_point);
    increase_transform(into->rotation, from->rotation);
    increase_transform(into->scale, from->scale, [](const QVector2D& a, const QVector2D& b){
        return QVector2D(a.x() * b.x(), a.y() * b.y());
    });
}

std::unique_ptr<glaxnimate::model::ShapeElement> glaxnimate::model::Repeater::to_path() const
{
    auto group = std::make_unique<glaxnimate::model::Group>(document());
    group->name.set(name.get());
    group->visible.set(visible.get());
    group->locked.set(locked.get());

    auto child = std::make_unique<glaxnimate::model::Group>(document());
    for ( auto sib : affected() )
        child->shapes.insert(sib->to_path());

    JoinAnimatables anim({&start_opacity, &end_opacity}, JoinAnimatables::NoValues);

    int n_copies = copies.get();
    float alpha_lerp = 1;
    auto func = [&alpha_lerp](float a, float b){
        return math::lerp(a, b, alpha_lerp);
    };
    for ( int i = 0; i < n_copies; i++ )
    {
        alpha_lerp = float(i) / (n_copies == 1 ? 1 : n_copies - 1);
        auto cloned = static_cast<glaxnimate::model::Group*>(group->shapes.insert_clone(child.get()));
        anim.apply_to(&cloned->opacity, func, &start_opacity, &end_opacity);


        if ( i == 0 )
            child->transform->assign_from(transform.get());
        else
            increase_transform(child->transform.get(), transform.get());

    }

    group->set_time(time());

    return group;
}

int glaxnimate::model::Repeater::max_copies() const
{
    int max = copies.get();
    for ( int i = 0, e = copies.keyframe_count(); i < e; ++i )
    {
        int val = copies.keyframe(i)->get();
        if ( val > max )
            max = val;
    }
    return max;
}
