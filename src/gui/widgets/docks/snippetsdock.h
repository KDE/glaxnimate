/*
 * SPDX-FileCopyrightText: 2024 Julius Künzel <julius.kuenzel@kde.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

# pragma once

#include "glaxnimate/model/assets/gradient.hpp"
#include "glaxnimate/model/shapes/fill.hpp"
#include "glaxnimate/model/shapes/stroke.hpp"

#include "widgets/dialogs/glaxnimate_window.hpp"
#include <QDockWidget>
#include <QObject>
#include <QtColorWidgets/color_palette_model.hpp>

namespace glaxnimate::gui {

class SnippetsDock : public QDockWidget
{
    Q_OBJECT

public:
    SnippetsDock(GlaxnimateWindow* parent);

    ~SnippetsDock();

private:
    class Private;
    std::unique_ptr<Private> d;
};

}
