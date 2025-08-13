/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "base.hpp"

#include <QtColorWidgets/color_utils.hpp>
#include "widgets/tools/color_picker_widget.hpp"

namespace glaxnimate::gui::tools {

class ColorPickerTool : public Tool
{
public:
    QString id() const override { return "color-picker"; }
    QIcon icon() const override { return QIcon::fromTheme("color-picker"); }
    QString name() const override { return i18n("Color Picker"); }
    QString action_name() const override { return QStringLiteral("tool_color_picker"); }
    QKeySequence key_sequence() const override { return Qt::Key_F8; }
    static int static_group() noexcept { return Registry::Style;  }
    int group() const noexcept override { return static_group(); }

    void mouse_move(const MouseEvent& event) override
    {
#if QT_VERSION_MAJOR < 6
        widget()->set_color(color_widgets::utils::get_screen_color(event.event->globalPos()));
#else
        widget()->set_color(color_widgets::utils::get_screen_color(event.event->globalPosition().toPoint()));
#endif
    }

    void mouse_release(const MouseEvent& event) override
    {
#if QT_VERSION_MAJOR < 6
        QColor color = color_widgets::utils::get_screen_color(event.event->globalPos());
#else
        QColor color = color_widgets::utils::get_screen_color(event.event->globalPosition().toPoint());
#endif
        widget()->set_color(color);
        if ( widget()->set_fill() )
            event.window->set_current_color(color);
        else
            event.window->set_secondary_color(color);
    }
    void key_press(const KeyEvent& event) override
    {
        if ( event.key() == Qt::Key_Shift )
            widget()->swap_fill_color();
    }
    void key_release(const KeyEvent& event) override
    {
        if ( event.key() == Qt::Key_Shift )
            widget()->swap_fill_color();
    }

    QCursor cursor() override { return Qt::CrossCursor; }

    void mouse_press(const MouseEvent& event) override { Q_UNUSED(event); }
    void mouse_double_click(const MouseEvent& event) override { Q_UNUSED(event); }
    void paint(const PaintEvent& event) override { Q_UNUSED(event); }
    void enable_event(const Event& event) override { Q_UNUSED(event); }
    void disable_event(const Event& event) override { Q_UNUSED(event); }

protected:
    QWidget* on_create_widget() override
    {
        return new ColorPickerWidget();
    }

    ColorPickerWidget* widget()
    {
        return static_cast<ColorPickerWidget*>(get_settings_widget());
    }

private:
    static Autoreg<ColorPickerTool> autoreg;
};


Autoreg<ColorPickerTool> ColorPickerTool::autoreg{max_priority};

} // namespace glaxnimate::gui::tools



