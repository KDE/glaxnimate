/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "clickable_tab_bar.hpp"

#include <QMouseEvent>

using namespace glaxnimate::gui;


void ClickableTabBar::mouseReleaseEvent(QMouseEvent* event)
{
    QTabBar::mouseReleaseEvent(event);

    int index = tabAt(event->pos());

    if ( index != -1 )
    {
        if ( event->button() == Qt::MiddleButton )
            emit tabCloseRequested(index);
        else if ( event->button() == Qt::RightButton )
            emit context_menu_requested(index);
    }
}


