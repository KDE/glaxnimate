/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <vector>
#include <QString>

#include "glaxnimate/renderer/renderer.hpp"

#pragma once

namespace glaxnimate::io::svg::detail {

const std::vector<std::pair<renderer::BlendMode, QString>> blend_modes = {
    {renderer::BlendMode::Normal, "normal"},
    {renderer::BlendMode::Multiply, "multiply"},
    {renderer::BlendMode::Screen, "screen"},
    {renderer::BlendMode::Overlay, "overlay"},
    {renderer::BlendMode::Darken, "darken"},
    {renderer::BlendMode::Lighten, "lighten"},
    {renderer::BlendMode::ColorDodge, "color-dodge"},
    {renderer::BlendMode::ColorBurn, "color-burn"},
    {renderer::BlendMode::HardLight, "hard-light"},
    {renderer::BlendMode::SoftLight, "soft-light"},
    {renderer::BlendMode::Difference, "difference"},
    {renderer::BlendMode::Exclusion, "exclusion"},
    {renderer::BlendMode::Hue, "hue"},
    {renderer::BlendMode::Saturation, "saturation"},
    {renderer::BlendMode::Color, "color"},
    {renderer::BlendMode::Luminosity, "luminosity"},
    {renderer::BlendMode::Add, "plus-lighter"},
};

template<class T>
QString enum_to_svg(const T& value, const std::vector<std::pair<T, QString>>& pairs, const QString& default_val)
{
    for ( const auto& p : pairs )
        if ( p.first == value )
            return p.second;
    return default_val;
}

template<class T>
T enum_from_svg(const QString& value, const std::vector<std::pair<T, QString>>& pairs, const T& default_val)
{
    for ( const auto& p : pairs )
        if ( p.second == value )
            return p.first;
    return default_val;
}

} // namespace glaxnimate::io::svg::detail
