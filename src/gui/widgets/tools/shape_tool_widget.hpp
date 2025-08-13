/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SHAPETOOLWIDGET_H
#define SHAPETOOLWIDGET_H

#include <memory>

#include <QWidget>

namespace glaxnimate::gui {

class ShapeToolWidget : public QWidget
{
    Q_OBJECT

public:
    ShapeToolWidget(QWidget* parent=nullptr);
    ~ShapeToolWidget();

    bool create_group() const;
    bool create_fill() const;
    bool create_stroke() const;
    bool create_layer() const;

private Q_SLOTS:
    void check_checks();

protected Q_SLOTS:
    void save_settings();

Q_SIGNALS:
    void checks_changed();

protected:
    class Private;
    ShapeToolWidget(std::unique_ptr<Private> d, QWidget* parent);

    void showEvent(QShowEvent *event) override;

    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui

#endif // SHAPETOOLWIDGET_H
