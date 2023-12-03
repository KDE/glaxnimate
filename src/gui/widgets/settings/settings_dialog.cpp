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

#include <KConfigSkeleton>
#include <KLazyLocalizedString>

#include "QtColorWidgets/ColorSelector"

#include "glaxnimate_settings.hpp"
#include <QComboBox>

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
    KPageWidgetItem* page;

    explicit AutoConfigPage(const KLazyLocalizedString& title, const QString& icon, KCoreConfigSkeleton* skeleton, KConfigDialog* parent):
        title(title),
        skeleton(skeleton),
        layout(new QFormLayout(this))
    {
        this->setLayout(layout);

        page = parent->addPage(this, title.toString(), icon);
    }

    AutoConfigPage& add_item_widget(const char* name, QWidget* widget, const KLazyLocalizedString& label, const KLazyLocalizedString& tooltip = {})
    {
        add_item_widget(skeleton->findItem(QString::fromLatin1(name)), widget, label, tooltip);
        return *this;
    }

    AutoConfigPage& add_item(const char* name, const KLazyLocalizedString& label, const KLazyLocalizedString& tooltip = {})
    {
        add_item(skeleton->findItem(QString::fromLatin1(name)), label, tooltip);
        return *this;
    }

    void add_item(const KConfigSkeletonItem* item, const KLazyLocalizedString& label_text, const KLazyLocalizedString& tooltip)
    {
        if ( !item )
            return;

        QWidget* wid = make_setting_widget(item);
        if ( wid )
            add_item_widget(item, wid, label_text, tooltip);
    }

    void add_item_widget(const KConfigSkeletonItem* item, QWidget* widget, const KLazyLocalizedString& label_text, const KLazyLocalizedString& tooltip)
    {
        QLabel* label = new QLabel(this);
        widget->setObjectName(QStringLiteral("kcfg_%1").arg(item->name()));
        label->setBuddy(widget);
        label->setObjectName(QStringLiteral("label_%1").arg(item->name()));
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

    static AutoConfigPage& builder(const KLazyLocalizedString& title, const QString& icon, KCoreConfigSkeleton* skeleton, KConfigDialog* parent)
    {
        auto page = new AutoConfigPage(title, icon, skeleton, parent);
        return *page;
    }
};

QComboBox* combo_from_choices(const QVariantMap& choices)
{
    auto wid = new QComboBox();
    // int index = 0;
    // QVariant val = opt.get_variant(target);
    for ( const QString& key : choices.keys() )
    {
        QVariant choice = choices[key];
        wid->addItem(key, choice);
        // if ( choice == val )
            // wid->setCurrentIndex(index);
        // index++;
    }
    return wid;
}

} // namespace

SettingsDialog::SettingsDialog(QWidget *parent)
: KConfigDialog(parent, QStringLiteral("settings"), GlaxnimateSettings::self())
{
    resize(940, 700);

    KCoreConfigSkeleton* skeleton = GlaxnimateSettings::self();

    AutoConfigPage::builder(kli18nc("Settings", "User Interface"), "preferences-desktop-theme", skeleton, this)
        .add_item("startup_dialog", kli18n("Show startup dialog"))
        .add_item(
            "timeline_scroll_horizontal",
            kli18n("Horizontal Timeline Scroll"),
            kli18n("If enabled, the timeline will scroll horizontally by default and vertically with Shift or Alt")
        )
    ;

    auto seconds = new QDoubleSpinBox();
    seconds->setSuffix(i18nc("Seconds suffix", "s"));

    AutoConfigPage::builder(kli18nc("Settings", "New Animation Defaults"), "video-webm", skeleton, this)
        .add_item("width", kli18n("Canvas Width"))
        .add_item("height", kli18n("Canvas Height"))
        .add_item("fps", kli18n("FPS"), kli18n("Frames per secons"))
        .add_item_widget("duration", seconds, kli18n("Duration"), kli18n("Duration in secons"))
    ;

    AutoConfigPage::builder(kli18nc("Settings", "Open / Save"), "kfloppy", skeleton, this)
        .add_item("max_recent_files", kli18n("Max Recent Files"), kli18n("Number of items to show in the recent files menu"))
        .add_item("backup_frequency", kli18n("Backup Frequency"), kli18n("Number of items to show in the recent files menu"))
        .add_item("use_native_io_dialog", kli18n("Use system file dialog"))
    ;
}
