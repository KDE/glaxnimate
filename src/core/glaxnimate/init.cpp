/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "init.hpp"

#include <QMetaType>

#include "glaxnimate/math/bezier/meta.hpp"
#include "glaxnimate/io/io_registry.hpp"

void glaxnimate::init()
{
    math::bezier::register_meta();
    glaxnimate::io::IoRegistry::load_formats();
}
