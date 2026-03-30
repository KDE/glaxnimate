/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <functional>
#include <memory>

#include <QIODevice>
#include <QByteArray>

namespace glaxnimate::gzip {

using ErrorFunc = std::function<void (const QString&)>;

bool compress(const QByteArray& data, QIODevice& output, const gzip::ErrorFunc& on_error, int level, quint32* compressed_size);

bool decompress(QIODevice& input, QByteArray& output, const ErrorFunc& on_error);
bool decompress(const QByteArray& input, QByteArray& output, const ErrorFunc& on_error);

bool is_compressed(QIODevice& input);
bool is_compressed(const QByteArray& input);


class GzipStream : public QIODevice
{
public:
    GzipStream(QIODevice* target, const ErrorFunc& on_error);
    ~GzipStream();

    bool atEnd() const override;
    bool isSequential() const override { return true; }
    bool open(QIODevice::OpenMode mode) override;

    qint64 ouput_size() const;

protected:
    qint64 readData(char * data, qint64 maxlen) override;
    qint64 writeData(const char * data, qint64 len) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};


} // namespace glaxnimate::gzip
