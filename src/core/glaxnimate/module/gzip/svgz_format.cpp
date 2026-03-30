/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/module/gzip/svgz_format.hpp"
#include "glaxnimate/module/gzip/gzip.hpp"

bool glaxnimate::io::svg::SvgzFormat::on_open(QIODevice& file, const QString& filename, model::Document* document, const QVariantMap& options)
{
    if ( gzip::is_compressed(file) )
    {
        auto on_error = [this](const QString& s){warning(s);};
        gzip::GzipStream decompressed(&file, on_error);
        decompressed.open(QIODevice::ReadOnly);
        return SvgFormat::on_open(decompressed, filename, document, options);
    }
    return SvgFormat::on_open(file, filename, document, options);
}

bool glaxnimate::io::svg::SvgzFormat::on_save(QIODevice& file, const QString& filename, model::Composition* comp, const QVariantMap& options)
{
    auto on_error = [this](const QString& s){warning(s);};
    gzip::GzipStream compressed(&file, on_error);
    compressed.open(QIODevice::WriteOnly);
    return SvgFormat::on_save(compressed, filename, comp, options);
}

bool glaxnimate::io::svg::SvgzFormat::on_save_static(QIODevice &file, const QString &filename, model::Composition *comp, model::FrameTime time, const QVariantMap &options)
{
    auto on_error = [this](const QString& s){warning(s);};
    gzip::GzipStream compressed(&file, on_error);
    compressed.open(QIODevice::WriteOnly);
    return SvgFormat::on_save_static(compressed, filename, comp, time, options);
}


QStringList glaxnimate::io::svg::SvgzFormat::extensions(Direction) const
{
    return {"svgz"};
}
