/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "gradient_editor.hpp"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include <KLocalizedString>

#include "glaxnimate/math/geom.hpp"
#include "glaxnimate/command/animation_commands.hpp"
#include "item_data.hpp"
#include "glaxnimate/utils/sort_gradient.hpp"

using namespace glaxnimate::gui;

graphics::GradientEditor::GradientEditor(model::Styler* styler)
    : styler_(styler)
{
    start.setVisible(false);
    finish.setVisible(false);
    highlight.setVisible(false);
    angle.setVisible(false);

    // Used by edit_tool
    start.set_role(MoveHandle::GradientStop);
    finish.set_role(MoveHandle::GradientStop);
    highlight.set_role(MoveHandle::GradientHighlight);

    start.setData(graphics::GradientStopIndex, 0);
    finish.setData(graphics::GradientStopIndex, 0);

    on_use_changed(styler_->use.get());

    connect(styler_, &model::Styler::use_changed, this, &GradientEditor::on_use_changed);

    connect(&start,     &MoveHandle::dragged,       this, &GradientEditor::start_dragged);
    connect(&start,     &MoveHandle::drag_finished, this, &GradientEditor::start_committed);
    connect(&finish,    &MoveHandle::dragged,       this, &GradientEditor::finish_dragged);
    connect(&finish,    &MoveHandle::drag_finished, this, &GradientEditor::finish_committed);
    connect(&highlight, &MoveHandle::dragged,       this, &GradientEditor::highlight_dragged);
    connect(&highlight, &MoveHandle::drag_finished, this, &GradientEditor::highlight_committed);
    connect(&angle, &MoveHandle::dragged,       this, &GradientEditor::angle_dragged);
    connect(&angle, &MoveHandle::drag_finished, this, &GradientEditor::angle_committed);
    connect(&start, &MoveHandle::clicked, this, [this](Qt::KeyboardModifiers mod){
        if ( (mod & Qt::ShiftModifier) )
            show_highlight();
    });
    connect(&highlight, &MoveHandle::clicked, this, [this](Qt::KeyboardModifiers mod){
        if ( (mod & Qt::ShiftModifier) )
            remove_highlight();
    });
}

void graphics::GradientEditor::on_use_changed(model::BrushStyle* new_use)
{
    auto new_gradient = new_use->cast<model::Gradient>();

    if ( new_gradient && !new_gradient->colors.get() )
        new_gradient = nullptr;


    if ( new_gradient == gradient_ )
        return;

    if ( gradient_ )
        disconnect(gradient_, &model::Gradient::style_changed, this, &GradientEditor::update_stops_from_gradient);

    gradient_ = new_gradient;

    update_handles(true);

    if ( !gradient_ )
    {
        start.setVisible(false);
        finish.setVisible(false);
        highlight.setVisible(false);
        angle.setVisible(false);

        start.clear_associated_properties();
        finish.clear_associated_properties();
        highlight.clear_associated_properties();
        angle.clear_associated_properties();
        update();
        return;
    }


    start.set_associated_properties({&gradient_->start_point, &gradient_->colors->colors});
    finish.set_associated_properties({&gradient_->end_point, &gradient_->colors->colors});
    highlight.set_associated_properties({&gradient_->highlight, &gradient_->colors->colors});
    angle.set_associated_properties({&gradient_->angle});

    connect(gradient_, &model::Gradient::style_changed, this, &GradientEditor::update_stops_from_gradient);

    start.setVisible(true);
    finish.setVisible(true);

    if ( gradient_->type.get() == model::Gradient::Radial && gradient_->highlight.get() != gradient_->start_point.get() )
        highlight.setVisible(true);
    else
        highlight.setVisible(false);

    angle.setVisible(gradient_->type.get() == model::Gradient::Conical);

    start.setPos(gradient_->start_point.get());
    finish.setPos(gradient_->end_point.get());
    highlight.setPos(gradient_->highlight.get());
    angle.setPos(angle_preferred_pos());
    update();
}

QString graphics::GradientEditor::command_name() const
{
    return i18n("Drag Gradient");
}

void graphics::GradientEditor::start_dragged(QPointF p, Qt::KeyboardModifiers mods)
{
    if ( !gradient_ )
        return;

    if ( mods & Qt::ControlModifier )
    {
        p = math::line_closest_point(gradient_->start_point.get(), gradient_->end_point.get(), p);
        start.setPos(p);
    }

    auto cmd = new command::SetMultipleAnimated(command_name(), false);
    cmd->push_property(&gradient_->start_point, p);

    if ( !highlight.isVisible() )
        cmd->push_property(&gradient_->highlight, p);

    styler_->push_command(cmd);

    update_handles(false);
    update();
}


void graphics::GradientEditor::start_committed()
{
    if ( !gradient_ )
        return;

    auto cmd = new command::SetMultipleAnimated(command_name(), true);
    cmd->push_property(&gradient_->start_point, gradient_->start_point.value());

    if ( !highlight.isVisible() )
        cmd->push_property(&gradient_->highlight, gradient_->start_point.value());

    styler_->push_command(cmd);
}

void graphics::GradientEditor::finish_dragged(QPointF p, Qt::KeyboardModifiers mods)
{
    if ( !gradient_ )
        return;

    if ( mods & Qt::ControlModifier )
    {
        p = math::line_closest_point(gradient_->start_point.get(), gradient_->end_point.get(), p);
        finish.setPos(p);
    }


    styler_->push_command(
        new command::SetMultipleAnimated(&gradient_->end_point, p, false)
    );

    update_handles(false);
    update();
}

void graphics::GradientEditor::finish_committed()
{
    if ( !gradient_ )
        return;

    styler_->push_command(
        new command::SetMultipleAnimated(&gradient_->end_point, gradient_->end_point.value(), true)
    );
}


void graphics::GradientEditor::highlight_dragged(const QPointF& p)
{
    if ( !gradient_ )
        return;

    styler_->push_command(
        new command::SetMultipleAnimated(&gradient_->highlight, p, false)
    );
}

void graphics::GradientEditor::highlight_committed()
{
    if ( !gradient_ )
        return;

    styler_->push_command(
        new command::SetMultipleAnimated(&gradient_->highlight, gradient_->highlight.value(), true)
    );
}


void graphics::GradientEditor::angle_dragged(const QPointF& p)
{
    if ( !gradient_ )
        return;


    auto start = gradient_->start_point.get();
    auto end = p;
    auto angle = math::rad2deg(math::atan2(end.y() - start.y(), end.x() - start.x()));

    styler_->push_command(
        new command::SetMultipleAnimated(&gradient_->angle, angle, false)
    );
}

void graphics::GradientEditor::angle_committed()
{
    if ( !gradient_ )
        return;

    styler_->push_command(
        new command::SetMultipleAnimated(&gradient_->angle, gradient_->angle.value(), true)
    );
}


QRectF graphics::GradientEditor::boundingRect() const
{
    if ( !gradient_ )
        return {};

    switch ( gradient_->type.get() )
    {
        case model::Gradient::Conical:
        {
            QPointF center = start.pos();
            qreal size = math::length(finish.pos() - center) * 1.1; // TODO
            return QRectF(center.x() - size, center.y() - size, center.x() + size , center.y() + size);
        }
        case model::Gradient::Linear:
            return QRectF(start.pos(), finish.pos());
        case model::Gradient::Radial:
            return QRectF(highlight.pos(), finish.pos());
    }

    return {};

}

void graphics::GradientEditor::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*)
{
    if ( gradient_ )
    {
        QPen p(option->palette.highlight(), 1);
        p.setCosmetic(true);
        painter->setPen(p);
        switch ( gradient_->type.get() )
        {
            case model::Gradient::Conical:
            {
                qreal size = math::length(finish.pos() - start.pos()) / 2;
                painter->drawEllipse(start.pos(), size, size);
                break;
            }
            case model::Gradient::Linear:
                painter->drawLine(start.pos(), finish.pos());
                break;
            case model::Gradient::Radial:
                painter->drawLine(highlight.pos(), finish.pos());
                break;
        }
    }
}

void graphics::GradientEditor::remove_highlight()
{
    if ( gradient_ && gradient_->type.get() == model::Gradient::Radial )
    {
        highlight.setPos(start.pos());
        highlight.setVisible(false);
        gradient_->highlight.set_undoable(gradient_->start_point.get());
    }
}

void graphics::GradientEditor::show_highlight()
{
    if ( gradient_ && gradient_->type.get() == model::Gradient::Radial )
    {
        highlight.setVisible(true);
        highlight.setPos(gradient_->highlight.get());
    }
}

void graphics::GradientEditor::remove_angle()
{
    if ( gradient_ && gradient_->type.get() == model::Gradient::Conical )
    {
        angle.setVisible(false);
        gradient_->angle.set_undoable(0);
    }
}

QPointF glaxnimate::gui::graphics::GradientEditor::angle_preferred_pos()
{
    QPointF start = gradient_->start_point.get();
    QPointF end = gradient_->end_point.get();
    auto polar = math::PolarVector(end - start);
    polar.angle = math::deg2rad(gradient_->angle.get());
    polar.length *= 0.8;
    return start + polar.to_cartesian();
}

bool glaxnimate::gui::graphics::GradientEditor::angle_visible() const
{
    return angle.isVisible();
}


void graphics::GradientEditor::show_angle()
{
    if ( gradient_ && gradient_->type.get() == model::Gradient::Conical )
    {
        angle.setVisible(true);
        angle.setPos(angle_preferred_pos());
    }
}

QPointF glaxnimate::gui::graphics::GradientEditor::stop_pos(qreal f)
{

    switch ( gradient_->type.get() )
    {
        case model::Gradient::Conical:
        {
            QPointF center = gradient_->start_point.get();
            math::PolarVector polar(gradient_->end_point.get() - center);
            polar.length /= 2;
            polar.angle = math::deg2rad(gradient_->angle.get()) + math::tau * f;
            return center + polar.to_cartesian();
        }
        case model::Gradient::Linear:
            return math::lerp(start.pos(), finish.pos(), f);
        case model::Gradient::Radial:
            return math::lerp(highlight.pos(), finish.pos(), f);
    }
    return {};
}

void graphics::GradientEditor::update_handles(bool create_stops)
{
    if ( create_stops )
        stops.clear();

    if ( !gradient_ )
        return;

    QPointF start = gradient_->start_point.get();
    QPointF end = gradient_->end_point.get();

    this->start.setPos(start);
    finish.setPos(end);
    highlight.setPos(gradient_->highlight.get());
    angle.setPos(angle_preferred_pos());
    highlight.setVisible(
        gradient_->type.get() == model::Gradient::Radial &&
        (gradient_->highlight.get() != gradient_->start_point.get() || highlight.isVisible())
    );
    angle.setVisible(gradient_->type.get() == model::Gradient::Conical);


    int i = 0;
    const auto& colors = gradient_->colors->colors.get();

    if ( create_stops )
    {
        for ( const auto& stop : colors )
        {
            stops.emplace_back(this, MoveHandle::Any, MoveHandle::Diamond);
            stops.back().set_role(MoveHandle::GradientStop);
            stops.back().setData(graphics::GradientStopIndex, i++);
            stops.back().setPos(stop_pos(stop.first));
            stops.back().setZValue(-10);
            stops.back().set_associated_property(&gradient_->colors->colors);
            connect(&stops.back(), &MoveHandle::dragged, this, &GradientEditor::stop_dragged);
            connect(&stops.back(), &MoveHandle::drag_finished, this, &GradientEditor::stop_committed);
        }
        finish.setData(graphics::GradientStopIndex, colors.size() - 1);
    }
    else
    {
        for ( auto& handle : stops )
        {
            handle.setPos(stop_pos(colors[i++].first));
        }
    }

    update();
}

void graphics::GradientEditor::stop_dragged()
{
    stop_move(false);
}

void graphics::GradientEditor::stop_committed()
{
    stop_move(true);
}

void graphics::GradientEditor::stop_move(bool commit)
{
    auto handle = static_cast<MoveHandle*>(sender());
    int index = handle->data(graphics::GradientStopIndex).toInt();

    auto colors = gradient_->colors->colors.get();
    if ( index < 0 || index >= colors.size() )
        return;

    QPointF pos = handle->pos();
    qreal ratio = 0;

    if ( gradient_->type.get() == model::Gradient::Conical )
    {
        auto start = gradient_->start_point.get();
        auto angle = math::atan2(pos.y() - start.y(), pos.x() - start.x()) - math::deg2rad(gradient_->angle.get());
        // atan2 returns [-pi, pi]
        ratio = math::fmod(angle / math::tau, 1.);
        pos = stop_pos(ratio);
    }
    else
    {
        QPointF start = gradient_->type.get() == model::Gradient::Radial ? gradient_->highlight.get() : gradient_->start_point.get();
        QPointF end = gradient_->end_point.get();
        // pos = math::line_closest_point(start, end, pos);

        if ( start == end )
        {
            pos = start;
        }
        else
        {
            qreal dist1 = math::length(start - pos);
            qreal dist2 = math::length(end - pos);
            ratio = math::bound(0., dist1 / (dist1+dist2), 1.);
            pos = math::lerp(start, end, ratio);
        }
    }

    handle->setPos(pos);
    colors[index].first = ratio;

    if ( commit )
        utils::sort_gradient(colors);

    gradient_->push_command(new command::SetMultipleAnimated(
        &gradient_->colors->colors,
        QVariant::fromValue(colors),
        commit
    ));

    if ( commit )
        update_handles(false);
}

void graphics::GradientEditor::update_stops_from_gradient()
{
    if ( !gradient_->colors.get() )
    {
        on_use_changed(nullptr);
        update_handles(true);
        return;
    }

    const auto& colors = gradient_->colors->colors.get();

    if ( colors.size() != int(stops.size()) )
        update_handles(true);
    else
        update_handles(false);
}

glaxnimate::model::Styler * graphics::GradientEditor::styler() const
{
    return styler_;
}

glaxnimate::model::Gradient * graphics::GradientEditor::gradient() const
{
    return gradient_;
}

bool graphics::GradientEditor::highlight_visible() const
{
    return highlight.isVisible();
}

