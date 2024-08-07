/*
 * SPDX-FileCopyrightText: 2024 Julius Künzel <julius.kuenzel@kde.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "logsdock.h"

#include "app/log/log_model.hpp"
#include "style/better_elide_delegate.hpp"
#include "ui_logs.h"

using namespace glaxnimate::gui;

class LogsDock::Private
{
public:
    ::Ui::dock_logs ui;
};

LogsDock::LogsDock(GlaxnimateWindow *parent, app::log::LogModel *logModel)
    : QDockWidget(parent)
    , d(std::make_unique<Private>())
{
    d->ui.setupUi(this);

    d->ui.view_logs->setModel(logModel);
    d->ui.view_logs->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    d->ui.view_logs->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    d->ui.view_logs->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);

    auto del = new style::BetterElideDelegate(Qt::ElideLeft, d->ui.view_logs);
    d->ui.view_logs->setItemDelegateForColumn(2, del);
}

LogsDock::~LogsDock() = default;

