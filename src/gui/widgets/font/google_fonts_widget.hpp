/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef GLAXNIMATE_GUI_FONT_GOOGLEFONTSWIDGET_H
#define GLAXNIMATE_GUI_FONT_GOOGLEFONTSWIDGET_H

#include <memory>
#include <QWidget>

#include "google_fonts_model.hpp"

namespace glaxnimate::gui::font {

class GoogleFontsWidget : public QWidget
{
    Q_OBJECT

public:
    GoogleFontsWidget(QWidget* parent = nullptr);
    ~GoogleFontsWidget();

    const GoogleFontsModel& model() const;

    void set_font_size(double size);

    model::CustomFont custom_font() const;
    const QFont& selected_font() const;

Q_SIGNALS:
    void font_changed(const QFont& font);

protected:
    void showEvent(QShowEvent* e) override;

private Q_SLOTS:
    void change_style(const QModelIndex& index);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui::font

#endif // GLAXNIMATE_GUI_FONT_GOOGLEFONTSWIDGET_H
