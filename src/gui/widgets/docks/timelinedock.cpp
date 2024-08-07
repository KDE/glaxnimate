/*
 * SPDX-FileCopyrightText: 2024 Julius Künzel <julius.kuenzel@kde.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "timelinedock.h"

#include "ui_timeline.h"

using namespace glaxnimate::gui;

class TimelineDock::Private
{
public:
    ::Ui::dock_timeline ui;
};

TimelineDock::TimelineDock(GlaxnimateWindow *parent)
    : QDockWidget(parent)
    , d(std::make_unique<Private>())
{
    d->ui.setupUi(this);

    d->ui.timeline_widget->set_controller(parent);

    d->ui.play_controls->set_record_enabled(false);
}

TimelineDock::~TimelineDock() = default;

void TimelineDock::clear_document()
{
    d->ui.timeline_widget->clear_document();
}

glaxnimate::gui::CompoundTimelineWidget* TimelineDock::timelineWidget()
{
    return d->ui.timeline_widget;
}

glaxnimate::gui::FrameControlsWidget* TimelineDock::playControls()
{
    return d->ui.play_controls;
}
