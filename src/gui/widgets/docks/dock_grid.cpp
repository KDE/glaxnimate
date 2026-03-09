/*
 * SPDX-FileCopyrightText: 2012-2026 Mattia Basaglia <dev@dragon.best>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "dock_grid.hpp"


#include "ui_dock_grid.h"
#include "glaxnimate_settings.hpp"


using namespace glaxnimate::gui;

class DockGrid::Private : public Ui::DockGrid
{
public:
    SnappingGrid* target = nullptr;
};

DockGrid::DockGrid(QWidget *parent) :
    QDockWidget(parent), d(std::make_unique<Private>())
{
    d->setupUi(this);
    connect(d->button_move,SIGNAL(clicked()),SIGNAL(move_grid()));
    d->button_move->setVisible(false);
}

glaxnimate::gui::DockGrid::~DockGrid() = default;


void DockGrid::set_grid(SnappingGrid *target_grid)
{

    d->target = target_grid;

    if ( d->target )
    {
        d->spin_size->setValue(d->target->size());
        connect(d->spin_size,SIGNAL(valueChanged(int)),d->target,SLOT(set_size(int)));


        d->combo_shape->setCurrentIndex(d->target->shape());

        d->check_enable->setChecked(d->target->is_enabled());
        connect(d->check_enable,SIGNAL(clicked(bool)),d->target,SLOT(enable(bool)));
        connect(d->target,SIGNAL(enabled(bool)),d->check_enable,SLOT(setChecked(bool)));

        d->spin_x->blockSignals(true);
        d->spin_x->setValue(d->target->origin().x());
        d->spin_x->blockSignals(false);
        d->spin_y->blockSignals(true);
        d->spin_y->setValue(d->target->origin().y());
        d->spin_y->blockSignals(false);

        connect(d->target,SIGNAL(moved(QPointF)),SLOT(grid_moved(QPointF)));


        setEnabled(true);
    }
    else
    {
        d->check_enable->setChecked(false);
        setEnabled(false);
    }
}

void DockGrid::unset_grid(SnappingGrid *grid)
{
    if ( d->target == grid )
    {
        grid->disconnect(this);
        disconnect(grid);
        d->check_enable->disconnect(grid);
        grid->disconnect(d->check_enable);
        d->spin_size->disconnect(grid);
        set_grid( nullptr );
    }
}

void DockGrid::changeEvent(QEvent *e)
{
    QDockWidget::changeEvent(e);
    switch (e->type()) {
        case QEvent::LanguageChange:
            d->retranslateUi(this);
            break;
        default:
            break;
    }
}

void DockGrid::position_spin_changed()
{
    if ( d->target )
        d->target->set_origin(QPointF(d->spin_x->value(), d->spin_y->value()));
    GlaxnimateSettings::self()->setGrid_origin_x(d->spin_x->value());
    GlaxnimateSettings::self()->setGrid_origin_y(d->spin_y->value());
}

void DockGrid::grid_moved(QPointF p)
{
    d->spin_x->setValue(p.x());
    d->spin_y->setValue(p.y());
}

void DockGrid::on_button_reset_clicked()
{
    d->spin_x->setValue(0);
    d->spin_y->setValue(0);
}

void DockGrid::on_combo_shape_currentIndexChanged(int index)
{
    if ( d->target )
        d->target->set_shape(SnappingGrid::GridShape(index));
    GlaxnimateSettings::self()->setGrid_shape(int(d->target->shape()));
}

void DockGrid::on_check_enable_toggled(bool arg1)
{
    if ( d->target )
        d->target->enable(arg1);
    GlaxnimateSettings::self()->setGrid_enabled(arg1);
}

void glaxnimate::gui::DockGrid::on_spin_size_valueChanged(int size)
{
    if ( d->target )
        d->target->set_size(size);
    GlaxnimateSettings::self()->setGrid_size(size);
}
