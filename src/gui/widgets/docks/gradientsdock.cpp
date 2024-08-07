#include "gradientsdock.h"

#include "app/log/log_model.hpp"
#include "style/better_elide_delegate.hpp"
#include "ui_gradients.h"

using namespace glaxnimate::gui;

class GradientsDock::Private
{
public:
    ::Ui::dock_gradients ui;
};

GradientsDock::GradientsDock(GlaxnimateWindow *parent)
    : QDockWidget(parent)
    , d(std::make_unique<Private>())
{
    d->ui.setupUi(this);

    d->ui.widget_gradients->set_window(parent);

    connect(d->ui.widget_gradients, &GradientListWidget::selected, this, &GradientsDock::selected);
}

GradientsDock::~GradientsDock() = default;

void GradientsDock::clear_document()
{
    d->ui.widget_gradients->set_document(nullptr);
}

void GradientsDock::set_document(model::Document* document)
{
    d->ui.widget_gradients->set_document(document);
}

void GradientsDock::set_current(model::Fill* fill, model::Stroke* stroke)
{
    d->ui.widget_gradients->set_current(fill, stroke);
}

void GradientsDock::set_targets(const std::vector<model::Fill*>& fills, const std::vector<model::Stroke*>& strokes)
{
    d->ui.widget_gradients->set_targets(fills, strokes);
}
