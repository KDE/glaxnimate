/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ps_filters.hpp"
#include <QImageReader>

using namespace glaxnimate::ps;


glaxnimate::ps::FilteredFile::FilteredFile(std::shared_ptr<FileInterface> inner)
    : inner(std::move(inner))
{

}

bool glaxnimate::ps::FilteredFile::readable() const
{
    return inner->readable();
}

bool glaxnimate::ps::FilteredFile::writable() const
{
    return inner->writable();
}

bool glaxnimate::ps::FilteredFile::is_open() const
{
    return inner->is_open();
}

bool glaxnimate::ps::FilteredFile::eof() const
{
    return (end_reached && read_buffer.isEmpty()) || inner->eof();
}

bool glaxnimate::ps::FilteredFile::is_filtered() const
{
    return true;
}

bool glaxnimate::ps::FilteredFile::is_seekable() const
{
    return false;
}

bool glaxnimate::ps::FilteredFile::seek(int)
{
    return false;
}

int glaxnimate::ps::FilteredFile::tell() const
{
    return -1;
}

void glaxnimate::ps::FilteredFile::unget_char(char c)
{
    read_buffer.push_back(c);
}

void glaxnimate::ps::FilteredFile::flush()
{
    write_buffer.clear();
    read_buffer.clear();
}

void glaxnimate::ps::FilteredFile::reset()
{
    read_buffer.clear();
    write_buffer.clear();
}

void glaxnimate::ps::FilteredFile::close()
{
    flush();
    on_close();
    inner->close();
}

void SimpleFileFilter::flush()
{
    if ( !write_buffer.isEmpty() )
    {
        filter_write(write_buffer);
    }

    FilteredFile::flush();
}

std::optional<char> glaxnimate::ps::SimpleFileFilter::get_char()
{
    if ( read_buffer.isEmpty() )
    {
        if ( end_reached )
            return {};

        QByteArray raw;
        raw.reserve(input_size);

        while ( raw.isEmpty() || needs_more(raw) )
        {
            auto byte = inner->get_char();
            if ( !byte )
            {
                end_reached = true;
                break;
            }

            if ( skip(*byte) )
                continue;

            if ( marks_end(*byte) )
            {
                end_reached = true;
                break;
            }

            raw.push_back(*byte);
        }

        read_buffer = convert(raw);
        if ( read_buffer.isEmpty() )
            return {};

        std::reverse(read_buffer.begin(), read_buffer.end());
    }

    char ch = read_buffer.back();
    read_buffer.erase(read_buffer.end() - 1);
    return ch;
}

void glaxnimate::ps::SimpleFileFilter::put_char(char c)
{
    write_buffer.push_back(c);
    if ( !needs_more(write_buffer) )
    {
        filter_write(write_buffer);
        write_buffer.clear();
    }
}

std::vector<uintptr_t> SimpleFileFilter::comparator() const
{
    return {std::uintptr_t(inner.get()), std::uintptr_t(convert_func)};
}

QByteArray glaxnimate::ps::FilteredFile::read(int count)
{
    QByteArray out;
    out.reserve(count);
    for ( int i = 0; i < count; i++ )
    {
        if ( auto ch = get_char() )
            out.push_back(*ch);
        else
            break;
    }
    return out;
}

void glaxnimate::ps::FilteredFile::write(const QByteArray &data)
{
    for ( char ch : data )
        put_char(ch);
}

QIODevice *glaxnimate::ps::FilteredFile::get_device() const
{
    return inner->get_device();
}

void glaxnimate::ps::FilteredFile::on_close()
{

}

void glaxnimate::ps::SimpleFileFilter::filter_write(const QByteArray &data)
{
    inner->write(convert(data));
}

bool glaxnimate::ps::SimpleFileFilter::skip(char ch) const
{
    return std::isspace(ch) || ch == '\0';
}

bool SimpleFileFilter::marks_end(char)
{
    return false;
}

void LZWEncode::put_char(char ch)
{
    if ( dict.empty() )
    {
        reset_dict();
        write_code(options.clear_table_marker);
        current = QByteArray(1, ch);
        return;
    }

    QByteArray candidate = current + ch;
    auto it = dict.find(candidate);
    if ( it != dict.end() )
    {
        current = candidate;
        return;
    }

    it = dict.find(current);
    write_code(it == dict.end() ? 0 : it->second);

    if ( next_code < LZWOptions::max_code )
    {
        dict[candidate] = next_code++;
        if ( next_code >= quint16(1u << code_length) - options.early_change && code_length < LZWOptions::max_bits )
            code_length++;
    }
    else
    {
        write_code(options.clear_table_marker);
        reset_dict();
    }

    current = QByteArray(1, ch);
}

void LZWEncode::reset_dict()
{
    dict.clear();
    for ( int i = 0; i < 256; i++ )
        dict[QByteArray(1, char(i))] = i;

    code_length = options.unit_length + 1;
    next_code = options.first_entry;
}

void LZWEncode::write_code(quint16 code)
{
    accum = (accum << code_length) | code;
    bit_count += code_length;
    while ( bit_count >= 8 )
    {
        bit_count -= 8;
        inner->put_char(char((accum >> bit_count) & 0xff));
    }
}

void LZWEncode::finish()
{
    if ( dict.empty() )
        return;

    if ( !current.isEmpty() )
        write_code(dict[current]);

    write_code(options.eod_marker);

    inner->put_char(char((accum << (8 - bit_count)) & 0xff));
    accum = 0;
    bit_count = 0;

    current.clear();
    dict.clear();
}

std::optional<char> LZWDecode::get_char()
{
    if ( read_buffer.isEmpty() )
    {
        if ( end_reached )
            return {};

        fill_buffer();

        if ( read_buffer.isEmpty() )
            return {};
        std::reverse(read_buffer.begin(), read_buffer.end());
    }

    char ch = read_buffer.back();
    read_buffer.erase(read_buffer.end() - 1);
    return ch;

}

void LZWDecode::fill_buffer()
{
    if ( dict.empty() )
        reset_dict();

    while ( read_buffer.isEmpty() )
    {
        auto ch = inner->get_char();
        if ( !ch )
            return;

        accum = (accum << 8) | quint8(*ch);
        bit_count += 8;

        while ( bit_count >= code_length )
        {
            bit_count -= code_length;
            quint16 code = quint16((accum >> bit_count) & ((1u << code_length) - 1));


            if ( code == options.clear_table_marker )
            {
                reset_dict();
                break;
            }

            if ( code == options.eod_marker )
            {
                end_reached = true;
                return;
            }

            QByteArray entry;
            if ( code < next_code )
                entry = dict[code];
            else if ( !first_code && code == next_code )
                entry = prev + prev[0];
            else
                break; // invalid code

            read_buffer = entry;

            if ( !first_code && next_code < options.max_code )
            {
                dict.push_back(prev + entry[0]);
                next_code++;
                if ( next_code >= quint16(1u << code_length) - options.early_change && code_length < options.max_bits )
                    code_length++;
            }

            prev = entry;
            first_code = false;
        }
    }
}

void LZWDecode::reset_dict()
{
    dict.resize(options.first_entry);
    for ( int i = 0; i < 256; i++ )
        dict[i] = QByteArray(1, char(i));
    code_length = options.unit_length + 1;
    next_code  = options.first_entry;
    first_code = true;
    prev.clear();
}

bool DCTDecode::read_into_image(QImage &out)
{
    FileDevice device(inner.get());
    QImageReader reader(&device, "jpeg");
    return reader.read(&out);
}

QByteArray DCTDecode::process_file(FileInterface *inner)
{
    QImage image;
    FileDevice device(inner);
    QImageReader reader(&device, "jpeg");
    if ( reader.read(&image) )
        return {};

    return QByteArray((const char*)image.constBits(), image.sizeInBytes());
}
