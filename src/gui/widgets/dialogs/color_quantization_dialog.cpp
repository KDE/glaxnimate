/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "color_quantization_dialog.hpp"
#include "ui_color_quantization_dialog.h"

#include <QEvent>

#include "trace/quantize.hpp"
#include "glaxnimate_settings.hpp"

using namespace glaxnimate::gui;
using namespace glaxnimate;

class ColorQuantizationDialog::Private
{
public:
    Ui::ColorQuantizationDialog ui;
};

ColorQuantizationDialog::ColorQuantizationDialog(QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
}

ColorQuantizationDialog::~ColorQuantizationDialog() = default;

std::vector<QRgb> ColorQuantizationDialog::quantize(const QImage& image, int k) const
{
    switch ( d->ui.combo_algo->currentIndex() )
    {
        case 0:
            return utils::quantize::k_means(
                image,
                k,
                d->ui.spin_means_iterations->value(),
                utils::quantize::KMeansMatch(d->ui.combo_means_match->currentIndex())
           );
        case 1:
            return utils::quantize::k_modes(image, k);
        case 2:
            return utils::quantize::octree(image, k);
        case 3:
            return utils::quantize::edge_exclusion_modes(image, k, d->ui.spin_eem_min_frequency->value() / 100.);
    }
    return {};
}

void ColorQuantizationDialog::init_settings()
{
    d->ui.combo_algo->setCurrentIndex(GlaxnimateSettings::quantize_algo());
    d->ui.spin_means_iterations->setValue(GlaxnimateSettings::quantize_means_iterations());
    d->ui.combo_means_match->setCurrentIndex(GlaxnimateSettings::quantize_means_match());
    d->ui.spin_eem_min_frequency->setValue(GlaxnimateSettings::quantize_eem_min_frequency());
}

void ColorQuantizationDialog::save_settings()
{
    GlaxnimateSettings::setQuantize_algo(d->ui.combo_algo->currentIndex());
    GlaxnimateSettings::setQuantize_means_iterations(d->ui.spin_means_iterations->value());
    GlaxnimateSettings::setQuantize_means_match(d->ui.combo_means_match->currentIndex());
    GlaxnimateSettings::setQuantize_eem_min_frequency(d->ui.spin_eem_min_frequency->value());
}

void ColorQuantizationDialog::reset_settings()
{
    d->ui.combo_algo->setCurrentIndex(GlaxnimateSettings::defaultQuantize_algoValue());
    d->ui.spin_means_iterations->setValue(GlaxnimateSettings::defaultQuantize_means_iterationsValue());
    d->ui.combo_means_match->setCurrentIndex(GlaxnimateSettings::defaultQuantize_means_matchValue());
    d->ui.spin_eem_min_frequency->setValue(GlaxnimateSettings::defaultQuantize_eem_min_frequencyValue());
}
