/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cli.hpp"

#include <QImageWriter>

#include <KLocalizedString>

#include "plugin/script_engine.hpp"

#include "glaxnimate/app_info.hpp"
#include "glaxnimate/io/io_registry.hpp"
#include "glaxnimate/io/svg/svg_renderer.hpp"
#include "glaxnimate/io/raster/raster_mime.hpp"

#include "plugin/executor.hpp"
#include "plugin/plugin.hpp"

glaxnimate::cli::ParsedArguments glaxnimate::gui::parse_cli(const QStringList& args)
{
    glaxnimate::cli::Parser parser(AppInfo::instance().description());

    parser.add_group(i18nc("@info:shell", "Informational Options"));
    parser.add_argument({{"--help", "-h"}, i18nc("@info:shell", "Show this help and exit"), glaxnimate::cli::Argument::ShowHelp});
    parser.add_argument({{"--version", "-v"}, i18nc("@info:shell", "Show version information and exit"), glaxnimate::cli::Argument::ShowVersion});

    parser.add_group(i18nc("@info:shell", "Options"));
    parser.add_argument({{"file"}, i18nc("@info:shell", "File to open")});
    parser.add_argument({{"--trace"}, i18nc("@info:shell", "When opening image files, trace them instead of embedding")});

    parser.add_group(i18nc("@info:shell", "GUI Options"));
    parser.add_argument({{"--default-ui"}, i18nc("@info:shell", "If present, doesn't restore the main window state")});
    parser.add_argument({
        {"--ipc"},
        i18nc("@info:shell", "Specify the name of the local socket/named pipe to connect to a host application."),
        glaxnimate::cli::Argument::String,
        {},
        "IPC-NAME"
    });
    parser.add_argument({
        {"--window-size"},
        i18nc("@info:shell", "Use a specific size for the main window"),
        glaxnimate::cli::Argument::Size,
        {},
        "WIDTHxHEIGHT"
    });

    parser.add_argument({
        {"--window-id"},
        i18nc("@info:shell", "Print the window id"),
    });

    parser.add_argument({{"--debug"}, i18nc("@info:shell", "Enables the debug menu")});


    parser.add_group(i18nc("@info:shell", "Export Options"));
    parser.add_argument({
        {"--export", "-o"},
        i18nc("@info:shell", "Export the input file to the given file instead of starting the GUI"),
        glaxnimate::cli::Argument::String,
        {},
        "EXPORT-FILENAME"
    });
    parser.add_argument({
        {"--export-format", "-f"},
        i18nc("@info:shell", "Specify the format for --export. If omitted it's determined based on the file name. See --export-format-list for a list of supported formats."),
        glaxnimate::cli::Argument::String,
        {},
        "EXPORT-FORMAT"
    });
    parser.add_argument({
        {"--export-format-list"},
        i18nc("@info:shell", "Shows possible values for --export-format"),
        glaxnimate::cli::Argument::Flag
    });

    parser.add_group(i18nc("@info:shell", "Render Frame Options"));
    parser.add_argument({
        {"--render", "-r"},
        i18nc("@info:shell", "Render frames the input file to the given file instead of starting the GUI"),
        glaxnimate::cli::Argument::String,
        {},
        "RENDER-FILENAME"
    });
    parser.add_argument({
        {"--render-format"},
        i18nc("@info:shell", "Specify the format for --render. If omitted it's determined based on the file name. See --render-format-list for a list of supported formats."),
        glaxnimate::cli::Argument::String,
        {},
        "RENDER-FORMAT"
    });
    parser.add_argument({
        {"--frame"},
        i18nc("@info:shell", "Frame number to render, use `all` to render all frames"),
        glaxnimate::cli::Argument::String,
        {"0"},
        "FRAME"
    });
    parser.add_argument({
        {"--render-format-list"},
        i18nc("@info:shell", "Shows possible values for --render-format"),
        glaxnimate::cli::Argument::Flag
    });

    return parser.parse(args);
}


namespace  {

/// \todo A lot of code copied from the console, maybe could be moved to Executor
class CliPluginExecutor : public glaxnimate::plugin::Executor
{
public:
    CliPluginExecutor(glaxnimate::model::Document* document)
    {
        globals["document"] = QVariant::fromValue(document);
        globals["window"] = {};

        for ( const auto& engine : glaxnimate::plugin::ScriptEngineFactory::instance().engines() )
        {
            auto ctx = engine->create_context();

            if ( !ctx )
                continue;

            QObject::connect(ctx.get(), &glaxnimate::plugin::ScriptExecutionContext::stdout_line, [this](const QString& s){ console_stdout(s);});
            QObject::connect(ctx.get(), &glaxnimate::plugin::ScriptExecutionContext::stderr_line, [this](const QString& s){ console_stderr(s);});

            try {
                ctx->app_module("glaxnimate");
                ctx->app_module("glaxnimate_gui");
                for ( const auto& p : globals )
                    ctx->expose(p.first, p.second);
            } catch ( const glaxnimate::plugin::ScriptError& err ) {
                console_stderr(err.message());
            }

            script_contexts.push_back(std::move(ctx));
        }

        glaxnimate::plugin::PluginRegistry::instance().set_executor(this);
    }

    bool execute(const glaxnimate::plugin::Plugin& plugin, const glaxnimate::plugin::PluginScript& script, const QVariantList& args) override
    {
        for ( const auto& ctx : script_contexts )
        {
            if ( ctx->engine() == plugin.data().engine )
            {
                bool ok = false;
                try {
                    ok = ctx->run_from_module(plugin.data().dir, script.module, script.function, args);
                    if ( !ok )
                        console_stderr(i18nc("@info:shell", "Could not run the plugin"));
                } catch ( const glaxnimate::plugin::ScriptError& err ) {
                    console_stderr(err.message());
                    ok = false;
                }
                return ok;
            }
        }

        console_stderr(i18nc("@info:shell", "Could not find an interpreter"));
        return false;
    }

    QVariant get_global(const QString& name) override
    {
        auto it = globals.find(name);
        if ( it != globals.end() )
            return it->second;
        return {};
    }

    void console_stderr(const QString& line)
    {
        glaxnimate::cli::show_message(line, true);
    }

    void console_stdout(const QString& line)
    {
        glaxnimate::cli::show_message(line, false);
    }

private:
    std::vector<glaxnimate::plugin::ScriptContext> script_contexts;
    std::map<QString, QVariant> globals;
};


QVariantMap io_settings(std::unique_ptr<glaxnimate::settings::SettingsGroup> group)
{
    QVariantMap vals;
    if ( group )
    {
        for ( const auto& setting : *group )
            vals[setting.slug] = setting.default_value;
    }
    return vals;
}

void log_message(const QString& message, glaxnimate::log::Severity severity)
{
    glaxnimate::cli::show_message(
        QStringLiteral("%1: %2")
        .arg(glaxnimate::log::Logger::severity_name(severity))
        .arg(message)
    );
}


std::unique_ptr<glaxnimate::model::Document> cli_open(const glaxnimate::cli::ParsedArguments& args)
{
    using namespace glaxnimate;

    QString input_filename = args.value("file").toString();
    auto importer = io::IoRegistry::instance().from_filename(input_filename, io::ImportExport::Import);
    if ( !importer || !importer->can_open() )
    {
        glaxnimate::cli::show_message(i18nc("@info:shell", "Unknown importer"), true);
        return {};
    }

    QFile input_file(input_filename);
    if ( !input_file.open(QIODevice::ReadOnly) )
    {
        glaxnimate::cli::show_message(i18nc("@info:shell", "Could not open input file for reading"), true);
        return {};
    }

    auto document = std::make_unique<glaxnimate::model::Document>(input_filename);

    CliPluginExecutor script_executor(document.get());

    auto open_settings = io_settings(importer->open_settings());
    open_settings["trace"] = args.value("trace");

    QObject::connect(importer, &io::ImportExport::message, &log_message);
    if ( !importer->open(input_file, input_filename, document.get(), open_settings) )
    {
        glaxnimate::cli::show_message(i18nc("@info:shell", "Error loading input file"), true);
        return {};
    }
    input_file.close();

    return document;
}

bool cli_export(const glaxnimate::cli::ParsedArguments& args)
{
    using namespace glaxnimate;

    if ( !args.is_defined("file") )
    {
        glaxnimate::cli::show_message(i18nc("@info:shell", "You need to specify a file to export"), true);
        return false;
    }

    io::ImportExport* exporter = nullptr;
    QString format = args.value("export-format").toString();
    QString output_filename = args.value("export").toString();

    if ( !format.isEmpty() )
        exporter = io::IoRegistry::instance().from_slug(format);
    else
        exporter = io::IoRegistry::instance().from_filename(output_filename, io::ImportExport::Export);

    if ( !exporter || !exporter->can_save() )
    {
        glaxnimate::cli::show_message(i18nc("@info:shell", "Unknown exporter. Use --export-format-list for a list of available formats"), true);
        return false;
    }

    auto document = cli_open(args);
    if ( !document || document->assets()->compositions->values.empty() )
        return false;

    QFile output_file(output_filename);
    if ( !output_file.open(QIODevice::WriteOnly) )
    {
        glaxnimate::cli::show_message(i18nc("@info:shell", "Could not open output file for writing"), true);
        return false;
    }

    QObject::connect(exporter, &io::ImportExport::message, &log_message);
    /// \todo fix this (pass argument?)
    auto comp = document->assets()->compositions->values[0];
    if ( !exporter->save(output_file, output_filename, comp, io_settings(exporter->save_settings(comp))) )
    {
        glaxnimate::cli::show_message(i18nc("@info:shell", "Error converting to the output format"), true);
        return false;
    }

    return true;
}

using render_funcptr = void (*)(glaxnimate::model::Composition* comp, glaxnimate::model::FrameTime time, QFile& file, const char* format);

void render_frame(
    const QString& filename,
    const char* format,
    glaxnimate::model::Composition* comp,
    glaxnimate::model::FrameTime time,
    render_funcptr renderer
)
{
    QFile file(filename);
    if ( !file.open(QFile::WriteOnly) )
    {
        glaxnimate::cli::show_message(i18nc("@info:shell", "Could not save to %1", filename), true);
        return;
    }

    renderer(comp, time, file, format);
}

void render_frame_svg(glaxnimate::model::Composition* comp, glaxnimate::model::FrameTime time, QFile& file, const char*)
{
    using namespace glaxnimate;
    io::svg::SvgRenderer rend(io::svg::NotAnimated, io::svg::CssFontType::FontFace);
    rend.write_main(comp, time);
    rend.write(&file, true);
}

void render_frame_img(glaxnimate::model::Composition* comp, glaxnimate::model::FrameTime time, QFile& file, const char* format)
{
    QImage image = glaxnimate::io::raster::RasterMime::frame_to_image(comp, time);
    if ( !image.save(&file, format) )
        glaxnimate::cli::show_message(i18nc("@info:shell", "Could not save to %1", file.fileName()), true);
}

bool cli_render(const glaxnimate::cli::ParsedArguments& args)
{
    using namespace glaxnimate;

    if ( !args.is_defined("file") )
    {
        glaxnimate::cli::show_message(i18nc("@info:shell", "You need to specify a file to render"), true);
        return false;
    }

    QString format = args.value("render-format").toString();
    QString output_filename = args.value("render").toString();
    QFileInfo finfo(output_filename);

    if ( !format.isEmpty() )
    {
        format = finfo.suffix();
        if ( !format.contains("svg") )
            format.clear();
    }

    std::string stdfmt = format.toUpper().toStdString();
    const char* cfmt = stdfmt.empty() ? nullptr : stdfmt.c_str();

    render_funcptr renderer = nullptr;

    if ( format == "svg" )
        renderer = &render_frame_svg;
    else
        renderer = &render_frame_img;

    auto document = cli_open(args);
    if ( !document )
        return false;

    auto dir = finfo.dir();
    if ( !dir.exists() )
    {
        auto name = dir.dirName();
        dir.cdUp();
        dir.mkpath(name);
        dir.cd(name);
    }

    /// \todo fix this (pass argument?)
    auto comp = document->assets()->compositions->values[0];

    QString frame = args.value("frame").toString();
    if ( frame == "all" || frame == "-" || frame == "*" )
    {

        float ip = comp->animation->first_frame.get();
        float op = comp->animation->last_frame.get();
        float pad = op != 0 ? std::ceil(std::log(op) / std::log(10)) : 1;

        for ( int f = ip; f < op; f += 1 )
        {
            QString frame_name = QString::number(f).rightJustified(pad, '0');
            QString file_name = dir.filePath(finfo.baseName() + frame_name + "." + finfo.completeSuffix());
            render_frame(file_name, cfmt, comp, f, renderer);
        }
    }
    else
    {
        render_frame(output_filename, cfmt, comp, frame.toDouble(), renderer);
    }

    return true;
}

} // namespace

void glaxnimate::gui::cli_main(gui::GlaxnimateApp& app, glaxnimate::cli::ParsedArguments& args)
{
    log::Logger::instance().add_listener<log::ListenerStderr>();

    if ( args.has_flag("export-format-list") )
    {
        app.initialize();
        int max_name_len = 0;
        std::vector<std::pair<QString, QString>> table;
        for ( const auto& exporter : io::IoRegistry::instance().exporters() )
        {
            table.emplace_back(exporter->slug(), exporter->name());
            max_name_len = qMax<int>(table.back().first.size(), max_name_len);
        }
        for ( const auto& entry : table )
            glaxnimate::cli::show_message(entry.first + QString(max_name_len - entry.first.size(), ' ') + " : " + entry.second, false);
        args.return_value = 0;
        return;
    }


    if ( args.has_flag("render-format-list") )
    {
        glaxnimate::cli::show_message("svg");
        for ( const auto& fmt : QImageWriter::supportedImageFormats() )
            glaxnimate::cli::show_message(QString::fromUtf8(fmt));
        args.return_value = 0;
        return;
    }

    if ( args.is_defined("export") )
    {
        app.initialize();
        if ( !cli_export(args) )
            args.return_value = 1;
        else
            args.return_value = 0;
    }

    if ( args.is_defined("render") )
    {
        app.initialize();
        if ( !cli_render(args) )
            args.return_value = 1;
        else
            args.return_value = 0;
    }
}
