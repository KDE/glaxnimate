/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <KCompressionDevice>
#include <QBuffer>

#include "glaxnimate/utils/i18n.hpp"

#include "gzip.hpp"

using namespace glaxnimate;


bool gzip::decompress(QIODevice& input, QByteArray& output, const gzip::ErrorFunc& on_error)
{
    KCompressionDevice compressed(&input, false, KCompressionDevice::GZip);
    compressed.open(QIODevice::ReadOnly);
    output = compressed.readAll();

    if ( compressed.error() )
    {
        on_error(i18n("Could not decompress data"));
        return false;
    }

    return true;
}

bool gzip::decompress(const QByteArray& input, QByteArray& output, const gzip::ErrorFunc& on_error)
{
    QBuffer buf(const_cast<QByteArray*>(&input));
    return decompress(buf, output, on_error);
}


bool gzip::compress(const QByteArray& data, QIODevice& output,
    const gzip::ErrorFunc& on_error, int,
    quint32* compressed_size)
{

    KCompressionDevice compressed(&output, false, KCompressionDevice::GZip);
    compressed.open(QIODevice::WriteOnly);
    compressed.write(data);

    if ( compressed.error() )
    {
        on_error(compressed.errorString());
        return false;
    }

    compressed.close();

    if ( compressed_size )
        *compressed_size = output.pos();

    return true;
}


class gzip::GzipStream::Private
{
public:
    Private(QIODevice* target, const ErrorFunc&)
        : device(target, false, KCompressionDevice::GZip)
    {}

    KCompressionDevice device;
};


gzip::GzipStream::GzipStream(QIODevice* target, const gzip::ErrorFunc& on_error)
    : d(std::make_unique<Private>(target, on_error))
{}

gzip::GzipStream::~GzipStream() {}

bool gzip::GzipStream::open(QIODevice::OpenMode mode)
{
    if ( d->device.openMode() != NotOpen )
    {
        setErrorString(i18n("Gzip stream already open"));
        return false;
    }

    return d->device.open(mode);
}

bool gzip::GzipStream::atEnd() const
{
    return d->device.atEnd();
}

qint64 gzip::GzipStream::writeData(const char* data, qint64 len)
{
    return d->device.write(data, len);
}

qint64 gzip::GzipStream::readData(char* data, qint64 maxlen)
{
    return d->device.read(data, maxlen);
}

