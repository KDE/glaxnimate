/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "font_size_widget.hpp"
#include "ui_font_size_widget.h"

#include <QEvent>
#include <QStandardItemModel>
#include <QSignalBlocker>
#include <QFontDatabase>

class glaxnimate::gui::font::FontSizeWidget::Private
{
public:
    Ui_FontSizeWidget ui;
    QStandardItemModel sizes_model;
    QList<int> standard_sizes;
};

glaxnimate::gui::font::FontSizeWidget::FontSizeWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);

    d->standard_sizes = QFontDatabase::standardSizes();
    for ( int s : d->standard_sizes )
        d->sizes_model.appendRow(new QStandardItem(QString::number(s)));
    d->ui.view_size->setModel(&d->sizes_model);

    connect(d->ui.view_size->selectionModel(), &QItemSelectionModel::currentChanged, this, &FontSizeWidget::size_selected);
    connect(d->ui.spin_size, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &FontSizeWidget::size_edited);
}

glaxnimate::gui::font::FontSizeWidget::~FontSizeWidget() = default;

void glaxnimate::gui::font::FontSizeWidget::set_font_size(qreal size)
{
    d->ui.spin_size->setValue(size);
    size_edited(size);
}


void glaxnimate::gui::font::FontSizeWidget::size_edited(double size)
{
    QSignalBlocker blocker(d->ui.view_size);

    for ( int i = 0; i < d->standard_sizes.size(); i++ )
        if ( d->standard_sizes[i] == size )
            d->ui.view_size->setCurrentIndex(d->sizes_model.index(i, 0));

    Q_EMIT font_size_changed(size);
}


void glaxnimate::gui::font::FontSizeWidget::size_selected(const QModelIndex& index)
{
    QSignalBlocker blocker(d->ui.spin_size);

    qreal size = index.data().toInt();
    d->ui.spin_size->setValue(size);;

    Q_EMIT font_size_changed(size);
}
