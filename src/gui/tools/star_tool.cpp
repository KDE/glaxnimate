/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "draw_tool_drag.hpp"
#include "model/shapes/polystar.hpp"
#include "math/math.hpp"
#include "widgets/tools/star_tool_widget.hpp"

namespace glaxnimate::gui::tools {

class StarTool : public DrawToolDrag
{
public:
    QString id() const override { return "draw-star"; }
    QIcon icon() const override { return QIcon::fromTheme("draw-polygon-star"); }
    QString name() const override { return i18n("Star"); }
    QString action_name() const override { return QStringLiteral("tool_draw_star"); }
    QKeySequence key_sequence() const override { return Qt::Key_Asterisk; }
    static int static_group() noexcept { return Registry::Shape; }
    int group() const noexcept override { return static_group(); }

protected:
    void on_drag_start() override
    {
        polar = {};
    }

    void on_drag(const MouseEvent& event) override
    {
        angle_from_points();
        if ( event.modifiers() & Qt::ControlModifier )
            snap_angle();
        update_shape();
    }

    void on_drag_complete(const MouseEvent& event) override
    {
        auto shape = std::make_unique<model::PolyStar>(event.window->document());
        shape->position.set(p1);
        shape->outer_radius.set(polar.length);
        shape->angle.set(math::rad2deg(polar.angle));
        shape->inner_radius.set(widget()->spoke_ratio() * polar.length);
        shape->type.set(widget()->star_type());
        shape->points.set(widget()->points());
        create_shape(i18n("Draw Star"), event, std::move(shape));
        bez.clear();
    }

    void paint(const PaintEvent& event) override
    {
        if ( dragging )
        {
            QPainterPath path;
            bez.add_to_painter_path(path);
            draw_shape(event, event.view->mapFromScene(path));
        }
    }

    QWidget* on_create_widget() override
    {
        return new StarToolWidget();
    }

    void key_press(const KeyEvent& event) override
    {
        if ( event.key() == Qt::Key_Escape || event.key() == Qt::Key_Back )
        {
            dragging = false;
            event.repaint();
            bez.clear();
            event.accept();
        }
        else if ( dragging && event.key() == Qt::Key_Control )
        {
            snap_angle();
            update_shape();
            event.repaint();
        }
    }

private:
    void snap_angle()
    {
        polar.angle = qRound(polar.angle / math::pi * 180 / 15) * 15. / 180. * math::pi;
    }

    void angle_from_points()
    {
        polar = p2 - p1;
        polar.angle = polar.angle + math::pi / 2;
    }

    void update_shape()
    {
        bez = model::PolyStar::draw(
            widget()->star_type(),
            p1,
            widget()->spoke_ratio() * polar.length,
            polar.length,
            polar.angle,
            widget()->points(),
            0,
            0,
            false
        );
    }

    math::PolarVector<QPointF> polar;
    math::bezier::Bezier bez;


    StarToolWidget* widget()
    {
        return static_cast<StarToolWidget*>(get_settings_widget());
    }

    static Autoreg<StarTool> autoreg;
};


tools::Autoreg<tools::StarTool> tools::StarTool::autoreg{max_priority + 2};

} // namespace glaxnimate::gui::tools
