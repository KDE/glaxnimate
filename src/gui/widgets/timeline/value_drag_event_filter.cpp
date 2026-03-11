/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "value_drag_event_filter.hpp"
#include "item_models/property_model_base.hpp"
#include "glaxnimate/model/animation/animatable.hpp"
#include "glaxnimate/command/animation_commands.hpp"

#include <QMouseEvent>
#include <QApplication>

using namespace glaxnimate::gui;

class glaxnimate::gui::ValueDragEventFilter::Private
{
public:
    QPointF start_value;
    QPointF min_value;
    QPointF max_value;
    QPointF drag_start;
    QAbstractItemView* view = nullptr;
    item_models::PropertyModelBase* property_model = nullptr;
    model::BaseProperty* prop = nullptr;
    enum {
        None,
        AnimPoint,
        AnimScale,
        AnimFloat,
        StaticFloat
    } prop_type = None;
    qreal percent_radius = 100;
    qreal radius_scale = 1;
    bool clamp = false;
    qreal round = 1;

    static constexpr const int mult_min = -1;
    static constexpr const int mult_max = 3;


    std::pair<qreal, qreal> value_comp(qreal start, bool percent)
    {
        qreal minv;
        qreal maxv;
        radius_scale = 1;
        if ( percent )
        {
            minv = 0;
            maxv = 1;
            radius_scale = 2;
        }
        else if ( qFuzzyIsNull(float(start)) )
        {
            minv = -percent_radius;
            maxv = percent_radius;
        }
        else if ( start > 0 )
        {
            minv = mult_min * start;
            maxv = mult_max * start;
        }
        else
        {
            minv = mult_max * start;
            maxv = mult_min * start;
        }

        return {minv, maxv};
    }

    void set_vals(QPointF val, bool percent)
    {
        auto x = value_comp(val.x(), percent);
        auto y = value_comp(val.y(), percent);
        start_value = val;
        min_value = QPointF(x.first, y.first);
        max_value = QPointF(x.second, y.second);
        clamp = percent;
        round = percent ? 0.01 : 1;
    }

    bool grab_property(model::BaseProperty* prop)
    {
        if ( !prop )
            return false;

        auto flags = prop->traits().flags;
        if ( flags & model::PropertyTraits::List || flags & model::PropertyTraits::ReadOnly || flags & model::PropertyTraits::OptionList )
            return false;

        auto animated = flags & model::PropertyTraits::Animated;

        if ( animated )
        {
            if ( prop->traits().type == model::PropertyTraits::Float )
            {
                auto anim_scalar = static_cast<model::AnimatedProperty<float>*>(prop);
                this->prop = prop;
                set_vals(QPointF(anim_scalar->get(), anim_scalar->get()), prop->traits().flags & model::PropertyTraits::Percent);
                prop_type = AnimFloat;
                return true;
            }
            else if ( prop->traits().type == model::PropertyTraits::Point )
            {

                auto anim_point = static_cast<model::AnimatedProperty<QPointF>*>(prop);
                set_vals(anim_point->get(), false);
                prop_type = AnimPoint;
                this->prop = prop;
                return true;
            }
            else if ( prop->traits().type == model::PropertyTraits::Scale )
            {

                auto anim_point = static_cast<model::AnimatedProperty<QVector2D>*>(prop);
                set_vals(anim_point->get().toPointF(), true);
                prop_type = AnimScale;
                this->prop = prop;
                return true;
            }
        }
        else if ( prop->traits().type == model::PropertyTraits::Float )
        {
            auto static_scalar = static_cast<model::Property<float>*>(prop);
            prop_type = StaticFloat;
            this->prop = prop;
            set_vals(QPointF(static_scalar->get(), static_scalar->get()), prop->traits().flags & model::PropertyTraits::Percent);
            return true;
        }

        return false;
    }

    bool mouse_press(QMouseEvent* event)
    {
        if ( event->button() != Qt::LeftButton )
            return false;

        QModelIndex index = view->indexAt(event->pos());
        if ( !grab_property(property_model->item(index).property) )
            return false;

        drag_start = event->pos();

        QApplication::setOverrideCursor(prop->traits().type == model::PropertyTraits::Point ? Qt::SizeAllCursor : Qt::SizeHorCursor);
        view->viewport()->grabMouse();

        event->accept();
        return true;
    }

    qreal value(qreal min, qreal max, qreal start, qreal percent)
    {
        qreal val = percent < 0 ? math::lerp(start, min, -percent) : math::lerp(start, max, percent);
        val = qRound(val / round) * round;
        return clamp ? math::bound(min, val, max) : val;
    }

    void set_value(QMouseEvent* event, bool commit)
    {
        QPointF percent = (event->pos() - drag_start) / (percent_radius * radius_scale);

        int prefer_y_axis = math::abs(percent.y()) > math::abs(percent.x());

        qreal val_x = value(min_value.x(), max_value.x(), start_value.x(), percent.x());
        qreal val_y = value(min_value.y(), max_value.y(), start_value.y(), percent.y());

        if ( event->modifiers() & Qt::ControlModifier )
        {
            if ( prefer_y_axis )
                val_x = start_value.x();
            else
                val_y = start_value.y();
        }

        switch ( prop_type )
        {
            case None:
                break;
            case AnimPoint:
            {
                auto concprop = static_cast<model::AnimatedProperty<QPointF>*>(prop);
                property_model->document()->push_command(new command::SetKeyframe(
                    concprop,
                    concprop->time(),
                    QPointF(val_x, val_y),
                    commit
                ));
                break;
            }
            case AnimScale:
            {
                auto concprop = static_cast<model::AnimatedProperty<QVector2D>*>(prop);
                property_model->document()->push_command(new command::SetKeyframe(
                    concprop,
                    concprop->time(),
                    QVector2D(val_x, val_y),
                    commit
                ));
                break;
            }
            case AnimFloat:
            {
                auto concprop = static_cast<model::AnimatedProperty<QPointF>*>(prop);
                property_model->document()->push_command(new command::SetKeyframe(
                    concprop,
                    concprop->time(),
                    prefer_y_axis ? val_y : val_x,
                    commit
                ));
                break;
            }
            case StaticFloat:
            {
                auto concprop = static_cast<model::Property<float>*>(prop);
                concprop->set_undoable(prefer_y_axis ? val_y : val_x, commit);
                break;
            }

        }

    }

    bool mouse_move(QMouseEvent* event)
    {
        if ( !prop )
            return false;

        set_value(event, false);
        event->accept();
        return true;
    }

    bool mouse_release(QMouseEvent* event)
    {
        if ( !prop )
            return false;

        set_value(event, true);
        QApplication::restoreOverrideCursor();
        view->releaseMouse();
        event->accept();
        return true;
    }

};

glaxnimate::gui::ValueDragEventFilter::ValueDragEventFilter(QAbstractItemView* view)
    : d(std::make_unique<Private>())
{
    view->viewport()->installEventFilter(this);
    d->view = view;
    d->property_model = static_cast<item_models::PropertyModelBase*>(view->model());
}

glaxnimate::gui::ValueDragEventFilter::~ValueDragEventFilter() = default;


bool glaxnimate::gui::ValueDragEventFilter::eventFilter(QObject *watched, QEvent *event)
{
    if ( watched != d->view->viewport() )
        return false;

    switch ( event->type() )
    {
        case QEvent::MouseButtonPress:
            return d->mouse_press(static_cast<QMouseEvent *>(event));
        case QEvent::MouseMove:
            return d->mouse_move(static_cast<QMouseEvent *>(event));
        case QEvent::MouseButtonRelease:
            return d->mouse_release(static_cast<QMouseEvent *>(event));
        default:
            break;
    }

    return false;
}
