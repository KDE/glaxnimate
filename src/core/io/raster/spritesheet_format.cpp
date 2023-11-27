/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "spritesheet_format.hpp"
#include "model/assets/composition.hpp"

#include <QImage>
#include <QPainter>
#include <QImageWriter>

glaxnimate::io::Autoreg<glaxnimate::io::raster::SpritesheetFormat> glaxnimate::io::raster::SpritesheetFormat::autoreg;

QStringList glaxnimate::io::raster::SpritesheetFormat::extensions() const
{
    QStringList formats;
    formats << "png"_qs;
    for ( const auto& fmt : QImageWriter::supportedImageFormats() )
        if ( fmt != "jpg" && fmt != "svg" )
            formats << QString::fromUtf8(fmt);
    return formats;
}

std::unique_ptr<app::settings::SettingsGroup> glaxnimate::io::raster::SpritesheetFormat::save_settings(model::Composition* comp) const
{
    int first_frame = comp->animation->first_frame.get();
    int last_frame = comp->animation->last_frame.get();
    int frames = last_frame - first_frame;
    return std::make_unique<app::settings::SettingsGroup>(app::settings::SettingList{
        app::settings::Setting("frame_width"_qs, tr("Frame Width"), tr("Width of each frame"), comp->width.get(), 1, 999'999),
        app::settings::Setting("frame_height"_qs, tr("Frame Height"), tr("Height of each frame"), comp->height.get(), 1, 999'999),
        app::settings::Setting("columns"_qs, tr("Columns"), tr("Number of columns in the sheet"), std::ceil(math::sqrt(frames)), 1, 64),
        app::settings::Setting("frame_step"_qs, tr("Time Step"), tr("By how much each rendered frame should increase time (in frames)"), 1, 1, 16),
    });
}

bool glaxnimate::io::raster::SpritesheetFormat::on_save(QIODevice& file, const QString& filename, model::Composition* comp, const QVariantMap& setting_values)
{
    Q_UNUSED(filename);

    int frame_w = setting_values["frame_width"_qs].toInt();
    int frame_h = setting_values["frame_height"_qs].toInt();
    int columns = setting_values["columns"_qs].toInt();
    int frame_step = setting_values["frame_step"_qs].toInt();

    if ( frame_w <= 0 || frame_h <= 0 || columns <= 0 || frame_step <= 0 )
        return false;

    int first_frame = comp->animation->first_frame.get();
    int last_frame = comp->animation->last_frame.get();
    int frames = (last_frame - first_frame) / frame_step;
    int rows = frames / columns;

    qreal scale_x = qreal(frame_w) / comp->width.get();
    qreal scale_y = qreal(frame_h) / comp->height.get();

    QImage bmp(frame_w * columns, frame_h * rows, QImage::Format_ARGB32);
    QPainter painter(&bmp);
    for ( int i = first_frame; i <= last_frame; i += frame_step )
    {
        painter.save();
        painter.scale(scale_x, scale_y);
        painter.translate((i % columns) * frame_w, (i / columns) * frame_h);
        painter.setClipRect(0, 0, frame_w, frame_h);
        comp->paint(&painter, i, model::VisualNode::Render);
        painter.restore();
    }

    painter.end();


    QImageWriter writer(&file, {});
    writer.setOptimizedWrite(true);
    if ( writer.write(bmp) )
        return true;

    error(writer.errorString());
    return false;
}

