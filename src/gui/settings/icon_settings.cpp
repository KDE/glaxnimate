/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "icon_settings.hpp"

#include "glaxnimate_settings.hpp"

#include <QtGlobal>
#include <QGuiApplication>
#include <QPalette>
#include <QStandardItemModel>

#include <KIconTheme>

class glaxnimate::gui::settings::IconSettings::Private
{
public:
    void set_theme(QString theme)
    {
        if ( theme.isEmpty() )
        {
            theme = system_theme;
        }
        // No `else` to allow system_theme to be the same as default_theme_name
        if ( theme == default_theme_name )
        {
            theme = get_default_theme_name();
        }

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

    QStandardItemModel* get_model()
    {
        if ( model )
            return model.get();

        themes = KIconTheme::list();
        model = std::make_unique<QStandardItemModel>(themes.size() + 2, 6);
        int i = 0;
        item(i++, i18n("System Default"), QString(), system_theme);
        item(i++, i18n("Glaxnimate Default"), default_theme_name, get_default_theme_name());
        for ( const auto& theme : themes )
            item(i++, theme, theme, theme);

        return model.get();
    }

    void item(int row, const QString& display, const QString& value, const QString& preview_theme)
    {
        QStandardItem* item = new QStandardItem();
        item->setText(display);
        item->setData(value, Qt::UserRole);
        int col = 0;
        // item->setIcon(QIcon(KIconTheme(preview_theme).iconPathByName(preview_icon, 32, KIconLoader::MatchBest)));
        model->setItem(row, col++, item);
        KIconTheme theme(preview_theme);
        model->setItem(row, col++, preview_icon(theme, QStringLiteral("document-open")));
        model->setItem(row, col++, preview_icon(theme, QStringLiteral("document-save")));
        model->setItem(row, col++, preview_icon(theme, QStringLiteral("draw-bezier-curves")));
        model->setItem(row, col++, preview_icon(theme, QStringLiteral("draw-polygon-star")));
        model->setItem(row, col++, preview_icon(theme, QStringLiteral("layer-raise")));
        model->setItem(row, col++, preview_icon(theme, QStringLiteral("align-horizontal-center")));
    }

    QStandardItem* preview_icon(const KIconTheme& theme, const QString& icon)
    {
        QString path = theme.iconPathByName(icon, 32, KIconLoader::MatchBest);
        if ( path.isEmpty() )
        {
            for ( const auto& inh : theme.inherits() )
            {
                path = KIconTheme(inh).iconPathByName(icon, 32, KIconLoader::MatchBest);
                if ( !path.isEmpty() )
                    break;
            }
            if ( path.isEmpty() )
                path = KIconTheme(QStringLiteral("breeze")).iconPathByName(icon, 32, KIconLoader::MatchBest);
        }

        return new QStandardItem(QIcon(path), QString());
    }

    std::unique_ptr<QStandardItemModel> model;
    QString default_theme_name = QStringLiteral("Glaxnimate");
    QString system_theme;
    QStringList themes;
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
#if defined(Q_OS_ANDROID) || defined(Q_OS_WIN) || defined(Q_OS_DARWIN)
    d->system_theme = d->default_theme_name;
#else
    d->system_theme = KIconTheme::current();
#endif
    QString theme = GlaxnimateSettings::icon_theme();
    d->set_theme(theme);
}

void glaxnimate::gui::settings::IconSettings::set_icon_theme(const QString& theme)
{
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

QModelIndex glaxnimate::gui::settings::IconSettings::current_item_index() const
{
    auto model = d->get_model();
    int row;
    QString theme = icon_theme();
    if ( theme.isEmpty() )
        row = 0;
    else if ( theme == d->default_theme_name )
        row = 1;
    else
        row = d->themes.indexOf(theme) + 2;
    return model->index(row, 0);
}

void glaxnimate::gui::settings::IconSettings::set_current_item_index(const QModelIndex& index)
{
    int row = index.row();
    if ( row < 0 || row >= d->themes.size() + 2 )
        return;
    if ( row == 0 )
        set_icon_theme({});
    else if ( row == 1 )
        set_icon_theme(d->default_theme_name);
    else
        set_icon_theme(d->themes[row - 2]);
}
