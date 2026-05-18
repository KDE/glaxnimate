/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "ps_value.hpp"

namespace glaxnimate::ps {

class FontDatabase
{
public:
    FontDatabase(const QStringList& families);
    FontDatabase();

    static QString normalized_typeface_name(const QString& family);

    QString get_family_name(const QByteArrayView& family) const;
    std::pair<QString, QString> family_and_style(const QByteArrayView& name) const;

private:
    std::map<QString, QString> families;
};

struct FontWrapper
{
    static constexpr const int CustomFontType = 50;

    FontWrapper(ValueDict font) : font(std::move(font)) {}

    bool is_font() const;

    static ValueDict font_from_database(const QByteArray &name);
    static ValueDict font_from_qfont(const QFont& font);
    static FontWrapper default_font();

    ValueDict scaled(float scale) const;

    ValueDict transformed(const QTransform& tf) const;

    std::optional<QString> decode_text(const QByteArray& text) const;

    QString family() const;
    float size() const;
    int weight() const;
    QString style() const;
    QTransform transform() const;

    ValueDict font;
};

} // namespace glaxnimate::ps
