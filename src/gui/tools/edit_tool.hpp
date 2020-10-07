#pragma once
#include "base.hpp"
#include "math/bezier_point.hpp"

namespace tools {

class EditTool : public Tool
{
public:
    EditTool();
    ~EditTool();

    QString id() const override { return "edit"; }
    QIcon icon() const override { return QIcon::fromTheme("edit-node"); }
    QString name() const override { return QObject::tr("Edit"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F2"), QKeySequence::PortableText); }

    void selection_set_vertex_type(math::BezierPointType t);
    void selection_delete();
    void selection_straighten();
    void selection_curve();

private:
    void mouse_press(const MouseEvent& event) override;
    void mouse_move(const MouseEvent& event) override;
    void mouse_release(const MouseEvent& event) override;
    void mouse_double_click(const MouseEvent& event) override;

    void paint(const PaintEvent& event) override;
    QCursor cursor() override;

    void key_press(const KeyEvent& event) override;
    void key_release(const KeyEvent& event) override;

    void on_selected(graphics::DocumentScene * scene, model::DocumentNode * node) override;

    void enable_event(const Event&) override;
    void disable_event(const Event&) override;

    QWidget* on_create_widget() override;

private:
    class Private;
    std::unique_ptr<Private> d;

    static Autoreg<EditTool> autoreg;
};

} // namespace tools
