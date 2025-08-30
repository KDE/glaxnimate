/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QAbstractButton>


class QTabBar;

namespace glaxnimate::gui {

class TabBarCloseButton : public QAbstractButton
{
    Q_OBJECT

public:
    static void add_button(QTabBar* bar, int index);

    explicit TabBarCloseButton(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void enterEvent(QEnterEvent * event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
};

} // namespace glaxnimate::gui
