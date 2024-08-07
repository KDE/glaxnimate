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
#include <QtColorWidgets/color_palette_model.hpp>

namespace glaxnimate::gui {

class SwatchesDock : public QDockWidget
{
    Q_OBJECT

public:
    SwatchesDock(GlaxnimateWindow* parent, color_widgets::ColorPaletteModel* palette_model);

    ~SwatchesDock();

    void add_new_color(const QColor& color);

    void clear_document();
    void set_document(model::Document* document);

Q_SIGNALS:
    void current_color_def(model::BrushStyle* def);
    void secondary_color_def(model::BrushStyle* def);
    void needs_new_color();

private:
    class Private;
    std::unique_ptr<Private> d;
};

}
