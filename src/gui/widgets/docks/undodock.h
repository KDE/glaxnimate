/*
 * SPDX-FileCopyrightText: 2024 Julius Künzel <julius.kuenzel@kde.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

# pragma once

#include "widgets/dialogs/glaxnimate_window.hpp"
#include "widgets/timeline/frame_controls_widget.hpp"
#include <QDockWidget>
#include <QObject>

namespace glaxnimate::gui {

class UndoDock : public QDockWidget
{
    Q_OBJECT

public:
    UndoDock(GlaxnimateWindow* parent);

    ~UndoDock();

    void setUndoGroup(QUndoGroup *group);

private:
    class Private;
    std::unique_ptr<Private> d;
};

}
