#include "strokedock.h"

#include "ui_stroke.h"

using namespace glaxnimate::gui;

class StrokeDock::Private
{
public:
    ::Ui::dock_stroke ui;
};

StrokeDock::StrokeDock(GlaxnimateWindow *parent)
    : QDockWidget(parent)
    , d(std::make_unique<Private>())
{
    d->ui.setupUi(this);

    connect(d->ui.stroke_style_widget, &StrokeStyleWidget::color_changed,
            this, &StrokeDock::color_changed);

    connect(d->ui.stroke_style_widget, &StrokeStyleWidget::pen_style_changed,
            this, &StrokeDock::pen_style_changed);
}

StrokeDock::~StrokeDock() = default;

void StrokeDock::clear_document()
{
    d->ui.stroke_style_widget->set_targets({});
    d->ui.stroke_style_widget->set_current(nullptr);
}

void StrokeDock::save_settings() const
{
    d->ui.stroke_style_widget->save_settings();
}

glaxnimate::model::Stroke * StrokeDock::current() const
{
    return d->ui.stroke_style_widget->current();
}

void StrokeDock::set_current(model::Stroke* stroke)
{
    return d->ui.stroke_style_widget->set_current(stroke);
}

void StrokeDock::set_targets(std::vector<model::Stroke *> targets)
{
    return d->ui.stroke_style_widget->set_targets(targets);
}

QColor StrokeDock::current_color() const
{
    return d->ui.stroke_style_widget->current_color();
}

void StrokeDock::set_color(const QColor& color)
{
    return d->ui.stroke_style_widget->set_color(color);
}

QPen StrokeDock::pen_style() const
{
    return d->ui.stroke_style_widget->pen_style();
}

void StrokeDock::set_palette_model(color_widgets::ColorPaletteModel* palette_model)
{
    d->ui.stroke_style_widget->set_palette_model(palette_model);
}

void StrokeDock::set_gradient_stop(model::Styler* styler, int index)
{
    d->ui.stroke_style_widget->set_gradient_stop(styler, index);
}
