/*
 * SPDX-FileCopyrightText: 2024 Julius Künzel <julius.kuenzel@kde.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

# pragma once

#include "model/assets/gradient.hpp"
#include "model/shapes/fill.hpp"
#include "model/shapes/stroke.hpp"
#include "widgets/dialogs/glaxnimate_window.hpp"
#include <QDockWidget>
#include <QObject>

namespace glaxnimate::gui {

class GradientsDock : public QDockWidget
{
    Q_OBJECT

public:
    GradientsDock(GlaxnimateWindow* parent);

    ~GradientsDock();

    void clear_document();
    void set_document(model::Document* document);
    void set_current(model::Fill* fill, model::Stroke* stroke);
    void set_targets(const std::vector<model::Fill*>& fills, const std::vector<model::Stroke*>& strokes);

Q_SIGNALS:
    void selected(model::Gradient* gradient, bool secondary);

private:
    class Private;
    std::unique_ptr<Private> d;
};

}
