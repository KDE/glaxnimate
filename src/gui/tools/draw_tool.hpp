#pragma once

#include "draw_tool_base.hpp"
#include "math/bezier/bezier.hpp"
#include "model/shapes/path.hpp"


namespace tools {


class DrawTool : public DrawToolBase
{
public:
    DrawTool();
    ~DrawTool();

    QString id() const override { return "draw-bezier"; }
    QIcon icon() const override { return QIcon::fromTheme("draw-bezier-curves"); }
    QString name() const override { return QObject::tr("Draw Bezier"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F3"), QKeySequence::PortableText); }

    void mouse_press(const MouseEvent& event) override;
    void mouse_move(const MouseEvent& event) override;
    void mouse_release(const MouseEvent& event) override;
    void mouse_double_click(const MouseEvent& event) override;
    void paint(const PaintEvent& event) override;
    void key_press(const KeyEvent& event) override;
    void key_release(const KeyEvent& event) override;
    void enable_event(const Event& event) override;
    void disable_event(const Event& event) override;
    void on_selected(graphics::DocumentScene * scene, model::DocumentNode * node) override;
    void on_deselected(graphics::DocumentScene * scene, model::DocumentNode * node) override;

private:
    class Private;
    std::unique_ptr<Private> d;

    static Autoreg<DrawTool> autoreg;
};

} // namespace tools
