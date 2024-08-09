/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "rectangle_tool.hpp"
#include "model/shapes/ellipse.hpp"

namespace glaxnimate::gui::tools {

class EllipseTool : public RectangleTool
{
public:
    QString id() const override { return "draw-ellipse"; }
    QIcon icon() const override { return QIcon::fromTheme("draw-ellipse"); }
    QString name() const override { return i18n("Ellipse"); }
    QString action_name() const override { return QStringLiteral("tool_draw_ellipse"); }
    QKeySequence key_sequence() const override { return Qt::Key_F7; }
    static int static_group() noexcept { return Registry::Shape; }
    int group() const noexcept override { return static_group(); }

    void on_drag_complete(const MouseEvent& event) override
    {
        auto shape = std::make_unique<model::Ellipse>(event.window->document());
        rect = rect.normalized();
        shape->position.set(rect.center());
        shape->size.set(rect.size());
        create_shape(i18n("Draw Ellipse"), event, std::move(shape));
    }

    void paint(const PaintEvent& event) override
    {
        if ( dragging )
        {
            QPainterPath path;
            path.addEllipse(rect);
            path = event.view->mapFromScene(path);
            draw_shape(event, path);
        }
    }

private:
    static Autoreg<EllipseTool> autoreg;
};

tools::Autoreg<tools::EllipseTool> tools::EllipseTool::autoreg{max_priority + 1};

} // namespace glaxnimate::gui::tools

