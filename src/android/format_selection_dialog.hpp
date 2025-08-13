/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef GLAXNIMATE_ANDROID_FORMAT_SELECTION_DIALOG_HPP
#define GLAXNIMATE_ANDROID_FORMAT_SELECTION_DIALOG_HPP

#include "io/io_registry.hpp"
#include "base_dialog.hpp"

namespace glaxnimate::android {

class FormatSelectionDialog : public BaseDialog
{
public:
    FormatSelectionDialog(QWidget* parent = nullptr);
    ~FormatSelectionDialog();

    io::ImportExport* format() const;

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::android

#endif // GLAXNIMATE_ANDROID_FORMAT_SELECTION_DIALOG_HPP
