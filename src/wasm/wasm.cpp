#include <emscripten/bind.h>

#include "glaxnimate/io/io_registry.hpp"
#include "glaxnimate/renderer/renderer.hpp"
#include "glaxnimate/model/assets/assets.hpp"

#include <emscripten/emscripten.h>
#include <QPainter>
#include <thorvg.h>

using namespace glaxnimate;

class GlaxnimateRenderer
{
public:
    GlaxnimateRenderer(const emscripten::val& options)
    {
        static bool initialized = false;
        if ( !initialized )
        {
            io::IoRegistry::load_formats();
            //tvg::Initializer::init(1);
            initialized = true;
        }

        if ( options["data"].isUndefined() )
            return;

        QString filename = options["filename"].isUndefined() ? "data" : QString::fromStdString(options["filename"].as<std::string>());
        QByteArray data = QByteArray::fromStdString(options["data"].as<std::string>());
        io::ImportExport* importer = nullptr;

        if ( !options["slug"].isUndefined() )
        {
            importer = io::IoRegistry::instance().from_slug(QString::fromStdString(options["slug"].as<std::string>()));
        }

        if ( !importer || !importer->can_handle(glaxnimate::io::ImportExport::Import) )
            return;

        document = std::make_unique<model::Document>(filename);
        if ( !importer->load(document.get(), data, {}, filename) )
        {
            document.reset();
            return;
        }

        auto comps = document->assets()->compositions.get();
        if ( comps->values.empty() )
        {
            document.reset();
            return;
        }

        comp = comps->values[0];
    }

    model::FrameTime current_time() const
    {
        return document->current_time();
    }

    void set_current_time(model::FrameTime t)
    {
        document->set_current_time(t);
    }

    void on_render()
    {
        if ( !comp )
            return;
        frame = comp->render_image(current_time()).convertToFormat(QImage::Format_RGBA8888);
        /*frame = QImage(width(), height(), QImage::Format_ARGB32);
        frame.fill(Qt::blue);

        auto renderer = renderer::default_renderer(10);
        renderer->set_image_surface(&frame);
        renderer->render_start();
        comp->paint(renderer.get(), current_time(), model::VisualNode::Render);
        // renderer->fill_rect(QRectF(0, 0, 100, 100), Qt::red);
        renderer->render_end();

        frame = frame.convertToFormat(QImage::Format_RGBA8888);*/
    }

    emscripten::val render()
    {
        on_render();
        const uchar* bits = frame.constBits();
        return emscripten::val(
            emscripten::typed_memory_view(frame.sizeInBytes(), bits)
        );
    }

    bool loaded() const
    {
        return comp;
    }

    float fps() const
    {
        return !comp ? 0 : comp->fps.get();
    }

    float width() const
    {
        return !comp ? 0 : comp->width.get();
    }

    float height() const
    {
        return !comp ? 0 : comp->height.get();
    }

    float last_frame() const
    {
        return !comp ? 0 : comp->animation->last_frame.get();
    }

private:
    QImage frame;
    std::unique_ptr<model::Document> document;
    model::Composition* comp = nullptr;

};

EMSCRIPTEN_BINDINGS(my_module) {
    emscripten::class_<GlaxnimateRenderer>("GlaxnimateRenderer")
        .constructor<emscripten::val>()
        .property("current_time", &GlaxnimateRenderer::current_time, &GlaxnimateRenderer::set_current_time)
        .property("loaded", &GlaxnimateRenderer::loaded)
        .property("fps", &GlaxnimateRenderer::fps)
        .property("width", &GlaxnimateRenderer::width)
        .property("height", &GlaxnimateRenderer::height)
        .property("last_frame", &GlaxnimateRenderer::last_frame)
        .function("render", &GlaxnimateRenderer::render)
    ;
}
