/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <KConfigDialog>

namespace glaxnimate::gui {

class SettingsDialog : public KConfigDialog
{
    Q_OBJECT

public:
    SettingsDialog(QWidget *parent);

private:
    // class AutoConfigPage;
};

} // namespace glaxnimate::gui
