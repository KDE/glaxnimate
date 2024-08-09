/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef FONTSIZEWIDGET_H
#define FONTSIZEWIDGET_H

#include <memory>
#include <QWidget>

namespace glaxnimate::gui::font {


class FontSizeWidget : public QWidget
{
    Q_OBJECT

public:
    FontSizeWidget(QWidget* parent = nullptr);
    ~FontSizeWidget();

    void set_font_size(qreal size);
    qreal font_size() const;

Q_SIGNALS:
    void font_size_changed(qreal size);

private Q_SLOTS:
    void size_edited(double size);
    void size_selected(const QModelIndex& index);

private:
    class Private;
    std::unique_ptr<Private> d;
};


} // namespace glaxnimate::gui::font

#endif // FONTSIZEWIDGET_H
