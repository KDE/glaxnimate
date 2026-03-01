/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "io_registry.hpp"

// Core formats
#include "io/glaxnimate/glaxnimate_format.hpp"
#include "io/glaxnimate/glaxnimate_mime.hpp"
#include "io/mime/json_mime.hpp"
#include "io/lottie/lottie_format.hpp"
#include "io/lottie/tgs_format.hpp"

// Raster
#include "io/raster/raster_format.hpp"
#include "io/raster/raster_mime.hpp"
#include "io/raster/spritesheet_format.hpp"

// External formats, mostly experimental
#include "io/aep/aep_format.hpp"
#include "io/rive/rive_format.hpp"

// SVG-like
#include "io/avd/avd_format.hpp"
#include "io/svg/svg_format.hpp"
#include "io/svg/svg_mime.hpp"

// Video
#ifdef VIDEO_ENABLED
#   include "io/video/video_format.hpp"
#endif

namespace {

template<class... Classes>
void register_classes()
{
    (glaxnimate::io::IoRegistry::instance().register_class<Classes>(), ...);
}

} // namespace


glaxnimate::io::glaxnimate::GlaxnimateFormat* default_format = nullptr;

glaxnimate::io::glaxnimate::GlaxnimateFormat* glaxnimate::io::glaxnimate::GlaxnimateFormat::instance()
{
    return default_format;
}

void glaxnimate::io::IoRegistry::load_formats()
{
    default_format = io::IoRegistry::instance().register_class<glaxnimate::GlaxnimateFormat>();

    register_classes<
        aep::AepFormat,
        aep::AepxFormat,
        avd::AvdFormat,
        glaxnimate::GlaxnimateMime,
        mime::JsonMime,
        lottie::LottieFormat,
        lottie::TgsFormat,
        raster::RasterFormat,
        raster::RasterMime,
        raster::SpritesheetFormat,
        rive::RiveFormat,
        svg::SvgFormat,
        svg::SvgMime
    >();

#ifdef VIDEO_ENABLED
    register_classes<
        video::VideoFormat
    >();
#endif
}
