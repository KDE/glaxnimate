/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "glaxnimate/utils/qstring_exception.hpp"
#include "glaxnimate/utils/i18n.hpp"
#include "glaxnimate/log/log.hpp"


namespace glaxnimate::script {

class ScriptError: public utils::QStringException<std::runtime_error>
{
public:
    using QStringException::QStringException;
};

} // namespace glaxnimate::script
