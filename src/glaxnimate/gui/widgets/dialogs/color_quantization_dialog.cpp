#include "color_quantization_dialog.hpp"
#include "ui_color_quantization_dialog.h"

#include <QEvent>

#include "glaxnimate/trace/quantize.hpp"

#include "app/settings/widget.hpp"

using namespace glaxnimate::gui;
using namespace glaxnimate;

class ColorQuantizationDialog::Private
{
public:
    Ui::ColorQuantizationDialog ui;
    app::settings::WidgetSettingGroup settings;
};

ColorQuantizationDialog::ColorQuantizationDialog(QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
}

ColorQuantizationDialog::~ColorQuantizationDialog() = default;

void ColorQuantizationDialog::changeEvent ( QEvent* e )
{
    QDialog::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

std::vector<QRgb> ColorQuantizationDialog::get_palette(trace::SegmentedImage image, int k) const
{
    switch ( d->ui.combo_algo->currentIndex() )
    {
        case 0:
            return trace::k_means(
                image,
                k,
                d->ui.spin_means_iterations->value(),
                trace::KMeansMatch(d->ui.combo_means_match->currentIndex())
           );
        case 1:
            return trace::k_modes(image, k);
        case 2:
            return trace::octree(image, k);
        case 3:
            return trace::edge_exclusion_modes(image, k, d->ui.spin_eem_min_frequency->value() / 100.);
    }
    return {};
}

void ColorQuantizationDialog::init_settings()
{
    d->settings.add(d->ui.combo_algo, "internal", "color_quantization_dialog_");
    d->settings.add(d->ui.spin_means_iterations, "internal", "color_quantization_dialog_");
    d->settings.add(d->ui.combo_means_match, "internal", "color_quantization_dialog_");
    d->settings.add(d->ui.spin_eem_min_frequency, "internal", "color_quantization_dialog_");
    d->settings.define();
}

void ColorQuantizationDialog::save_settings()
{
    d->settings.save();
}

void ColorQuantizationDialog::reset_settings()
{
    d->settings.reset();
}
