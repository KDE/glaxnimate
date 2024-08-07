#include "swatchesdock.h"

#include "app/log/log_model.hpp"
#include "style/better_elide_delegate.hpp"
#include "ui_swatches.h"

using namespace glaxnimate::gui;

class SwatchesDock::Private
{
public:
    ::Ui::dock_swatches ui;
};

SwatchesDock::SwatchesDock(GlaxnimateWindow *parent, color_widgets::ColorPaletteModel* palette_model)
    : QDockWidget(parent)
    , d(std::make_unique<Private>())
{
    d->ui.setupUi(this);

    d->ui.document_swatch_widget->set_palette_model(palette_model);

    connect(d->ui.document_swatch_widget, &DocumentSwatchWidget::needs_new_color, this, &SwatchesDock::needs_new_color);
    connect(d->ui.document_swatch_widget, &DocumentSwatchWidget::current_color_def, this, &SwatchesDock::current_color_def);
    connect(d->ui.document_swatch_widget, &DocumentSwatchWidget::secondary_color_def, this, &SwatchesDock::secondary_color_def);
}

SwatchesDock::~SwatchesDock() = default;

void SwatchesDock::add_new_color(const QColor& color)
{
    d->ui.document_swatch_widget->add_new_color(color);
}

void SwatchesDock::clear_document()
{
    d->ui.document_swatch_widget->set_document(nullptr);
}

void SwatchesDock::set_document(model::Document* document)
{
    d->ui.document_swatch_widget->set_document(document);
}
