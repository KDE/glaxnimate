/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <vector>
#include <utility>
#include <memory>

#include <QDir>
#include <QVariant>
#include <QIcon>
#include <QAbstractListModel>

#include <KConfig>


namespace glaxnimate::gui {

struct IconPath
{
    QString name;
    QDir path;
    int size;
};

struct IconTheme
{
    QString id;
    QIcon example;
    QDir path;
    std::unique_ptr<KConfig> index;
    QStringList inherits;
    std::vector<IconPath> paths;

    IconTheme(const QString& id, const QDir& dir);
    IconTheme() = default;
    IconTheme(IconTheme&&) = default;
    IconTheme& operator=(IconTheme&&) = default;

    QString name() const;
    QString comment() const;
    QIcon find_icon(const QString& name) const;

    void load_example();
};


class IconThemeManager : public QObject
{
    Q_OBJECT

public:
    const std::vector<IconTheme>& available_themes() const;
    const IconTheme& theme(const QString& id) const;

    static IconThemeManager& instance()
    {
        static IconThemeManager instance;
        return instance;
    }

    /**
     * \brief Theme name that respects the dark/light palette
     */
    QString default_theme() const;

    void set_theme_and_fallback(const QString& theme) const;

public:
    void update_from_application_palette() const;
    void set_theme(const QString& theme) const;

private:
    IconThemeManager();
    ~IconThemeManager();
    IconThemeManager(const IconThemeManager&) = delete;
    IconThemeManager& operator=(const IconThemeManager&) = delete;

    class Private;
    std::unique_ptr<Private> d;
};

class IconThemeModel : public QAbstractListModel
{
public:
    IconThemeModel();

    QVariant data(const QModelIndex & index, int role) const override;
    int rowCount(const QModelIndex & parent) const override;

private:
    const std::vector<IconTheme>* available_themes;
};

} // namespace glaxnimate::gui
