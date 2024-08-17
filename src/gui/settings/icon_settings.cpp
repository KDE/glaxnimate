/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "icon_settings.hpp"

#include "glaxnimate_settings.hpp"

#include <QGuiApplication>
#include <QPalette>
#include <QStandardItemModel>

#include <KIconTheme>

class glaxnimate::gui::settings::IconSettings::Private
{
public:
    void set_theme(const QString& theme)
    {
        if ( theme == default_theme_name )
        {
            set_theme(get_default_theme_name());
            return;
        }

        KIconTheme::forceThemeForTests(theme);
        QIcon::setThemeName(theme);
    }

    QString get_default_theme_name()
    {
        QPalette palette = QGuiApplication::palette();
        if ( palette.color(QPalette::Button).value() < 100 )
            return QStringLiteral("breeze-dark");
        else
            return QStringLiteral("breeze");
    }

    void clear_theme()
    {
        KIconTheme::forceThemeForTests(QString());
        QIcon::setThemeName(KIconTheme::current());
        qDebug() << KIconTheme::current();
    }

    QStandardItemModel* get_model()
    {
        if ( model )
            return model.get();

        QStringList themes = KIconTheme::list();
        model = std::make_unique<QStandardItemModel>(themes.size() + 2, 1);
        model->setItem(0, 0, item(i18n("System Default"), QString(), system_theme));
        model->setItem(1, 0, item(i18n("Glaxnimate Default"), default_theme_name, get_default_theme_name()));
        int i = 2;
        for ( const auto& theme : themes )
            model->setItem(i++, 0, item(theme, theme, theme));


        return model.get();
    }

    QStandardItem* item(const QString& display, const QString& value, const QString& preview_theme)
    {
        QStandardItem* item = new QStandardItem();
        item->setText(display);
        item->setData(value, Qt::UserRole);
        item->setIcon(QIcon(KIconTheme(preview_theme).iconPathByName(preview_icon, 32, KIconLoader::MatchBest)));
        return item;
    }

    std::unique_ptr<QStandardItemModel> model;
    QString default_theme_name = QStringLiteral("Glaxnimate");
    QString preview_icon = QStringLiteral("document-open");
    QString system_theme;
};

glaxnimate::gui::settings::IconSettings & glaxnimate::gui::settings::IconSettings::instance()
{
    static IconSettings instance;
    return instance;
}

glaxnimate::gui::settings::IconSettings::IconSettings() : d(std::make_unique<Private>())
{
}

void glaxnimate::gui::settings::IconSettings::initialize()
{
    d->system_theme = KIconTheme::current();
    QString theme = GlaxnimateSettings::icon_theme();
    if ( !theme.isEmpty() )
        d->set_theme(theme);
}

void glaxnimate::gui::settings::IconSettings::set_icon_theme(const QString& theme)
{
    if ( theme.isEmpty() )
        d->clear_theme();
    else
        d->set_theme(theme);

    GlaxnimateSettings::setIcon_theme(theme);
}

QAbstractItemModel * glaxnimate::gui::settings::IconSettings::item_model() const
{
    return d->get_model();
}

QString glaxnimate::gui::settings::IconSettings::icon_theme() const
{
    return GlaxnimateSettings::icon_theme();
}


void glaxnimate::gui::settings::IconSettings::palette_change() const
{
    if ( GlaxnimateSettings::icon_theme() == d->default_theme_name )
        d->set_theme(d->get_default_theme_name());
}
