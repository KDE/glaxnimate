/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#include "shape_tool_widget.hpp"
#include "ui_shape_tool_widget.h"

#include "glaxnimate_settings.hpp"

class glaxnimate::gui::ShapeToolWidget::Private
{
public:

    virtual ~Private() = default;

    void load_settings()
    {
        ui.check_group->setChecked(GlaxnimateSettings::shape_group());
        ui.check_layer->setChecked(GlaxnimateSettings::shape_layer());
        ui.check_raw_shape->setChecked(GlaxnimateSettings::shape_raw_shape());
        ui.check_fill->setChecked(GlaxnimateSettings::shape_fill());
        ui.check_stroke->setChecked(GlaxnimateSettings::shape_stroke());

        check_checks();
        on_load_settings();
    }

    void save_settings()
    {
        GlaxnimateSettings::setShape_group(ui.check_group->isChecked());
        GlaxnimateSettings::setShape_layer(ui.check_layer->isChecked());
        GlaxnimateSettings::setShape_raw_shape(ui.check_raw_shape->isChecked());
        GlaxnimateSettings::setShape_fill(ui.check_fill->isChecked());
        GlaxnimateSettings::setShape_fill(ui.check_fill->isChecked());

        on_save_settings();
    }

    void check_checks()
    {
        if ( !ui.check_group->isChecked() && !ui.check_layer->isChecked() )
        {
            if ( ui.check_fill->isEnabled() )
            {
                old_check_fill = ui.check_fill->isChecked();
                ui.check_fill->setEnabled(false);
                ui.check_fill->setChecked(false);

                old_check_stroke = ui.check_stroke->isChecked();
                ui.check_stroke->setEnabled(false);
                ui.check_stroke->setChecked(false);
            }
        }
        else if ( !ui.check_fill->isEnabled() )
        {
            ui.check_fill->setEnabled(true);
            ui.check_fill->setChecked(old_check_fill);

            ui.check_stroke->setEnabled(true);
            ui.check_stroke->setChecked(old_check_stroke);
        }
    }

    void setup_ui(ShapeToolWidget* parent)
    {
        ui.setupUi(parent);
        on_setup_ui(parent, ui.layout);
    }

    void retranslate(ShapeToolWidget* parent)
    {
        ui.retranslateUi(parent);
        on_retranslate();
    }

    bool create_fill() const
    {
        return ui.check_fill->isChecked();
    }

    bool create_layer() const
    {
        return ui.check_layer->isChecked();
    }

    bool create_group() const
    {
        return ui.check_group->isChecked();
    }

    bool create_stroke() const
    {
        return ui.check_stroke->isChecked();
    }

protected:
    virtual void on_load_settings() {}
    virtual void on_save_settings() {}
    virtual void on_setup_ui(ShapeToolWidget*, QVBoxLayout*) {}
    virtual void on_retranslate() {}

private:
    Ui::ShapeToolWidget ui;
    bool old_check_fill;
    bool old_check_stroke;
};
