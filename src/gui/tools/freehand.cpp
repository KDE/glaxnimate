/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "draw_tool_base.hpp"
#include "model/shapes/path.hpp"
#include "math/bezier/operations.hpp"

namespace glaxnimate::gui::tools {

class FreehandTool : public DrawToolBase
{
public:
    QString id() const override { return "draw-freehand"; }
    QIcon icon() const override { return QIcon::fromTheme("draw-freehand"); }
    QString name() const override { return i18n("Draw Freehand"); }
    QString action_name() const override { return QStringLiteral("tool_draw_freehand"); }
    QKeySequence key_sequence() const override { return Qt::Key_F5; }
    static int static_group() noexcept { return Registry::Draw; }
    int group() const noexcept override { return static_group(); }

    void mouse_press(const MouseEvent& event) override
    {
        if ( event.button() == Qt::LeftButton )
        {
            path.add_point(event.scene_pos);
        }
    }
    void mouse_move(const MouseEvent& event) override
    {
        if ( !path.empty() )
        {
            path.add_point(event.scene_pos);
            event.repaint();
        }

    }
    void mouse_release(const MouseEvent& event) override
    {
        if ( path.size() > 1 )
        {
            auto shape = std::make_unique<model::Path>(event.window->document());
            math::bezier::simplify(path, 128);
            shape->shape.set(path);
            path.clear();
            create_shape(i18n("Draw Freehand"), event, std::move(shape));
        }
        else
        {
            if ( path.size() == 1 )
                path.clear();
            edit_clicked(event);
        }
    }

    void mouse_double_click(const MouseEvent& event) override
    {
        edit_clicked(event);
    }

    void paint(const PaintEvent& event) override
    {
        if ( !path.empty() )
        {
            QPainterPath ppath;
            path.add_to_painter_path(ppath);
            draw_shape(event, event.view->mapFromScene(ppath));
        }
    }

    void key_press(const KeyEvent& event) override
    {
        if ( event.key() == Qt::Key_Escape )
        {
            path.clear();
            event.repaint();
            event.accept();
        }
    }

    void key_release(const KeyEvent& event) override { Q_UNUSED(event); }
    void enable_event(const Event& event) override { Q_UNUSED(event); }
    void disable_event(const Event& event) override { Q_UNUSED(event); }

private:
    static Autoreg<FreehandTool> autoreg;
    math::bezier::Bezier path;
};


tools::Autoreg<tools::FreehandTool> tools::FreehandTool::autoreg{max_priority + 1};

} // namespace glaxnimate::gui::tools
