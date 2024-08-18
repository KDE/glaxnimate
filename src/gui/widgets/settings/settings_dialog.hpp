/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>
#include <KConfigDialog>
#include <KXmlGuiWindow>

namespace glaxnimate::gui {

class SettingsDialog : public KConfigDialog
{
    Q_OBJECT

public:
    SettingsDialog(KXmlGuiWindow *parent);
    ~SettingsDialog();

protected:
    void updateSettings() override;
    void updateWidgets() override;
    bool hasChanged() override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui
