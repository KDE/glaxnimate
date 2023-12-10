/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "named_color.hpp"

#include "model/document.hpp"
#include "model/assets/assets.hpp"
#include "command/object_list_commands.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::NamedColor)

QString glaxnimate::model::NamedColor::type_name_human() const
{
    return i18n("Unnamed Color");
}

QBrush glaxnimate::model::NamedColor::brush_style(FrameTime t) const
{
    return color.get_at(t);
}

void glaxnimate::model::NamedColor::fill_icon(QPixmap& icon) const
{
    icon.fill(color.get_at(0));
}

bool glaxnimate::model::NamedColor::remove_if_unused(bool clean_lists)
{
    if ( clean_lists && users().empty() )
    {
        document()->push_command(new command::RemoveObject(
            this,
            &document()->assets()->colors->values
        ));
        return true;
    }
    return false;
}
