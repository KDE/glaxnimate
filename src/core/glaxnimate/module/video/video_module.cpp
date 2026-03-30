/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "video_module.hpp"
#include "video_format.hpp"

#include <libavutil/version.h>
#include <libavformat/version.h>
#include <libavcodec/version.h>
#include <libswscale/version.h>

std::vector<glaxnimate::module::ExternalComponent> glaxnimate::video::Module::components() const
{
    return {
        {QStringLiteral("avutil"), {}, LIBAVUTIL_IDENT, QStringLiteral("https://libav.org/"), "LGPL"},
        {QStringLiteral("avformat"), {}, LIBAVFORMAT_IDENT, QStringLiteral("https://libav.org/"), "LGPL"},
        {QStringLiteral("avcodec"), {}, LIBAVCODEC_IDENT, QStringLiteral("https://libav.org/"), "LGPL"},
        {QStringLiteral("swscale"), {}, LIBSWSCALE_IDENT, QStringLiteral("https://libav.org/"), "LGPL"},

    };
}

void glaxnimate::video::Module::initialize()
{
    register_io_classes<video::VideoFormat>();
}
