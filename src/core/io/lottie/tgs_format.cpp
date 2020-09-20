#include "tgs_format.hpp"
#include "cbor_write_json.hpp"
#include "utils/gzip.hpp"


bool io::lottie::TgsFormat::on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap&)
{
    QByteArray json;
    if ( !utils::gzip::decompress(file, json, [this](const QString& s){ error(s); }) )
        return false;
    return load_json(json, document);
}

bool io::lottie::TgsFormat::on_save(QIODevice& file, const QString&, model::Document* document, const QVariantMap&)
{
    validate(document);

    QCborMap json = LottieFormat::to_json(document, true);
    json[QLatin1String("tgs")] = 1;
    QByteArray data = cbor_write_json(json, true);

    quint32 compressed_size = 0;
    if ( !utils::gzip::compress(data, file, [this](const QString& s){ error(s); }, 9, &compressed_size) )
        return false;

    qreal size_k = compressed_size / 1024.0;
    if ( size_k > 64 )
        error(tr("File too large: %1k, should be under 64k").arg(size_k));

    return true;
}


void io::lottie::TgsFormat::validate(model::Document* document)
{
    qreal width = document->main_composition()->height.get();
    if ( width != 512 )
        error(tr("Invalid width: %1, should be 512").arg(width));

    qreal height = document->main_composition()->height.get();
    if ( height != 512 )
        error(tr("Invalid height: %1, should be 512").arg(height));

    qreal fps = document->main_composition()->fps.get();
    if ( fps != 30 && fps != 60 )
        error(tr("Invalid fps: %1, should be 30 or 60").arg(fps));
}


io::Autoreg<io::lottie::TgsFormat> io::lottie::TgsFormat::autoreg = {};
