/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef EXTERNALFONTWIDGET_H
#define EXTERNALFONTWIDGET_H

#include <memory>
#include <QWidget>

#include "model/custom_font.hpp"

namespace glaxnimate::gui::font {

class ExternalFontWidget : public QWidget
{
    Q_OBJECT

public:
    ExternalFontWidget(QWidget* parent = nullptr);
    ~ExternalFontWidget();

    void set_font_size(double size);

    model::CustomFont custom_font() const;
    const QFont& selected_font() const;

protected:
    void showEvent(QShowEvent * event) override;

private Q_SLOTS:
    void url_from_file();
    void load_url();
    void url_changed(const QString& url);

Q_SIGNALS:
    void font_changed(const QFont& font);

private:
    class Private;
    std::unique_ptr<Private> d;
};


} // namespace glaxnimate::gui::font

#endif // EXTERNALFONTWIDGET_H
