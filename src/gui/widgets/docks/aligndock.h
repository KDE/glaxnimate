/*
 * SPDX-FileCopyrightText: 2024 Julius Künzel <julius.kuenzel@kde.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

# pragma once

#include "widgets/dialogs/glaxnimate_window.hpp"
#include <QDockWidget>
#include <QObject>

namespace glaxnimate::gui {

class AlignDock : public QDockWidget
{
    Q_OBJECT

public:
    AlignDock(GlaxnimateWindow* parent);

    ~AlignDock();

private:
    class Private;
    std::unique_ptr<Private> d;
};

}
