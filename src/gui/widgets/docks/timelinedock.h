/*
 * SPDX-FileCopyrightText: 2024 Julius Künzel <julius.kuenzel@kde.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

# pragma once

#include "widgets/dialogs/glaxnimate_window.hpp"
#include "widgets/timeline/compound_timeline_widget.hpp"
#include "widgets/timeline/frame_controls_widget.hpp"
#include <QDockWidget>
#include <QObject>

namespace glaxnimate::gui {

class TimelineDock : public QDockWidget
{
    Q_OBJECT

public:
    TimelineDock(GlaxnimateWindow* parent);

    ~TimelineDock();

    void clear_document();

    glaxnimate::gui::CompoundTimelineWidget* timelineWidget();
    glaxnimate::gui::FrameControlsWidget* playControls();


private:
    class Private;
    std::unique_ptr<Private> d;
};

}
