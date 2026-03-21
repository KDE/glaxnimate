/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "glaxnimate/io/utils.hpp"

std::vector<std::unique_ptr<glaxnimate::model::KeyframeBase>> glaxnimate::io::split_keyframes(model::AnimatableBase* prop)
{
    std::vector<std::unique_ptr<model::KeyframeBase>> split_kfs;
    if ( !prop->animated() )
        return split_kfs;

    auto range = prop->keyframe_range();
    std::unique_ptr<model::KeyframeBase> previous = range.begin()->clone();

    auto kf = range.begin();
    for ( ++kf; kf != range.end(); ++kf )
    {
        if ( previous->transition().hold() )
        {
            split_kfs.push_back(std::move(previous));
            previous = kf->clone();
            continue;
        }

        std::array<qreal, 2> raw_splits;
        std::tie(raw_splits[0], raw_splits[1]) = previous->transition().bezier().extrema(1);

        std::vector<qreal> splits;
        for ( qreal t : raw_splits )
        {
            QPointF p = previous->transition().bezier().solve(t);
            if ( p.y() < 0 || p.y() > 1 )
                splits.push_back(t);
        }

        if ( splits.size() == 0 )
        {
            split_kfs.push_back(std::move(previous));
            previous = kf->clone();
        }
        else
        {
            auto next = kf;
            auto next_segment = previous->split(&*next, splits);
            previous = std::move(next_segment.back());
            split_kfs.insert(split_kfs.end(), std::make_move_iterator(next_segment.begin()), std::make_move_iterator(next_segment.end() - 1));
        }
    }

    split_kfs.push_back(std::move(previous));

    return split_kfs;
}
