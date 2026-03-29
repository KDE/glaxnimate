/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "glaxnimate/module/module.hpp"

${GLAXNIMATE_MODULES_INCLUDES}

void glaxnimate::module::Registry::register_loaded_modules(Registry& registry)
{
${GLAXNIMATE_MODULES_REGISTER}
}
