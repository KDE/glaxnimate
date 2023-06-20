/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef EMOJI_DATA_HPP
#define EMOJI_DATA_HPP

#include <vector>
#include <QString>

namespace glaxnimate::emoji {

struct Emoji
{
    QString name;
    QString unicode;
    QString hex_slug;
};

struct EmojiSubGroup
{
    QString name;
    std::vector<Emoji> emoji;
};

struct EmojiGroup
{
    QString name;
    std::vector<const EmojiSubGroup*> children;

    static const std::vector<const EmojiGroup*> table;

    const Emoji& first() const
    {
        return children[0]->emoji[0];
    }
};

} // glaxnimate::emoji



#endif // EMOJI_DATA_HPP
