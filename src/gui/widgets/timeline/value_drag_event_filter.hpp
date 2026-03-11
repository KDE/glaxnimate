/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QAbstractItemView>

namespace glaxnimate::gui {

class ValueDragEventFilter : public QObject
{
public:
    ValueDragEventFilter(QAbstractItemView* view);
    ~ValueDragEventFilter();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui
