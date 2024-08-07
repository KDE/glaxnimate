/*
 * SPDX-FileCopyrightText: 2024 Julius Künzel <julius.kuenzel@kde.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "colorsdock.h"

#include "ui_colors.h"

using namespace glaxnimate::gui;

class ColorsDock::Private
{
public:
    ::Ui::dock_colors ui;
};

ColorsDock::ColorsDock(GlaxnimateWindow *parent)
    : QDockWidget(parent)
    , d(std::make_unique<Private>())
{
    d->ui.setupUi(this);

    connect(d->ui.fill_style_widget, &FillStyleWidget::current_color_changed,
            this, &ColorsDock::current_color_changed);

    connect(d->ui.fill_style_widget, &FillStyleWidget::secondary_color_changed,
            this, &ColorsDock::secondary_color_changed);
}

ColorsDock::~ColorsDock() = default;

void ColorsDock::clear_document()
{
    d->ui.fill_style_widget->set_targets({});
    d->ui.fill_style_widget->set_current(nullptr);
}

void ColorsDock::save_settings() const
{
    d->ui.fill_style_widget->save_settings();
}

glaxnimate::model::Fill * ColorsDock::current() const
{
    return d->ui.fill_style_widget->current();
}

void ColorsDock::set_current(glaxnimate::model::Fill* fill)
{
    return d->ui.fill_style_widget->set_current(fill);
}

void ColorsDock::set_targets(const std::vector<model::Fill*>& new_targets)
{
    return d->ui.fill_style_widget->set_targets(new_targets);
}

QColor ColorsDock::current_color() const
{
    return d->ui.fill_style_widget->current_color();
}

void ColorsDock::set_current_color(const QColor& c)
{
    return d->ui.fill_style_widget->set_current_color(c);
}

QColor ColorsDock::secondary_color() const
{
    return d->ui.fill_style_widget->secondary_color();
}

void ColorsDock::set_palette_model(color_widgets::ColorPaletteModel* palette_model)
{
    d->ui.fill_style_widget->set_palette_model(palette_model);
}

void ColorsDock::set_gradient_stop(model::Styler* styler, int index)
{
    d->ui.fill_style_widget->set_gradient_stop(styler, index);
}
