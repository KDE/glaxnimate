/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tgs_format.hpp"

#include "glaxnimate/io/lottie/cbor_write_json.hpp"
#include "glaxnimate/model/shapes/shapes/polystar.hpp"
#include "glaxnimate/model/shapes/composable/image.hpp"
#include "glaxnimate/model/shapes/style/stroke.hpp"
#include "glaxnimate/model/shapes/modifiers/repeater.hpp"
#include "glaxnimate/model/shapes/modifiers/inflate_deflate.hpp"
#include "glaxnimate/model/shapes/modifiers/offset_path.hpp"
#include "glaxnimate/model/shapes/modifiers/zig_zag.hpp"
#include "glaxnimate/io/lottie/validation.hpp"
#include "glaxnimate/module/gzip/gzip.hpp"


using namespace glaxnimate;
using namespace glaxnimate::io::lottie;

namespace {

class TgsVisitor : public glaxnimate::io::lottie::ValidationVisitor
{

public:
    explicit TgsVisitor(LottieFormat* fmt)
        : ValidationVisitor(fmt)
    {
        allowed_fps.push_back(30);
        allowed_fps.push_back(60);
        fixed_size = QSize(512, 512);
        max_frames = 180;
    }

private:
    void on_visit(model::DocumentNode * node) override
    {
        if ( qobject_cast<model::PolyStar*>(node) )
        {
            show_error(node, i18n("Star Shapes are not officially supported"), log::Info);
        }
        else if ( qobject_cast<model::Image*>(node) || qobject_cast<model::Bitmap*>(node) )
        {
            show_error(node, i18n("Images are not supported"), log::Error);
        }
        else if ( auto st = qobject_cast<model::Stroke*>(node) )
        {
            if ( qobject_cast<model::Gradient*>(st->use.get()) )
                show_error(node, i18n("Gradient strokes are not officially supported"), log::Info);
        }
        else if ( auto layer = qobject_cast<model::Layer*>(node) )
        {
            if ( layer->mask->has_mask() )
                show_error(node, i18n("Masks are not officially supported"), log::Error);
        }
        else if ( qobject_cast<model::Repeater*>(node) )
        {
            show_error(node, i18n("Repeaters are not officially supported"), log::Info);
        }
        else if ( qobject_cast<model::InflateDeflate*>(node) )
        {
            show_error(node, i18n("Inflate/Deflate is not supported"), log::Warning);
        }
        else if ( qobject_cast<model::OffsetPath*>(node) )
        {
            show_error(node, i18n("Offset Path is not supported"), log::Warning);
        }
        else if ( qobject_cast<model::ZigZag*>(node) )
        {
            show_error(node, i18n("ZigZag is not supported"), log::Warning);
        }
    }
};

} // namespace

bool glaxnimate::io::lottie::TgsFormat::on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap&)
{
    QByteArray json;
    if ( !gzip::decompress(file, json, [this](const QString& s){ error(s); }) )
        return false;
    return load_json(json, document);
}

bool glaxnimate::io::lottie::TgsFormat::on_save(QIODevice& file, const QString&, model::Composition* comp, const QVariantMap&)
{
    QVariantMap settings;
    settings[QStringLiteral("duplicate_masks")] = true;

    validate(comp->document(), comp);

    QCborMap json = LottieFormat::to_json(comp, true, true, settings);
    json[QLatin1String("tgs")] = 1;
    QByteArray data = cbor_write_json(json, true);

    quint32 compressed_size = 0;
    if ( !gzip::compress(data, file, [this](const QString& s){ error(s); }, 9, &compressed_size) )
        return false;

    qreal size_k = compressed_size / 1024.0;
    if ( size_k > 64 )
        error(i18n("File too large: %1k, should be under 64k", size_k));

    return true;
}


void glaxnimate::io::lottie::TgsFormat::validate(model::Document* document, model::Composition* comp)
{
    TgsVisitor(this).visit(document, comp);
}
