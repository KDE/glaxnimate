/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "settings_dialog.hpp"

#include <QFormLayout>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QDir>
#include <QIcon>
#include <QComboBox>
#include <QListView>
#include <QPushButton>
#include <QTableView>
#include <QHeaderView>

#include <KConfigSkeleton>
#include <KLazyLocalizedString>

#include "QtColorWidgets/ColorSelector"

#include "app/widgets/no_close_on_enter.hpp"
#include "glaxnimate_settings.hpp"
#include "settings/icon_settings.hpp"

using namespace glaxnimate::gui;

namespace {

struct Entry
{
    KLazyLocalizedString label_text;
    KLazyLocalizedString tooltip_text;
    QLabel* label;
    QWidget* widget;

    void retranslate() const
    {
        label->setText(label_text.toString());
        QString tooltip = tooltip_text.isEmpty() ? QString() : tooltip_text.toString();
        label->setToolTip(tooltip);
        widget->setToolTip(tooltip);
        widget->setWhatsThis(tooltip);
    }
};

class AutoConfigPage : public QWidget
{
public:
    std::vector<Entry> entries;
    KLazyLocalizedString title;
    KCoreConfigSkeleton* skeleton;
    QFormLayout* layout;
    KPageWidgetItem* page = nullptr;

    explicit AutoConfigPage(const KLazyLocalizedString& title):
        title(title),
        layout(new QFormLayout(this))
    {
        this->setLayout(layout);
    }

    void add_item(const KConfigSkeletonItem* item, const KLazyLocalizedString& label_text, const KLazyLocalizedString& tooltip)
    {
        if ( !item )
            return;

        QWidget* wid = make_setting_widget(item);
        if ( wid )
            add_item_widget(item, wid, label_text, tooltip, true);
    }

    void add_item_widget(const KConfigSkeletonItem* item, QWidget* widget, const KLazyLocalizedString& label_text, const KLazyLocalizedString& tooltip, bool auto_name)
    {
        if ( auto_name )
            widget->setObjectName(QStringLiteral("kcfg_%1").arg(item->name()));
        add_widget(widget, label_text, tooltip);
    }

    void add_widget(QWidget* widget, const KLazyLocalizedString& label_text, const KLazyLocalizedString& tooltip)
    {
        QLabel* label = new QLabel(this);
        label->setBuddy(widget);
        layout->addRow(label, widget);
        entries.push_back({label_text, tooltip, label, widget});
        entries.back().retranslate();
    }

    static QWidget* make_setting_widget(const KConfigSkeletonItem* item)
    {
        QVariant val = item->property();

        int type = val.userType();
        if ( type == QMetaType::Int )
        {
            return new QSpinBox();
        }
        else if ( type == QMetaType::Bool )
        {
            return new QCheckBox();
        }
        else if ( type == QMetaType::Float || type == QMetaType::Double )
        {
            return new QDoubleSpinBox();
        }
        else if ( type == QMetaType::QString )
        {
            return new QLineEdit();
        }
        else if ( type == QMetaType::QColor )
        {
            auto wid = new color_widgets::ColorSelector();
            wid->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            return wid;
        }

        return nullptr;
    }

    void retranslate()
    {
        page->setName(title.toString());
        for ( const auto& entry : entries )
            entry.retranslate();
    }

protected:
    void changeEvent ( QEvent* e ) override
    {
        QWidget::changeEvent(e);

        if ( e->type() == QEvent::LanguageChange)
            retranslate();
    }
};

class AutoConfigBuilder
{
public:
    AutoConfigPage* page;
    KCoreConfigSkeleton* skeleton;
    QString icon;
    KConfigDialog* parent;

    AutoConfigBuilder(const KLazyLocalizedString& title, const QString& icon, KCoreConfigSkeleton* skeleton, KConfigDialog* parent):
        page(new AutoConfigPage(title)),
        skeleton(skeleton),
        icon(icon),
        parent(parent)
    {
    }

    AutoConfigBuilder& add_item_widget(const char* name, QWidget* widget, const KLazyLocalizedString& label, const KLazyLocalizedString& tooltip = {}, bool auto_name = true)
    {
        page->add_item_widget(skeleton->findItem(QString::fromLatin1(name)), widget, label, tooltip, auto_name);
        return *this;
    }

    AutoConfigBuilder& add_item(const char* name, const KLazyLocalizedString& label, const KLazyLocalizedString& tooltip = {})
    {
        page->add_item(skeleton->findItem(QString::fromLatin1(name)), label, tooltip);
        return *this;
    }

    KPageWidgetItem* commit()
    {
        page->page = parent->addPage(page, page->title.toString(), icon);
        return page->page;
    }

    AutoConfigBuilder& add_widget(QWidget* widget, const KLazyLocalizedString& label, const KLazyLocalizedString& tooltip = {})
    {
        page->add_widget(widget, label, tooltip);
        return *this;
    }
};

} // namespace

class SettingsDialog::Private
{
public:
    KPageWidgetItem* ui_page = nullptr;

    QComboBox* color_scheme_view = nullptr;
    std::map<KPageWidgetItem*, settings::CustomSettingsGroup*> custom_pages;
    app::widgets::NoCloseOnEnter ncoe;
    QTableView* icon_view = nullptr;
};

SettingsDialog::SettingsDialog(KXmlGuiWindow *parent)
    : KConfigDialog(parent, QStringLiteral("settings"), GlaxnimateSettings::self()),
    d(std::make_unique<Private>())
{
    resize(940, 700);
    installEventFilter(&d->ncoe);

    KCoreConfigSkeleton* skeleton = GlaxnimateSettings::self();

    d->icon_view = new QTableView();
    d->icon_view->setModel(gui::settings::IconSettings::instance().item_model());
    d->icon_view->horizontalHeader()->setVisible(false);
    d->icon_view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    d->icon_view->horizontalHeader()->setDefaultSectionSize(32);
    d->icon_view->setIconSize(QSize(32, 32));
    d->icon_view->verticalHeader()->setVisible(false);
    d->icon_view->setSelectionMode(QAbstractItemView::SingleSelection);
    d->icon_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->icon_view->setShowGrid(false);

    d->ui_page = AutoConfigBuilder(kli18nc("Settings", "User Interface"), "preferences-desktop-theme", skeleton, this)
        .add_item("startup_dialog", kli18n("Show startup dialog"))
        .add_item(
            "timeline_scroll_horizontal",
            kli18n("Horizontal Timeline Scroll"),
            kli18n("If enabled, the timeline will scroll horizontally by default and vertically with Shift or Alt")
        )
        .add_item_widget("icon_theme", d->icon_view, kli18n("Icon Theme"), kli18n("Icon theme for the application"), false)
        .commit()
    ;

    auto seconds = new QDoubleSpinBox();
    seconds->setSuffix(i18nc("Seconds suffix", "s"));

    AutoConfigBuilder(kli18nc("Settings", "New Animation Defaults"), "video-webm", skeleton, this)
        .add_item("width", kli18n("Canvas Width"))
        .add_item("height", kli18n("Canvas Height"))
        .add_item("fps", kli18n("FPS"), kli18n("Frames per seconds"))
        .add_item_widget("duration", seconds, kli18n("Duration"), kli18n("Duration in seconds"))
        .commit()
    ;

    AutoConfigBuilder(kli18nc("Settings", "Open / Save"), "kfloppy", skeleton, this)
        .add_item("max_recent_files", kli18n("Max Recent Files"), kli18n("Number of items to show in the recent files menu"))
        .add_item("backup_frequency", kli18n("Backup Frequency"), kli18n("Number of items to show in the recent files menu"))
        .add_item("use_native_io_dialog", kli18n("Use system file dialog"))
        .commit()
    ;

    for ( const auto& group : GlaxnimateSettings::self()->custom_groups() )
    {
        auto page = addPage(group->make_widget(this), group->label().toString(), group->icon());
        d->custom_pages[page] = group.get();
    }
}

SettingsDialog::~SettingsDialog() = default;

void glaxnimate::gui::SettingsDialog::updateSettings()
{
    auto iter = d->custom_pages.find(currentPage());
    if ( iter != d->custom_pages.end() )
        iter->second->save(*GlaxnimateSettings::self()->config());

    KConfigDialog::updateSettings();

    gui::settings::IconSettings::instance().set_current_item_index(d->icon_view->currentIndex());
}

void glaxnimate::gui::SettingsDialog::updateWidgets()
{
    d->icon_view->setCurrentIndex(settings::IconSettings::instance().current_item_index());
}
