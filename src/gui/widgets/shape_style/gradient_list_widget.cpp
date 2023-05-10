/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "gradient_list_widget.hpp"
#include "ui_gradient_list_widget.h"

#include <QEvent>
#include <QMetaEnum>
#include <QRegularExpression>
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QVBoxLayout>

#include <QtColorWidgets/gradient_delegate.hpp>
#include <QtColorWidgets/gradient_list_model.hpp>

#include "model/document.hpp"
#include "model/assets/assets.hpp"
#include "model/shapes/fill.hpp"
#include "model/shapes/stroke.hpp"

#include "command/undo_macro_guard.hpp"
#include "command/object_list_commands.hpp"
#include "command/property_commands.hpp"

#include "item_models/gradient_list_model.hpp"

using namespace glaxnimate::gui;
using namespace glaxnimate;

class GradientListWidget::Private
{
public:
    Ui::GradientListWidget ui;
    item_models::GradientListModel model;
    model::Document* document = nullptr;
    glaxnimate::gui::SelectionManager* window = nullptr;
    std::vector<model::Styler*> fills;
    std::vector<model::Styler*> strokes;
    model::Fill* current_fill = nullptr;
    model::Stroke* current_stroke = nullptr;
    color_widgets::GradientDelegate delegate;
    color_widgets::GradientListModel presets;
    GradientListWidget* parent;

    model::GradientColors* current()
    {
        if ( !document )
            return nullptr;
        auto index = ui.list_view->currentIndex();
        if ( !index.isValid() )
            return nullptr;
        return document->assets()->gradient_colors->values[index.row()];
    }

    struct TypeButtonSlot
    {
        std::pair<QToolButton*, QToolButton*> btn_others;
        GradientListWidget* widget;
        model::Gradient::GradientType gradient_type;
        bool secondary;

        void operator() (bool checked)
        {
            if ( !checked )
            {
                widget->d->clear_gradient(secondary);
            }
            else
            {
                btn_others.first->setChecked(false);
                btn_others.second->setChecked(false);
                widget->d->set_gradient(secondary, gradient_type);
            }
        }
    };

    TypeButtonSlot slot_conical(GradientListWidget* widget, bool secondary)
    {
        return {
            {secondary ? ui.btn_stroke_linear : ui.btn_fill_linear, secondary ? ui.btn_stroke_radial : ui.btn_fill_radial},
            widget,
            model::Gradient::Conical,
            secondary
        };
    }

    TypeButtonSlot slot_radial(GradientListWidget* widget, bool secondary)
    {
        return {
            {secondary ? ui.btn_stroke_linear : ui.btn_fill_linear, secondary ? ui.btn_stroke_conical : ui.btn_fill_conical},
            widget,
            model::Gradient::Radial,
            secondary
        };
    }

    TypeButtonSlot slot_linear(GradientListWidget* widget, bool secondary)
    {
        return {
            {secondary ? ui.btn_stroke_radial : ui.btn_fill_radial, secondary ? ui.btn_stroke_conical : ui.btn_fill_conical},
            widget,
            model::Gradient::Linear,
            secondary
        };
    }

    void set_gradient(bool secondary, model::Gradient::GradientType gradient_type)
    {
        auto& targets = secondary ? strokes : fills;

        // no valid selection
        if ( targets.empty() )
            return;

        // gather colors
        model::GradientColors* colors = current();
        if ( !colors )
        {
            if ( document->assets()->gradient_colors->values.empty() )
                add_gradient();
            else
                ui.list_view->setCurrentIndex(model.gradient_to_index(document->assets()->gradient_colors->values.back()));

            colors = current();
        }

        // Undo macro
        command::UndoMacroGuard macro(tr("Set %1 Gradient").arg(model::Gradient::gradient_type_name(gradient_type)), document);

        // Gather bounding box
        auto shape_element = window->current_shape();
        QRectF bounds;

        if ( shape_element )
        {
            bounds = shape_element->local_bounding_rect(shape_element->time());
            if ( bounds.isNull() )
            {
                if ( auto parent = window->current_shape_container() )
                    bounds = parent->bounding_rect(shape_element->time());
            }
        }

        if ( bounds.isNull() )
            bounds = QRectF(QPointF(0, 0), window->current_composition()->size());


        // Insert Gradient
        auto grad = std::make_unique<model::Gradient>(document);
        grad->colors.set(colors);
        grad->type.set(gradient_type);

        if ( gradient_type == model::Gradient::Linear )
            grad->start_point.set(QPointF(bounds.left(), bounds.center().y()));
        else
            grad->start_point.set(bounds.center());

        grad->highlight.set(grad->start_point.get());
        grad->end_point.set(QPointF(bounds.right(), bounds.center().y()));

        model::Gradient* gradient = grad.get();
        document->push_command(new command::AddObject<model::Gradient>(
            &document->assets()->gradients->values,
            std::move(grad)
        ));

        // Apply changes
        for ( auto styler : targets )
        {
            if ( styler->docnode_locked_recursive() )
                continue;

            // update existing Gradient object
            model::Gradient* old = styler->use.get() ? styler->use->cast<model::Gradient>() : nullptr;
            if ( old )
            {
                document->push_command(new command::SetPropertyValue(
                    &old->type,
                    QVariant::fromValue(gradient_type)
                ));

                document->push_command(new command::SetPropertyValue(
                    &old->colors,
                    QVariant::fromValue(colors)
                ));

                emit parent->selected(old, secondary);
            }
            else
            {
                styler->use.set_undoable(QVariant::fromValue(gradient));
            }
        }

        gradient->remove_if_unused(false);

        emit parent->selected(gradient, secondary);

    }

    void clear_gradient(bool secondary)
    {
        auto& targets = secondary ? strokes : fills;
        if ( targets.empty() )
            return;

        command::UndoMacroGuard macro(tr("Remove Gradient"), document);

        for ( auto styler : targets )
        {
            if ( styler->docnode_locked_recursive() )
                continue;

            model::Gradient* old = styler->use.get() ? styler->use->cast<model::Gradient>() : nullptr;

            styler->use.set_undoable(QVariant::fromValue((model::BrushStyle*)nullptr));

            if ( old )
                old->remove_if_unused(false);
        }

        emit parent->selected(nullptr, secondary);
    }

    void add_gradient()
    {
        add_gradient({{0, window->current_color()}, {1, window->secondary_color()}}, tr("Gradient"));
    }

    void add_gradient(const QGradientStops& stops, const QString& name)
    {
        if ( !document )
            return;

        auto ptr = std::make_unique<model::GradientColors>(document);
        auto raw = ptr.get();
        ptr->colors.set(stops);
        ptr->name.set(name);
        document->push_command(new command::AddObject(
            &document->assets()->gradient_colors->values,
            std::move(ptr)
        ));

        clear_buttons();
        ui.list_view->setCurrentIndex(model.gradient_to_index(raw));
    }

    void clear_buttons()
    {
        ui.btn_fill_linear->setChecked(false);
        ui.btn_fill_radial->setChecked(false);
        ui.btn_fill_conical->setChecked(false);
        ui.btn_stroke_linear->setChecked(false);
        ui.btn_stroke_radial->setChecked(false);
        ui.btn_stroke_conical->setChecked(false);
    }

    void set_targets(const std::vector<model::Fill*>& fills, const std::vector<model::Stroke*>& strokes)
    {
        this->fills.assign(fills.begin(), fills.end());
        this->strokes.assign(strokes.begin(), strokes.end());

        ui.btn_fill_linear->setEnabled(fills.size());
        ui.btn_fill_radial->setEnabled(fills.size());
        ui.btn_fill_conical->setEnabled(fills.size());
        ui.btn_stroke_linear->setEnabled(strokes.size());
        ui.btn_stroke_radial->setEnabled(strokes.size());
        ui.btn_stroke_conical->setEnabled(strokes.size());
    }

    void buttons_from_targets(bool set_current)
    {
        auto gradient_fill = current_fill ? qobject_cast<model::Gradient*>(current_fill->use.get()) : nullptr;
        auto gradient_stroke = current_stroke ? qobject_cast<model::Gradient*>(current_stroke->use.get()) : nullptr;

        clear_buttons();

        model::GradientColors* colors_fill = gradient_fill ? gradient_fill->colors.get() : nullptr;
        model::GradientColors* colors_stroke = gradient_stroke ? gradient_stroke->colors.get() : nullptr;

        if ( !colors_fill && !colors_stroke )
            return;

        model::GradientColors* colors;

        if ( set_current )
        {
            colors = colors_fill ? colors_fill : colors_stroke;
            ui.list_view->setCurrentIndex(model.gradient_to_index(colors));
        }
        else
        {
            colors = current();
        }



        if ( colors_fill == colors )
        {
            switch ( gradient_fill->type.get() )
            {
                case model::Gradient::Radial:
                    ui.btn_fill_radial->setChecked(true);
                    break;
                case model::Gradient::Linear:
                    ui.btn_fill_linear->setChecked(true);
                    break;
                case model::Gradient::Conical:
                    ui.btn_fill_conical->setChecked(true);
                    break;
            }
        }

        if ( colors_stroke == colors )
        {
            switch ( gradient_stroke->type.get() )
            {
                case model::Gradient::Radial:
                    ui.btn_stroke_radial->setChecked(true);
                    break;
                case model::Gradient::Linear:
                    ui.btn_stroke_linear->setChecked(true);
                    break;
                case model::Gradient::Conical:
                    ui.btn_stroke_conical->setChecked(true);
                    break;
            }
        }
    }

    void current_gradient_changed()
    {
        model::GradientColors* colors = current();
        if ( !colors )
        {
            clear_buttons();
            return;
        }

        buttons_from_targets(false);
    }

    void delete_gradient()
    {
        model::GradientColors* colors = current();
        if ( !colors )
        {
            if ( document->assets()->gradient_colors->values.empty() )
                return;

            colors = document->assets()->gradient_colors->values.back();
        }

        document->push_command(new command::RemoveObject(
            colors,
            &document->assets()->gradient_colors->values
        ));
    }

    void build_presets()
    {
        QMetaEnum meta = QMetaEnum::fromType<QGradient::Preset>();
        static QRegularExpression nocamel {"(\\w)([A-Z])"};

        for ( int i = 0; i < meta.keyCount(); i++ )
        {
            auto value = QGradient::Preset(meta.value(i));
            if ( value == QGradient::NumPresets )
                continue;

            QString name = meta.key(i);
            name.replace(nocamel, "\\1 \\2");
            QGradient grad(value);
            presets.setGradient(name, grad.stops());
        }
        presets.setEditMode(color_widgets::GradientListModel::EditName);
        connect(ui.btn_preset, &QAbstractButton::clicked, ui.btn_preset, [this]{ from_preset(); });
    }

    void from_preset()
    {
        QDialog dialog(window->as_widget());
        dialog.setWindowTitle(tr("Gradient Presets"));
        dialog.setWindowIcon(QIcon::fromTheme("color-gradient"));
        QVBoxLayout lay;
        dialog.setLayout(&lay);
        QComboBox combo;
        combo.setModel(&presets);
        combo.setEditable(true);
        combo.setInsertPolicy(QComboBox::NoInsert);
        lay.addWidget(&combo);
        QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        lay.addWidget(&buttons);
        connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        if ( dialog.exec() != QDialog::Rejected )
        {
            int index = combo.currentIndex();
            if ( index != -1 )
                add_gradient(presets.gradientStops(index), presets.nameFromIndex(index));
        }
    }
};

GradientListWidget::GradientListWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->parent = this;
    d->ui.setupUi(this);
    d->ui.list_view->setModel(&d->model);
    d->ui.list_view->horizontalHeader()->setSectionResizeMode(item_models::GradientListModel::Users, QHeaderView::ResizeToContents);
    d->ui.list_view->setItemDelegateForColumn(item_models::GradientListModel::Gradient, &d->delegate);

    d->build_presets();

    connect(d->ui.btn_new, &QAbstractButton::clicked, this, [this]{ d->add_gradient(); });
    connect(d->ui.btn_remove, &QAbstractButton::clicked, this, [this]{ d->delete_gradient(); });
    connect(d->ui.btn_fill_linear,   &QAbstractButton::clicked, this, d->slot_linear(this, false));
    connect(d->ui.btn_fill_radial,   &QAbstractButton::clicked, this, d->slot_radial(this, false));
    connect(d->ui.btn_fill_conical,   &QAbstractButton::clicked, this, d->slot_conical(this, false));
    connect(d->ui.btn_stroke_linear, &QAbstractButton::clicked, this, d->slot_linear(this, true));
    connect(d->ui.btn_stroke_radial, &QAbstractButton::clicked, this, d->slot_radial(this, true));
    connect(d->ui.btn_stroke_conical, &QAbstractButton::clicked, this, d->slot_conical(this, true));
}

GradientListWidget::~GradientListWidget() = default;

void GradientListWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void GradientListWidget::set_document(model::Document* document)
{
    d->document = document;
    d->fills = {};
    d->strokes = {};
    d->current_fill = nullptr;
    d->current_stroke = nullptr;
    d->clear_buttons();

    if ( !document )
        d->model.set_defs(nullptr);
    else
        d->model.set_defs(document->assets());
}

void GradientListWidget::set_window(glaxnimate::gui::SelectionManager* window)
{
    d->window = window;
}


void GradientListWidget::set_targets(const std::vector<model::Fill*>& fills, const std::vector<model::Stroke*>& strokes)
{
    d->set_targets(fills, strokes);
}

void GradientListWidget::change_current_gradient()
{
    d->current_gradient_changed();
}

void glaxnimate::gui::GradientListWidget::set_current(model::Fill* fill, model::Stroke* stroke)
{
    d->current_fill = fill;
    d->current_stroke = stroke;
    d->buttons_from_targets(true);
}
