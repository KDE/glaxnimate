#include "widget_palette_editor.hpp"
#include "ui_widget_palette_editor.h"

#include <QApplication>
#include <QEvent>
#include <QInputDialog>

#include <QtColorWidgets/ColorDelegate>

#include "app/settings/palette_settings.hpp"

class WidgetPaletteEditor::Private
{
public:
    enum Roles
    {
        ColorRole = Qt::UserRole,
        ColorGroup
    };

    void refresh_custom()
    {
        ui.palette_view->blockSignals(true);
        ui.palette_view->clearContents();

        int i = 0;
        for ( const auto& p : app::settings::PaletteSettings::roles() )
        {
            ui.palette_view->setItem(i, 0, color_item(edited, p.second, QPalette::Active));
            ui.palette_view->setItem(i, 1, color_item(edited, p.second, QPalette::Disabled));

            ++i;
        }
        ui.palette_view->blockSignals(false);
    }

    QTableWidgetItem* color_item(const QPalette& palette, QPalette::ColorRole role, QPalette::ColorGroup group)
    {
        auto item = new QTableWidgetItem();
        QColor color = palette.color(group, role);
        item->setData(Qt::DisplayRole, color);
        item->setData(Qt::EditRole, color);
        item->setData(ColorRole, role);
        item->setData(ColorGroup, group);
        return item;
    }

    void setup_view()
    {
        ui.palette_view->blockSignals(true);
        ui.palette_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        ui.palette_view->setItemDelegateForColumn(0, &delegate);
        ui.palette_view->setItemDelegateForColumn(1, &delegate);
        int i = 0;
        for ( const auto& p : app::settings::PaletteSettings::roles() )
        {
            ui.palette_view->setRowCount(i+1);
            ui.palette_view->setVerticalHeaderItem(i, new QTableWidgetItem(p.first));

            ui.palette_view->setItem(i, 0, color_item(settings->default_palette, p.second, QPalette::Active));
            ui.palette_view->setItem(i, 1, color_item(settings->default_palette, p.second, QPalette::Disabled));

            ++i;
        }
        ui.palette_view->blockSignals(false);
    }

    void add_palette(QString name_hint)
    {
        if ( name_hint.isEmpty() )
            name_hint = tr("Custom");

        QString name = name_hint;
        for ( int i = 1; settings->palettes.contains(name); i++ )
            name = tr("%1 %2").arg(name_hint).arg(i);

        settings->palettes[name] = edited;
        ui.combo_saved->addItem(name);
        ui.combo_saved->setCurrentText(name);
    }

    bool use_default()
    {
        return ui.combo_saved->currentIndex() == 0;
    }

    app::settings::PaletteSettings* settings;


    Ui::WidgetPaletteEditor ui;
    color_widgets::ColorDelegate delegate;
    QPalette edited;
};

WidgetPaletteEditor::WidgetPaletteEditor ( app::settings::PaletteSettings* settings, QWidget* parent )
    : QWidget ( parent ), d ( std::make_unique<Private>() )
{
    d->settings = settings;
    d->ui.setupUi ( this );
    d->setup_view();
    d->edited = settings->default_palette;
    for ( auto name : settings->palettes.keys() )
        d->ui.combo_saved->addItem(name);

    if ( settings->palettes.find(settings->selected) != settings->palettes.end() )
        d->ui.combo_saved->setCurrentText(settings->selected);
}

WidgetPaletteEditor::~WidgetPaletteEditor() = default;

void WidgetPaletteEditor::changeEvent ( QEvent* e )
{
    QWidget::changeEvent ( e );

    if ( e->type() == QEvent::LanguageChange ) {
        d->ui.retranslateUi ( this );
    }
}

void WidgetPaletteEditor::update_color ( int row, int column )
{
    auto item = d->ui.palette_view->item(row, column);
    if ( !item )
        return;

    auto group = item->data(Private::ColorGroup).value<QPalette::ColorGroup>();
    auto role = item->data(Private::ColorRole).value<QPalette::ColorRole>();
    auto color = item->data(Qt::DisplayRole).value<QColor>();

    d->edited.setColor(group, role, color);

    if ( group == QPalette::Active )
        d->edited.setColor(QPalette::Inactive, role, color);

    d->ui.preview_widget->setPalette(d->edited);

    if ( d->use_default() )
        d->add_palette({});
}

void WidgetPaletteEditor::select_palette(const QString& name)
{
    if ( d->use_default() )
        d->edited = d->settings->default_palette;
    else
        d->edited = d->settings->palettes[name];

    d->refresh_custom();
    d->ui.preview_widget->setPalette(d->edited);
}

void WidgetPaletteEditor::add_palette()
{
    bool ok = false;
    QString default_name = d->ui.combo_saved->currentText();
    if ( d->use_default() )
        default_name = tr("Custom");

    QString name_hint = QInputDialog::getText(
        this,
        tr("Add Theme"),
        tr("Name"),
        QLineEdit::Normal,
        default_name.isEmpty() ? tr("Custom") : default_name,
        &ok
    );

    if ( !ok )
        return;

    d->add_palette(name_hint);
}

void WidgetPaletteEditor::apply_palette()
{
    if ( d->use_default() )
    {
        d->settings->set_selected("");
    }
    else
    {
        QString name = d->ui.combo_saved->currentText();
        d->settings->palettes[name] = d->edited;
        d->settings->set_selected(name);
    }
}

void WidgetPaletteEditor::remove_palette()
{
    if ( !d->use_default() )
    {
        d->settings->palettes.remove(d->ui.combo_saved->currentText());
        d->ui.combo_saved->removeItem(d->ui.combo_saved->currentIndex());
    }
}
