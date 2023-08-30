/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "bitmap.hpp"
#include <QPainter>
#include <QImageWriter>
#include <QImageReader>
#include <QFileInfo>
#include <QBuffer>
#include <QUrl>

#include "model/document.hpp"
#include "model/assets/assets.hpp"
#include "command/object_list_commands.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::Bitmap)

void glaxnimate::model::Bitmap::paint(QPainter* painter) const
{
    painter->drawPixmap(0, 0, image);
}

void glaxnimate::model::Bitmap::refresh(bool rebuild_embedded)
{
    QImageReader reader;
    QImage qimage;

    bool load_data = true;

    if ( rebuild_embedded || data.get().isEmpty() )
    {
        if ( !filename.get().isEmpty() )
        {
            QFileInfo finfo = file_info();
            if ( !finfo.isFile() )
                return;
            reader.setFileName(finfo.absoluteFilePath());
            format.set(reader.format());
            qimage = reader.read();
            if ( rebuild_embedded && embedded() )
                data.set(build_embedded(qimage));
            load_data = false;
        }
        else if ( !url.get().isEmpty() )
        {
            document()->assets()->network_downloader.get(url.get(), [this, rebuild_embedded](QByteArray response){
                QImageReader reader;
                QImage qimage;
                QBuffer buf(&response);
                buf.open(QIODevice::ReadOnly);
                reader.setDevice(&buf);
                format.set(reader.format());
                qimage = reader.read();
                if ( rebuild_embedded && embedded() )
                    data.set(build_embedded(qimage));

                image = QPixmap::fromImage(qimage);
                width.set(image.width());
                height.set(image.height());

                document()->graphics_invalidated();
                emit loaded();
            }, this);
            return;
        }
    }

    if ( load_data )
    {
        QBuffer buf(const_cast<QByteArray*>(&data.get()));
        buf.open(QIODevice::ReadOnly);
        reader.setDevice(&buf);
        format.set(reader.format());
        qimage = reader.read();
    }

    image = QPixmap::fromImage(qimage);
    width.set(image.width());
    height.set(image.height());

    emit loaded();
}

QByteArray glaxnimate::model::Bitmap::build_embedded(const QImage& img) const
{
    QByteArray new_data;
    QBuffer buf(&new_data);
    buf.open(QIODevice::WriteOnly);
    QImageWriter writer(&buf, format.get().toLatin1());
    writer.write(img);
    return new_data;
}

bool glaxnimate::model::Bitmap::embedded() const
{
    return !data.get().isEmpty();
}

void glaxnimate::model::Bitmap::embed(bool embedded)
{
    if ( embedded == this->embedded() )
        return;

    if ( !embedded )
        data.set_undoable({});
    else
        data.set_undoable(build_embedded(image.toImage()));
}

void glaxnimate::model::Bitmap::on_refresh()
{
    refresh(false);
}

QIcon glaxnimate::model::Bitmap::instance_icon() const
{
    return image;
}

bool glaxnimate::model::Bitmap::from_url(const QUrl& url)
{
    if ( url.scheme().isEmpty() || url.scheme() == "file" )
        return from_file(url.path());

    if ( url.scheme() == "data" )
        return from_base64(url.path());

    this->url.set(url.toString());

    return true;
}

bool glaxnimate::model::Bitmap::from_file(const QString& file)
{
    filename.set(file);
    return !image.isNull();
}

bool glaxnimate::model::Bitmap::from_base64(const QString& data)
{
    auto chunks = data.split(',');
    if ( chunks.size() != 2 )
        return false;
    auto mime_settings = chunks[0].split(';');
    if ( mime_settings.size() != 2 || mime_settings[1] != "base64" )
        return false;

    auto formats = QImageReader::imageFormatsForMimeType(mime_settings[0].toLatin1());
    if ( formats.empty() )
        return false;

    auto decoded = QByteArray::fromBase64(chunks[1].toLatin1());
    format.set(formats[0]);
    this->data.set(decoded);
    return !image.isNull();
}


bool glaxnimate::model::Bitmap::from_raw_data(const QByteArray& data)
{
    QBuffer buf(const_cast<QByteArray*>(&data));
    buf.open(QBuffer::ReadOnly);
    auto format = QImageReader::imageFormat(&buf);
    if ( format.isEmpty() )
        return false;

    this->format.set(format);
    this->data.set(data);
    return !image.isNull();

}

QUrl glaxnimate::model::Bitmap::to_url() const
{
    if ( !embedded() )
    {
        return QUrl::fromLocalFile(file_info().absoluteFilePath());
    }

    QByteArray fmt = format.get().toLatin1();
    QByteArray mime_type;
    for ( const auto& mime : QImageWriter::supportedMimeTypes() )
        if ( QImageWriter::imageFormatsForMimeType(mime).contains(fmt) )
        {
            mime_type = mime;
            break;
        }

    if ( mime_type.isEmpty() )
        return {};

    QString data_url = "data:";
    data_url += mime_type;
    data_url += ";base64,";
    data_url += data.get().toBase64();
    return QUrl(data_url);
}

QString glaxnimate::model::Bitmap::object_name() const
{
    if ( embedded() )
        return tr("Embedded image");
    return QFileInfo(filename.get()).fileName();
}


QFileInfo glaxnimate::model::Bitmap::file_info() const
{
    return QFileInfo(document()->io_options().path, filename.get());
}


bool glaxnimate::model::Bitmap::remove_if_unused(bool)
{
    if ( users().empty() )
    {
        document()->push_command(new command::RemoveObject(
            this,
            &document()->assets()->images->values
        ));
        return true;
    }
    return false;
}

void glaxnimate::model::Bitmap::set_pixmap(const QImage& pix, const QString& format)
{
    this->format.set(format);
    data.set(build_embedded(pix));
}

QByteArray glaxnimate::model::Bitmap::image_data() const
{
    if ( !data.get().isEmpty() )
        return data.get();

    if ( image.isNull() )
        return {};

    return build_embedded(image.toImage());
}

QSize glaxnimate::model::Bitmap::size() const
{
    return {width.get(), height.get()};
}
