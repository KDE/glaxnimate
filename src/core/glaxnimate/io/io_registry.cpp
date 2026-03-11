/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "glaxnimate/io/io_registry.hpp"

// Core formats
#include "glaxnimate/io/glaxnimate/glaxnimate_format.hpp"
#include "glaxnimate/io/glaxnimate/glaxnimate_mime.hpp"
#include "glaxnimate/io/mime/json_mime.hpp"
#include "glaxnimate/io/lottie/lottie_format.hpp"

#ifdef GLAXNIMATE_CORE_KDE
#   include "glaxnimate/io/lottie/tgs_format.hpp"
#endif

// Raster
#include "glaxnimate/io/raster/raster_format.hpp"
#include "glaxnimate/io/raster/raster_mime.hpp"
#include "glaxnimate/io/raster/spritesheet_format.hpp"

// External formats, mostly experimental
#include "glaxnimate/io/aep/aep_format.hpp"
#include "glaxnimate/io/rive/rive_format.hpp"

// SVG-like
#include "glaxnimate/io/avd/avd_format.hpp"
#include "glaxnimate/io/svg/svg_format.hpp"
#include "glaxnimate/io/svg/svg_mime.hpp"

// Video
#ifdef GLAXNIMATE_VIDEO_ENABLED
#   include "glaxnimate/io/video/video_format.hpp"
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
        rive::RiveFormat,
        svg::SvgFormat,
        svg::SvgMime
    >();

#ifdef GLAXNIMATE_CORE_KDE
    register_classes<
        lottie::TgsFormat
    >();
#endif

#ifdef GLAXNIMATE_VIDEO_ENABLED
    register_classes<
        video::VideoFormat
    >();
#endif

    // Raster after video for precedence
    register_classes<
        raster::RasterFormat,
        raster::RasterMime,
        raster::SpritesheetFormat
    >();
}
