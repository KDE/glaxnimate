/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/model/animation/keyframe_base.hpp"

std::vector<std::unique_ptr<glaxnimate::model::KeyframeBase>> glaxnimate::model::KeyframeBase::split(const KeyframeBase* other, std::vector<qreal> splits) const
{
    std::vector<std::unique_ptr<KeyframeBase>> kfs;
    if ( transition().hold() )
    {
        kfs.push_back(clone());
        kfs.push_back(other->clone());
        return kfs;
    }

    auto splitter = this->splitter(other);

    kfs.reserve(splits.size()+2);
    qreal prev_split = 0;
    const KeyframeBase* to_split = this;
    std::unique_ptr<KeyframeBase> split_right;
    QPointF old_p;
    for ( qreal split : splits )
    {
        // Skip zeros
        if ( qFuzzyIsNull(split) )
            continue;

        qreal split_ratio = (split - prev_split) / (1 - prev_split);
        prev_split = split;
        auto transitions = to_split->transition().split_t(split_ratio);
        // split_ratio is t
        // p.x() is time lerp
        // p.y() is value lerp
        QPointF p = transition().bezier().solve(split);
        splitter->step(p);
        auto split_left = splitter->left(old_p);
        split_left->set_transition(transitions.first);
        old_p = p;
        split_right = splitter->right(p);
        split_right->set_transition(transitions.second);
        to_split = split_right.get();
        kfs.push_back(std::move(split_left));
    }
    kfs.push_back(std::move(split_right));
    kfs.push_back(splitter->last());
    kfs.back()->set_transition(other->transition());

    return kfs;
}

