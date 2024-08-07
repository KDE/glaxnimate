/*
 * SPDX-FileCopyrightText: 2024 Julius Künzel <julius.kuenzel@kde.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

# pragma once

#include "QtColorWidgets/color_palette_model.hpp"
#include "model/shapes/fill.hpp"
#include "widgets/dialogs/glaxnimate_window.hpp"
#include <QDockWidget>
#include <QObject>

namespace glaxnimate::gui {

class ColorsDock : public QDockWidget
{
    Q_OBJECT

public:
    ColorsDock(GlaxnimateWindow* parent);

    ~ColorsDock();

    void clear_document();
    void save_settings() const;

    glaxnimate::model::Fill * current() const;
    void set_current(glaxnimate::model::Fill* fill);

    void set_targets(const std::vector<model::Fill*>& new_targets);

    QColor current_color() const;
    void set_current_color(const QColor& c);
    QColor secondary_color() const;

    void set_palette_model(color_widgets::ColorPaletteModel* palette_model);

public Q_SLOTS:
    void set_gradient_stop(model::Styler* styler, int index);

Q_SIGNALS:
    void current_color_changed(const QColor& c);
    void secondary_color_changed(const QColor& c);

private:
    class Private;
    std::unique_ptr<Private> d;
};

}
