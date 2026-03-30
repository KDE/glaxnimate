/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "gzip_module.hpp"

#include "tgs_format.hpp"
#include "svgz_format.hpp"

#include <zlib.h>

std::vector<glaxnimate::module::ExternalComponent> glaxnimate::gzip::Module::components() const
{
    return {{QStringLiteral("zlib"), {}, ZLIB_VERSION, QStringLiteral("https://zlib.net/"), "Custom"}};
}

void glaxnimate::gzip::Module::initialize()
{
    register_io_classes<io::lottie::TgsFormat, io::svg::SvgzFormat>();
}
