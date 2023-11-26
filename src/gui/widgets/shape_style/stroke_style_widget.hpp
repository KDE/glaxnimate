/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef STROKESTYLEWIDGET_H
#define STROKESTYLEWIDGET_H

#include <memory>
#include <QWidget>

#include "model/shapes/stroke.hpp"


namespace color_widgets {
class ColorPaletteModel;
} // namespace color_widgets

namespace glaxnimate::gui {

class StrokeStyleWidget : public QWidget
{
    Q_OBJECT

public:
    StrokeStyleWidget(QWidget* parent = nullptr);

    ~StrokeStyleWidget();

    void save_settings() const;

    model::Stroke* current() const;
    void set_current(model::Stroke* stroke);
    void set_targets(std::vector<model::Stroke*> targets);

    void set_stroke_width(qreal w);

    void set_gradient_stop(model::Styler* styler, int index);

    QPen pen_style() const;
    QColor current_color() const;

    void set_palette_model(color_widgets::ColorPaletteModel* palette_model);

private:
    void before_set_target();
    void after_set_target();

protected:
    void changeEvent ( QEvent* e ) override;
    void paintEvent(QPaintEvent * event) override;

public Q_SLOTS:
    void set_color(const QColor& color);

Q_SIGNALS:
    void color_changed(const QColor& color);
    void pen_style_changed();

private Q_SLOTS:
    void check_cap();
    void check_join();
    void check_color(const QColor& color);
    void color_committed(const QColor& color);
    void property_changed(const model::BaseProperty* prop);
    void check_width(double w);
    void check_miter(double w);
    void commit_width();
    void clear_color();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui
#endif // STROKESTYLEWIDGET_H
