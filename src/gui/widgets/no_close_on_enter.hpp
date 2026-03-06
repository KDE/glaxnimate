/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QKeyEvent>

namespace glaxnimate::gui {

class NoCloseOnEnter : public QObject
{
protected:
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if ( keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter )
                return true;
        }

        return QObject::eventFilter(obj, event);
    }
};


} // namespace glaxnimate::gui
