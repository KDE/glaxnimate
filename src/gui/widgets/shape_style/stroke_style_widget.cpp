/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "stroke_style_widget.hpp"
#include "ui_stroke_style_widget.h"

#include <QPainter>
#include <QButtonGroup>
#include <QtMath>
#include <QStyleOptionFrame>
#include <QPointer>

#include "glaxnimate_app.hpp"
#include "glaxnimate_settings.hpp"
#include "model/document.hpp"
#include "command/animation_commands.hpp"
#include "utils/pseudo_mutex.hpp"

using namespace glaxnimate::gui;
using namespace glaxnimate;

class StrokeStyleWidget::Private
{
public:
    Qt::PenCapStyle cap;
    Qt::PenJoinStyle join;
    Ui::StrokeStyleWidget ui;
    QButtonGroup group_cap;
    QButtonGroup group_join;
    bool dark_theme = false;
    QPalette::ColorRole background = QPalette::Base;
    utils::PseudoMutex updating;
    int stop = -1;
    model::Stroke* current_target = nullptr;
    std::vector<model::Styler*> targets = {};


    bool can_update_target()
    {
        for ( auto target : targets )
        {
            if ( !target->docnode_locked_recursive() )
                return true;
        }

        return false;
    }

    void update_background(const QColor& color)
    {
        background = QPalette::Base;
        qreal val = color.valueF();
        qreal sat = color.saturationF();

        bool swap = val > 0.65 && sat < 0.5;
        if ( dark_theme )
            swap = !swap;

        if ( swap )
            background = QPalette::Text;
    }

    void update_from_target()
    {
        if ( current_target && !updating )
        {
            auto lock = updating.get_lock();
            ui.spin_stroke_width->setValue(current_target->width.get());
            set_cap_style(current_target->cap.get());
            set_join_style(current_target->join.get());
            ui.spin_miter->setValue(current_target->miter_limit.get());

            ui.color_selector->from_styler(current_target, stop);
        }
    }

    void set_cap_style(model::Stroke::Cap cap)
    {
        this->cap = Qt::PenCapStyle(cap);

        switch ( cap )
        {
            case model::Stroke::ButtCap:
                ui.button_cap_butt->setChecked(true);
                break;
            case model::Stroke::RoundCap:
                ui.button_cap_round->setChecked(true);
                break;
            case model::Stroke::SquareCap:
                ui.button_cap_square->setChecked(true);
                break;
        }
    }

    void set_join_style(model::Stroke::Join join)
    {
        this->join = Qt::PenJoinStyle(join);

        switch ( join )
        {
            case model::Stroke::BevelJoin:
                ui.button_join_bevel->setChecked(true);
                break;
            case model::Stroke::RoundJoin:
                ui.button_join_round->setChecked(true);
                break;
            case model::Stroke::MiterJoin:
                ui.button_join_miter->setChecked(true);
                break;
        }
    }

    template<class Prop>
    void set(const QString& name, Prop (model::Stroke::*prop), const QVariant& value, bool commit)
    {
        if ( updating )
            return;

        auto cmd = std::make_unique<command::SetMultipleAnimated>(name, commit);

        for ( auto target : targets )
        {
            if ( !target->docnode_locked_recursive() )
                cmd->push_property_not_animated(&(static_cast<model::Stroke*>(target)->*prop), value);
        }

        if ( !cmd->empty() && current_target )
            current_target->push_command(cmd.release());
    }

    void set_color(const QColor&, bool commit)
    {
        if ( updating )
            return;

        ui.color_selector->apply_to_targets(i18n("Update Stroke Color"), targets, stop, commit);
    }
};

StrokeStyleWidget::StrokeStyleWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);

#ifdef Q_OS_ANDROID
    d->ui.button_cap_butt->setIcon(QIcon::fromTheme("stroke-cap-butt"));
    d->ui.button_cap_round->setIcon(QIcon::fromTheme("stroke-cap-round"));
    d->ui.button_cap_square->setIcon(QIcon::fromTheme("stroke-cap-square"));
    d->ui.button_join_bevel->setIcon(QIcon::fromTheme("stroke-join-bevel"));
    d->ui.button_join_miter->setIcon(QIcon::fromTheme("stroke-join-miter"));
    d->ui.button_join_round->setIcon(QIcon::fromTheme("stroke-join-round"));
    d->ui.main_layout->setContentsMargins(0, 0, 0, 0);
#endif

    d->group_cap.addButton(d->ui.button_cap_butt);
    d->group_cap.addButton(d->ui.button_cap_round);
    d->group_cap.addButton(d->ui.button_cap_square);

    d->group_join.addButton(d->ui.button_join_bevel);
    d->group_join.addButton(d->ui.button_join_round);
    d->group_join.addButton(d->ui.button_join_miter);

    d->ui.spin_stroke_width->setValue(GlaxnimateSettings::stroke_width());
    d->ui.spin_miter->setValue(GlaxnimateSettings::stroke_miter());
    d->set_cap_style(model::Stroke::Cap(GlaxnimateSettings::stroke_cap()));
    d->set_join_style(model::Stroke::Join(GlaxnimateSettings::stroke_join()));

//    d->ui.tab_widget->setTabEnabled(2, false);
    d->ui.tab_widget->removeTab(2);

    d->ui.color_selector->set_current_color(GlaxnimateSettings::color_secondary());
    d->ui.color_selector->hide_secondary();

    d->dark_theme = palette().window().color().valueF() < 0.5;
}

StrokeStyleWidget::~StrokeStyleWidget() = default;

void StrokeStyleWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    qreal stroke_width = d->ui.spin_stroke_width->value();
    const qreal frame_margin = 6;
    const qreal margin = frame_margin+stroke_width*M_SQRT2/2;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QStyleOptionFrame panel;
    panel.initFrom(this);
    if ( isEnabled() )
        panel.state = QStyle::State_Enabled;
    panel.rect = d->ui.frame->geometry();
    panel.lineWidth = 2;
    panel.midLineWidth = 0;
    panel.state |= QStyle::State_Raised;
    panel.frameShape = QFrame::StyledPanel;
//     style()->drawPrimitive(QStyle::PE_Frame, &panel, &p, this);
    style()->drawControl(QStyle::CE_ShapedFrame, &panel, &p, this);

    p.setPen(pen_style());
    p.setBrush(Qt::NoBrush);

    QRectF draw_area = QRectF(d->ui.frame->geometry()).adjusted(margin, margin, -margin, -margin);
    QPolygonF poly;
    poly.push_back(draw_area.bottomLeft());
    poly.push_back(QPointF(draw_area.center().x(), draw_area.top()));
    poly.push_back(draw_area.bottomRight());

    p.drawPolyline(poly);
}

void StrokeStyleWidget::check_cap()
{
    if ( d->ui.button_cap_butt->isChecked() )
        d->cap = Qt::FlatCap;
    else if ( d->ui.button_cap_round->isChecked() )
        d->cap = Qt::RoundCap;
    else if ( d->ui.button_cap_square->isChecked() )
        d->cap = Qt::SquareCap;

    d->set(i18n("Set Line Cap"), &model::Stroke::cap, int(d->cap), true);

    Q_EMIT pen_style_changed();
    update();
}

void StrokeStyleWidget::check_join()
{
    if ( d->ui.button_join_bevel->isChecked() )
        d->join = Qt::BevelJoin;
    else if ( d->ui.button_join_round->isChecked() )
        d->join = Qt::RoundJoin;
    else if ( d->ui.button_join_miter->isChecked() )
        d->join = Qt::MiterJoin;

    d->set(i18n("Set Line Join"), &model::Stroke::join, int(d->join), true);

    Q_EMIT pen_style_changed();
    update();
}

void StrokeStyleWidget::save_settings() const
{
    GlaxnimateSettings::setStroke_width(d->ui.spin_stroke_width->value());
    GlaxnimateSettings::setStroke_miter(d->ui.spin_miter->value());
    GlaxnimateSettings::setStroke_cap(int(d->cap));
    GlaxnimateSettings::setStroke_join(int(d->join));
}

QPen StrokeStyleWidget::pen_style() const
{
    QPen pen(d->ui.color_selector->current_color(), d->ui.spin_stroke_width->value());
    pen.setCapStyle(d->cap);
    pen.setJoinStyle(d->join);
    pen.setMiterLimit(d->ui.spin_miter->value());
    return pen;
}

QColor StrokeStyleWidget::current_color() const
{
    return d->ui.color_selector->current_color();
}


void StrokeStyleWidget::set_color(const QColor& color)
{
    d->ui.color_selector->set_current_color(color);
    if ( d->can_update_target() )
        d->set_color(color, true);
}

void StrokeStyleWidget::check_color(const QColor& color)
{
    d->update_background(color);
    if ( d->can_update_target() )
        d->set_color(color, false);

    update();
    Q_EMIT pen_style_changed();
    Q_EMIT color_changed(color);
}

void glaxnimate::gui::StrokeStyleWidget::before_set_target()
{
    if ( d->current_target )
    {
        disconnect(d->current_target, &model::Object::property_changed, this, &StrokeStyleWidget::property_changed);
    }
}


void glaxnimate::gui::StrokeStyleWidget::after_set_target()
{
    if ( d->current_target )
    {
        d->update_from_target();
//         Q_EMIT color_changed(d->ui.color_selector->current_color());
        connect(d->current_target, &model::Object::property_changed, this, &StrokeStyleWidget::property_changed);
        update();
    }
}

void glaxnimate::gui::StrokeStyleWidget::set_current(model::Stroke* stroke)
{
    before_set_target();
    d->current_target = stroke;
    d->stop = -1;
    after_set_target();
}

void glaxnimate::gui::StrokeStyleWidget::set_targets(std::vector<model::Stroke *> targets)
{
    d->targets.clear();
    d->targets.assign(targets.begin(), targets.end());
}


void StrokeStyleWidget::property_changed(const model::BaseProperty* prop)
{
    d->update_from_target();
    if ( prop == &d->current_target->color || prop == &d->current_target->use )
        Q_EMIT color_changed(d->ui.color_selector->current_color());
    update();
}

void StrokeStyleWidget::check_miter(double w)
{
    d->set(i18n("Set Miter Limit"), &model::Stroke::miter_limit, w, false);

    Q_EMIT pen_style_changed();
    update();
}

void StrokeStyleWidget::check_width(double w)
{
    d->set(i18n("Set Line Width"), &model::Stroke::width, w, false);

    Q_EMIT pen_style_changed();
    update();
}

void StrokeStyleWidget::color_committed(const QColor& color)
{
    if ( d->can_update_target() )
        d->set_color(color, true);
    Q_EMIT pen_style_changed();
}

void StrokeStyleWidget::commit_width()
{
    d->set(i18n("Set Line Width"), &model::Stroke::width, d->ui.spin_stroke_width->value(), true);
    Q_EMIT pen_style_changed();
}

model::Stroke * StrokeStyleWidget::current() const
{
    return d->current_target;
}

void StrokeStyleWidget::set_palette_model(color_widgets::ColorPaletteModel* palette_model)
{
    d->ui.color_selector->set_palette_model(palette_model);
}

void StrokeStyleWidget::set_gradient_stop(model::Styler* styler, int index)
{
    if ( auto stroke = styler->cast<model::Stroke>() )
    {
        before_set_target();
        d->current_target = stroke;
        d->stop = index;
        after_set_target();
    }
}

void StrokeStyleWidget::set_stroke_width(qreal w)
{
    d->ui.spin_stroke_width->setValue(w);
}

void StrokeStyleWidget::clear_color()
{
    d->ui.color_selector->clear_targets(i18n("Clear Line Color"), d->targets);
    Q_EMIT pen_style_changed();
}
