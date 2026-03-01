/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#ifdef GLAXNIMATE_CORE_KDE

#include <KLazyLocalizedString>

namespace glaxnimate::util {

using LazyLocalizedString = KLazyLocalizedString;

} // namespace glaxnimate::util

#else

#include <QString>

namespace {


inline const QString& arg_spread(const QString& str)
{
    return str;
}

template<class Head, class... Args>
QString arg_spread(const QString& str, Head&& head, Args&&... args)
{
    return arg_spread(str.arg(std::forward<Head>(head)), std::forward<Args>(args)...);
}


} // namespace

template<class Initializer, class... Args>
QString i18n(Initializer&& init, Args&&... args)
{
  return arg_spread(QString(std::forward<Initializer>(init)), std::forward<Args>(args)...);
}

#define kli18n(x) x

namespace glaxnimate::util {

class LazyLocalizedString
{
    const char * text;
public:
    constexpr LazyLocalizedString(const char* text = "") : text(text) {}
    constexpr inline const char *untranslatedText() const { return text; }
    QString toString() const { return text; }
};

} // namespace glaxnimate::util
#endif
