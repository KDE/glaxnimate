/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "icon_theme.hpp"

#include <QPalette>
#include <QGuiApplication>

#include <KLocalizedString>
#include <KConfigGroup>

using namespace glaxnimate::gui;

QString glaxnimate::gui::IconTheme::name() const
{
    if ( index )
        return index->group(QStringLiteral("Icon Theme")).readEntry("Name", id);
    return i18nc("Name of the default icon theme", "Glaxnimate Default");
}

QString glaxnimate::gui::IconTheme::comment() const
{
    if ( index )
        return index->group(QStringLiteral("Icon Theme")).readEntry("Comment", id);
    return i18n("Default icon theme");
}

QIcon glaxnimate::gui::IconTheme::find_icon(const QString& name) const
{
    if ( paths.empty() )
        return {};

    QString hint;
    std::map<int, QString> sizes;
    for ( const auto& path : paths )
    {
        for ( const auto& entry : path.path.entryInfoList(QDir::Files) )
        {
            if ( entry.baseName() == name )
            {
                sizes[path.size] = entry.absoluteFilePath();
                break;
            }
        }
    }

    if ( sizes.empty() )
        return {};

    QIcon icon(sizes.rbegin()->second);
    for ( const auto& p : sizes )
        icon.addFile(p.second, QSize(p.first, p.first));
    return icon;
}

glaxnimate::gui::IconTheme::IconTheme(const QString& id, const QDir& dir):
    id(id),
    path(dir)
{
    if ( !id.isEmpty() )
    {
        index = std::make_unique<KConfig>(dir.absoluteFilePath(QStringLiteral("index.theme")), KConfig::SimpleConfig);
        inherits = index->group(QStringLiteral("Icon Theme")).readEntry("Inherits", QStringList{});

        for ( const auto& group_name : index->groupList() )
        {
            auto group = index->group(group_name);
            if ( group.hasKey("Size") )
            {
                int size = group.readEntry("Size", 1);
                int scale = group.readEntry("Scale", 1);
                if ( scale == 1 )
                    paths.push_back({group_name, path.absoluteFilePath(group_name), size});
            }
        }
    }
}

void glaxnimate::gui::IconTheme::load_example()
{
    if ( !index )
        return;
    // Several themes have an `example` not included in the theme itself o_O
    // QString example_name = index->group(QStringLiteral("Icon Theme")).readEntry("Example", "folder");
    QString example_name = QStringLiteral("document-open");
    example = find_icon(example_name);
}


class IconThemeManager::Private
{
public:
    QString default_theme_light = QStringLiteral("breeze");
    QString default_theme_dark = QStringLiteral("breeze-dark");
    std::vector<IconTheme> available_themes;
    std::map<QString, const IconTheme*> id_map;
};

glaxnimate::gui::IconThemeManager::IconThemeManager()
    : d(std::make_unique<Private>())
{
}

glaxnimate::gui::IconThemeManager::~IconThemeManager() = default;

const IconTheme & glaxnimate::gui::IconThemeManager::theme(const QString& id) const
{
    auto it = d->id_map.find(id);
    if ( it == d->id_map.end() )
        return d->available_themes[0];
    return *it->second;
}

const std::vector<IconTheme>& glaxnimate::gui::IconThemeManager::available_themes() const
{
    if ( d->available_themes.empty() )
    {
        std::map<QString, IconTheme> found_themes;
        for ( QDir search : QIcon::themeSearchPaths() )
        {
            for ( const auto& avail : search.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot) )
            {
                QDir subdir(avail.filePath());
                auto id = avail.baseName();
                if ( subdir.exists("index.theme") && !found_themes.count(id) && id != QStringLiteral("default") )
                {
                    IconTheme theme(id, subdir);
                    if ( !theme.paths.empty() )
                        found_themes.emplace(id, std::move(theme));
                }
            }
        }

        d->available_themes.reserve(found_themes.size() + 1);
        d->available_themes.emplace_back(i18nc("Name of the default icon theme", "Glaxnimate Default"), QString());
        for ( auto& theme : found_themes )
            d->available_themes.push_back(std::move(theme.second));

        for ( const auto& theme : d->available_themes )
            d->id_map[theme.id] = &theme;

        for ( auto& theme : d->available_themes )
            theme.load_example();

        for ( auto& theme : d->available_themes )
        {
            if ( theme.example.isNull() )
            {
                for ( const auto& parent_name : theme.inherits )
                {
                    auto& parent = this->theme(parent_name);
                    if ( !parent.example.isNull() )
                    {
                        theme.example = parent.example;
                        break;
                    }
                }
            }
        }
    }

    return d->available_themes;
}

QString glaxnimate::gui::IconThemeManager::default_theme() const
{
    QPalette palette = QGuiApplication::palette();
    if ( palette.color(QPalette::Button).value() < 100 )
        return d->default_theme_dark;
    else
        return d->default_theme_light;
}

void glaxnimate::gui::IconThemeManager::set_theme(const QString& theme) const
{
    if ( theme.isEmpty() )
        QIcon::setThemeName(default_theme());
    else
        QIcon::setThemeName(theme);
}

void glaxnimate::gui::IconThemeManager::update_from_application_palette() const
{
    set_theme_and_fallback(QIcon::themeName());
}

void glaxnimate::gui::IconThemeManager::set_theme_and_fallback(const QString& theme) const
{
    QString default_theme = this->default_theme();
    QIcon::setFallbackThemeName(default_theme);

    if ( theme == d->default_theme_light || theme == d->default_theme_dark || theme.isEmpty() )
        QIcon::setThemeName(default_theme);
    else
        set_theme(theme);
}

glaxnimate::gui::IconThemeModel::IconThemeModel()
    : available_themes(&IconThemeManager::instance().available_themes())
{
}

QVariant glaxnimate::gui::IconThemeModel::data(const QModelIndex& index, int role) const
{
    int row = index.row();
    if ( !index.isValid() || row < 0 || row >= int(available_themes->size()) )
        return {};

    const IconTheme& theme = (*available_themes)[row];

    switch ( role )
    {
        case Qt::DisplayRole:
            return theme.name();
        case Qt::ToolTipRole:
            return theme.comment();
        case Qt::DecorationRole:
            return theme.example;
        case Qt::UserRole:
            return theme.id;
    }
    return {};
}

int glaxnimate::gui::IconThemeModel::rowCount(const QModelIndex& parent) const
{
    if ( parent.isValid() )
        return 0;
    return available_themes->size();
}

