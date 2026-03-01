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


#define i18n(x) x
#define kli18n(x) x

namespace glaxnimate::util {

using LazyLocalizedString = QString;

} // namespace glaxnimate::util
#endif
