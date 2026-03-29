/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/module/module.hpp"

#include "glaxnimate/math/bezier/meta.hpp"
#include "glaxnimate/app_info.hpp"
#include "glaxnimate/renderer/thorvg_renderer.hpp"

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

using namespace glaxnimate;

glaxnimate::io::glaxnimate::GlaxnimateFormat* default_format = nullptr;

glaxnimate::io::glaxnimate::GlaxnimateFormat* glaxnimate::io::glaxnimate::GlaxnimateFormat::instance()
{
    return default_format;
}

namespace {

class CoreModule : public glaxnimate::module::Module
{
public:
    CoreModule() : Module(i18n("Glaxnimate Core"), AppInfo::instance().version()) {}

    std::vector<glaxnimate::module::ExternalComponent> components() const override
    {
        return {};
    }

protected:
    void initialize() override
    {
        math::bezier::register_meta();
        init_file_formats();
    }

    void init_file_formats()
    {
        using namespace glaxnimate::io;
        default_format = io::IoRegistry::instance().register_class<io::glaxnimate::GlaxnimateFormat>();

        register_io_classes<
            aep::AepFormat,
            aep::AepxFormat,
            avd::AvdFormat,
            io::glaxnimate::GlaxnimateMime,
            mime::JsonMime,
            lottie::LottieFormat,
            rive::RiveFormat,
            svg::SvgFormat,
            svg::SvgMime
        >();

#ifdef GLAXNIMATE_CORE_KDE
        register_io_classes<
            lottie::TgsFormat
        >();
#endif

        // Raster after video for precedence
        register_io_classes<
            raster::RasterFormat,
            raster::RasterMime,
            raster::SpritesheetFormat
        >();

        renderer::Renderer::register_factory(QStringLiteral("ThorVG"), [](int q){ return std::make_unique<renderer::ThorvgRenderer>(q); });
    }
};

} // namespace



glaxnimate::module::Registry::Registry()
{
    install<CoreModule>();
}

module::Module::Module(const QString &name, const QString &version)
    : name_(name), version_(version.isEmpty() ? AppInfo::instance().version() : version)
{}

module::Registry &module::registry()
{
    return Registry::instance();
}

void module::initialize()
{
    Registry::instance();
}
