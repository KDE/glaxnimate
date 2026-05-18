/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ps_font.hpp"

#include <QFontDatabase>
#include <QFontMetricsF>
#include <QMetaEnum>

#include "glaxnimate/math/math.hpp"

#include "ps_gstate.hpp"

using namespace glaxnimate;
using namespace glaxnimate::ps;

FontDatabase::FontDatabase(const QStringList &families)
{
    for ( const auto& fam : families )
        this->families[normalized_typeface_name(fam)] = fam;
}

FontDatabase::FontDatabase() : FontDatabase(QFontDatabase::families())
{}

QString FontDatabase::normalized_typeface_name(const QString& family)
{
    return family.toLower().remove(' ');
}

QString FontDatabase::get_family_name(const QByteArrayView &family) const
{
    auto it = families.find(normalized_typeface_name(QString::fromLatin1(family)));
    if ( it == families.end() )
        return {};
    return it->second;
}

std::pair<QString, QString> FontDatabase::family_and_style(const QByteArrayView &name) const
{
    int style_index = name.indexOf('-');
    QByteArrayView family;
    QByteArrayView style;
    if ( style_index == -1 )
    {
        family = name;
    }
    else
    {
        family = name.first(style_index);
        style = name.sliced(style_index + 1);
    }

    QString norm_fam = get_family_name(family);
    QString norm_style;
    for ( char c : style )
    {
        if ( std::isupper(c) )
            norm_style.push_back(' ');
        norm_style.push_back(c);
    }

    if ( !norm_fam.isEmpty() )
    {
        QStringList styles = QFontDatabase::styles(norm_fam);
        if ( !styles.contains(norm_style) )
        {
            norm_style = QStringLiteral("Regular");
            if ( !styles.contains(norm_style) && !styles.empty() )
                norm_style = styles[0];
        }
    }

    return {norm_fam, norm_style};
}

bool FontWrapper::is_font() const
{
    return font.contains("FontType") && font.contains("FontMatrix");
}

ValueDict FontWrapper::font_from_database(const QByteArray &name)
{
    static FontDatabase db;
    auto famsty = db.family_and_style(name);
    return font_from_qfont(QFontDatabase::font(famsty.first, famsty.second, 1));
}


ValueDict FontWrapper::font_from_qfont(const QFont &font)
{

    ValueDict font_info;

    QString f_family;
    QString f_style;
    float f_size;
    int f_weight;
    bool f_fixed;
    bool f_italic;
    int underline_pos;

    // QFontInfo doesn't work without GUI application
    if ( font.family().isEmpty() )
    {
        f_family = QStringLiteral("Unknown");
        f_style = font.styleName();
        f_size = font.pointSizeF();
        f_weight = font.weight();
        f_fixed = font.fixedPitch();
        f_italic = font.italic();
        underline_pos = 0;
    }
    else
    {
        // I think this gives more accurate info
        QFontInfo info(font);
        QFontMetricsF metrics(font);

        f_family = info.family();
        f_style = info.styleName();
        f_size = info.pointSizeF();
        f_weight = info.weight();
        f_fixed = info.fixedPitch();
        f_italic = info.italic();
        underline_pos = metrics.underlinePos();
    }


    QByteArray family = f_family.toLatin1();
    QByteArray full_name = family + ' ' + f_style.toLatin1();
    QByteArray weight;
    auto meta_weight = QMetaEnum::fromType<QFont::Weight>();
    weight = meta_weight.valueToKey(math::bound(100, f_weight / 100 * 100, 900));


    font_info["FamilyName"] = family;
    font_info["isFixedPitch"] = f_fixed;
    font_info["FullName"] = full_name;
    font_info["Weight"] = weight;
    font_info["ItalicAngle"] = f_italic ? -12.f : 0.f;
    font_info["UnderlineThickness"] = f_weight / 10;
    font_info["UnderlinePosition"] = -underline_pos;

    ValueDict font_dict;
    // Custom font type
    font_dict["FontType"] = CustomFontType;
    font_dict["FontMatrix"] = ValueArray{1, 0, 0, 1, 0, 0};
    font_dict["FontName"] = Value::from<Value::String>(
        QStringLiteral("%1-%2").arg(
            QString(f_family).remove(' '),
            QString(f_style).remove(' ')
        ).toLatin1(),
    Value::Name);
    font_dict["FontInfo"] = std::move(font_info);

    // Custom Stuff
    font_dict["FontSize"] = f_size;
    font_dict["FontTransformed"] = false;
    font_dict["FontNumWeight"] = f_weight;
    font_dict["FontStyle"] = f_style.toLatin1();
    /*ValueArray stylesv;
    stylesv.reserve(styles.size());
    for ( const auto& style : styles )
        stylesv.emplace_back(style.toLatin1());
    font_dict["FontStyles"] = std::move(stylesv);*/

    return font_dict;
}

FontWrapper FontWrapper::default_font()
{
    static FontWrapper default_font(font_from_qfont(QFont()));
    return default_font;
}

ValueDict FontWrapper::scaled(float scale) const
{
    auto font_copy = font.shallow_copy();

    if ( font["FontType"].cast<int>() == CustomFontType )
    {
        if ( !font["FontTransformed"].cast<bool>() )
        {
            font_copy["FontSize"] = font["FontSize"].cast<float>() * scale;
            scale = 1;
        }
    }

    auto fm = *array_to_matrix(font.get("FontMatrix").cast<ValueArray>());
    fm.scale(scale, scale);
    font_copy["FontMatrix"] = matrix_to_array(fm);

    QTransform sm = array_to_matrix(font.get("ScaleMatrix").cast<ValueArray>()).value_or(QTransform());
    sm.scale(scale, scale);
    font_copy["ScaleMatrix"] = matrix_to_array(sm);

    font_copy["OrigFont"] = font;

    return font_copy;
}

ValueDict FontWrapper::transformed(const QTransform& tf) const
{
    auto font_copy = font.shallow_copy();

    auto fm = *array_to_matrix(font.get("FontMatrix").cast<ValueArray>());
    fm *= tf;
    font_copy["FontMatrix"] = matrix_to_array(fm);

    QTransform sm = array_to_matrix(font.get("ScaleMatrix").cast<ValueArray>()).value_or(QTransform());
    sm *= tf;
    font_copy["ScaleMatrix"] = matrix_to_array(sm);

    font_copy["OrigFont"] = font;

    if ( font["FontType"].cast<int>() == CustomFontType )
    {
        font_copy["FontTransformed"] = true;
    }

    return font_copy;

}

std::optional<QString> FontWrapper::decode_text(const QByteArray &text) const
{
    if ( font["FontType"].cast<int>() == CustomFontType )
    {
        return QString::fromUtf8(text);
    }
    return {};
}

QString FontWrapper::family() const
{
    return QString::fromLatin1(font["FamilyName"].cast<String>().bytes());
}

float FontWrapper::size() const
{
    if ( font["FontType"].cast<int>() == CustomFontType )
    {
        return font["FontSize"].cast<float>();
    }

    return 1;
}

int FontWrapper::weight() const
{
    if ( font["FontType"].cast<int>() == CustomFontType )
    {
        return font["FontNumWeight"].cast<int>();
    }

    return QFont::Normal;
}

QString FontWrapper::style() const
{
    if ( font["FontType"].cast<int>() == CustomFontType )
    {
        return QString::fromLatin1(font["FontStyle"].cast<String>().bytes());
    }

    return {};
}

QTransform FontWrapper::transform() const
{
    return array_to_matrix(font.get("ScaleMatrix").cast<ValueArray>()).value_or(QTransform());
}
