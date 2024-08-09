/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef GRADIENTLISTWIDGET_H
#define GRADIENTLISTWIDGET_H

#include <memory>
#include <QWidget>

#include "widgets/dialogs/selection_manager.hpp"

namespace glaxnimate::model {
    class Document;
    class Fill;
    class Stroke;
    class BrushStyle;
    class Gradient;
} // namespace glaxnimate::model

namespace glaxnimate::gui {

class GradientListWidget : public QWidget
{
    Q_OBJECT

public:
    GradientListWidget(QWidget* parent = nullptr);
    ~GradientListWidget();

    void set_document(model::Document* doc);
    void set_targets(const std::vector<model::Fill*>& fills, const std::vector<model::Stroke*>& strokes);
    void set_current(model::Fill* fill, model::Stroke* stroke);
    void set_window(glaxnimate::gui::SelectionManager* window);

private Q_SLOTS:
    void change_current_gradient();

Q_SIGNALS:
    void selected(model::Gradient* gradient, bool secondary);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui

#endif // GRADIENTLISTWIDGET_H
