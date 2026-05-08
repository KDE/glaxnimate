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

using namespace glaxnimate::ps;

bool glaxnimate::ps::is_font(const ValueDict &dict)
{
    return dict.contains("FontType") && dict.contains("FontMatrix");
}

ValueDict font_from_database(const QByteArray &name)
{

    auto styles = QFontDatabase::styles(name);
    QFont font = QFontDatabase::font(name, styles.empty() ? u"Regular"_s : styles[0], 1);

    // I think this gives more accurate info
    QFontInfo info(font);
    QFontMetricsF metrics(font);

    ValueDict font_info;
    QByteArray family = info.family().toLatin1();
    QByteArray full_name = family + ' ' + info.styleName().toLatin1();
    QByteArray weight;
    auto meta_weight = QMetaEnum::fromType<QFont::Weight>();
    weight = meta_weight.valueToKey(math::bound(100, info.weight() / 100 * 100, 900));


    font_info["FamilyName"] = family;
    font_info["isFixedPitch"] = info.fixedPitch();
    font_info["FullName"] = full_name;
    font_info["Weight"] = weight;
    font_info["ItalicAngle"] = info.italic() ? -12.f : 0.f;
    font_info["UnderlineThickness"] = info.weight() / 10;
    font_info["UnderlinePosition"] = -metrics.underlinePos();


    ValueDict font_dict;
    // Custom font type
    font_dict["FontType"] = CustomFontType;
    font_dict["FontMatrix"] = ValueArray{1, 0, 0, 1, 0, 0};
    font_dict["FontName"] = Value::from<Value::String>(family, Value::Name);
    font_dict["FontInfo"] = std::move(font_info);

    // Custom Stuff
    font_dict["FontSize"] = info.pointSizeF();
    font_dict["FontTransformed"] = false;
    ValueArray stylesv;
    stylesv.reserve(styles.size());
    for ( const auto& style : styles )
        stylesv.emplace_back(style.toLatin1());
    font_dict["FontStyles"] = std::move(stylesv);

    return font_dict;
}

ValueDict scale_font(const ValueDict &font, float scale)
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

    auto fm = to_matrix(font.get("FontMatrix").cast<ValueArray>());
    fm.scale(scale, scale);
    font_copy["FontMatrix"] = matrix_to_array(fm);

    QTransform sm = to_matrix(font.get("ScaleMatrix").cast<ValueArray>());
    sm.scale(scale, scale);
    font_copy["ScaleMatrix"] = matrix_to_array(sm);

    font_copy["OrigFont"] = font;

    return font_copy;
}

ValueDict transform_font(const ValueDict &font, const QTransform& tf)
{
    auto font_copy = font.shallow_copy();

    auto fm = to_matrix(font.get("FontMatrix").cast<ValueArray>());
    fm *= tf;
    font_copy["FontMatrix"] = matrix_to_array(fm);

    QTransform sm = to_matrix(font.get("ScaleMatrix").cast<ValueArray>());
    sm *= tf;
    font_copy["ScaleMatrix"] = matrix_to_array(sm);

    font_copy["OrigFont"] = font;

    if ( font["FontType"].cast<int>() == CustomFontType )
    {
        font_copy["FontTransformed"] = true;
    }

    return font_copy;

}

