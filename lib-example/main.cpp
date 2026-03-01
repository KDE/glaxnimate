/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <iostream>
#include "io/io_registry.hpp"
#include "renderer/renderer.hpp"
#include "model/assets/assets.hpp"


using namespace glaxnimate;

int main(int argc, char *argv[])
{
    // Parse args
    if ( argc != 4 )
    {
        std::clog << "Usage: " << argv[0] << " input_file frame output_image\n";
        return 1;
    }

    QString in_filename = argv[1];
    float frame = std::stof(argv[2]);
    QString out_filename = argv[3];

    // Load file
    io::IoRegistry::load_formats();
    auto importer = io::IoRegistry::instance().from_filename(in_filename, io::ImportExport::Import);
    if ( !importer )
    {
        std::clog << "No suitable importer for " << in_filename.toStdString() << "\n";
        return 2;
    }

    QFile in_file(in_filename);
    if ( !in_file.open(QIODevice::ReadOnly) )
    {
        std::clog << "Could not read from " << in_filename.toStdString() << "\n";
        return 2;
    }

    model::Document document(in_filename);
    if ( !importer->load(&document, in_file.readAll(), {}, in_filename) )
    {
        std::clog << "Could load " << in_filename.toStdString() << "\n";
        return 2;
    }

    auto comps = document.assets()->compositions.get();
    if ( comps->values.empty() )
    {
        std::clog << "No compositions in " << in_filename.toStdString() << "\n";
        return 2;

    }

    // Render
    auto comp = comps->values[0];
    QImage bmp(comp->width.get(), comp->height.get(), QImage::Format_ARGB32);
    bmp.fill(Qt::transparent);

    auto renderer = renderer::default_renderer(10);
    renderer->set_image_surface(&bmp);
    renderer->render_start();
    comp->paint(renderer.get(), frame, model::VisualNode::Render);
    renderer->render_end();

    bmp.save(out_filename);

    return 0;
}
