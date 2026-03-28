/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/io/raster/raster_format.hpp"

#include <QFileInfo>


#include "glaxnimate/io/io_registry.hpp"
#include "glaxnimate/model/shapes/composable/image.hpp"
#include "glaxnimate/model/assets/assets.hpp"

QStringList glaxnimate::io::raster::RasterFormat::extensions(Direction direction) const
{
    QStringList formats;
    for ( const auto& fmt : QImageReader::supportedImageFormats() )
        if ( (direction == FrameExport || (fmt != "gif" && fmt != "webp")) && fmt != "svg" )
            formats << QString::fromUtf8(fmt);
    return formats;
}

bool glaxnimate::io::raster::RasterFormat::on_open(QIODevice& dev, const QString& filename, model::Document* document, const QVariantMap& settings)
{
    auto main = document->assets()->add_comp_no_undo();

    main->animation->last_frame.set(main->fps.get());


    model::FrameTime default_time = settings["default_time"].toFloat();
    main->animation->last_frame.set(default_time == 0 ? default_time : 180);

    auto bmp = document->assets()->images->values.insert(std::make_unique<model::Bitmap>(document));
    if ( auto file = qobject_cast<QFile*>(&dev) )
        bmp->filename.set(file->fileName());
    else
        bmp->data.set(dev.readAll());
    auto img = std::make_unique<model::Image>(document);
    img->image.set(bmp);
    QPointF p(bmp->pixmap().width() / 2.0, bmp->pixmap().height() / 2.0);
    if ( !filename.isEmpty() )
        img->name.set(QFileInfo(filename).baseName());
    img->transform->anchor_point.set(p);
    img->transform->position.set(p);
    main->shapes.insert(std::move(img));
    main->width.set(bmp->pixmap().width());
    main->height.set(bmp->pixmap().height());
    return !bmp->pixmap().isNull();
}

bool glaxnimate::io::raster::RasterFormat::on_save_static(QIODevice &file, const QString &filename, model::Composition *comp, model::FrameTime time, const QVariantMap &)
{
    QString format = QFileInfo(filename).suffix();
    if ( format.isEmpty() )
        format = QStringLiteral("png");
    return  comp->render_image(time).save(&file, format.toStdString().c_str());
}
