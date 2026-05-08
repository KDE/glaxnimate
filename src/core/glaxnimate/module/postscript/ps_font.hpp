/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "ps_value.hpp"

namespace glaxnimate::ps {

constexpr const int CustomFontType = 50;

bool is_font(const ValueDict& dict);

ValueDict font_from_database(const QByteArray &name);

ValueDict scale_font(const ValueDict& font, float scale);

ValueDict transform_font(const ValueDict& font, const QTransform& tf);


} // namespace glaxnimate::ps
