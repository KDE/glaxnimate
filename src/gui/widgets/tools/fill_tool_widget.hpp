/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef FILLTOOLWIDGET_H
#define FILLTOOLWIDGET_H

#include <memory>
#include "shape_tool_widget.hpp"

namespace glaxnimate::gui {

class FillToolWidget : public ToolWidgetBase
{
    Q_OBJECT

public:
    FillToolWidget(QWidget* parent = nullptr);
    ~FillToolWidget();

    bool fill() const;
    bool stroke() const;

    void swap_fill_color();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui

#endif // FILLTOOLWIDGET_H
