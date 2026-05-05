/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ps_value.hpp"
#include "ps_text_encoding.hpp"


namespace glaxnimate::ps {

class SizedFileFilter : public FilteredFile
{
public:
    using FilterFunc = QByteArray (*)(const QByteArray&);

    SizedFileFilter(
        std::shared_ptr<FileInterface> inner,
        ValueDict options,
        FilterFunc convert_func,
        int input_size,
        int output_size
    ) : FilteredFile(std::move(inner), std::move(options)),
        convert_func(convert_func),
        input_size(input_size),
        output_size(output_size)
    {}

protected:
    bool needs_more(const QByteArray& data) const override
    {
        return data.size() % input_size != 0;
    }

    QByteArray convert(const QByteArray& data) const override
    {
        return convert_func(data);
    }

    int buffer_size_hint() const override
    {
        return input_size;
    }

    std::uintptr_t filter_type_id() const override
    {
        return std::uintptr_t(convert_func);
    }

    FilterFunc convert_func;
    int input_size;
    int output_size;
};

class ASCIIHexDecode : public SizedFileFilter
{
public:
    ASCIIHexDecode(std::shared_ptr<FileInterface> inner, ValueDict options)
        : SizedFileFilter(
            std::move(inner),
            std::move(options),
            &QByteArray::fromHex,
            2, 1
        )
    {}

protected:
    bool marks_end(char ch) override
    {
        return ch == '>';
    }
};

class ASCIIHexEncode : public SizedFileFilter
{
public:
    ASCIIHexEncode(std::shared_ptr<FileInterface> inner, ValueDict options)
        : SizedFileFilter(
            std::move(inner),
            std::move(options),
            &ASCIIHexEncode::hex_encode,
            2, 1
        )
    {}

protected:
    static QByteArray hex_encode(const QByteArray& data)
    {
        return data.toHex();
    }

    void on_close() override
    {
        inner->write(">");
    }

};


class ASCII85Decode : public SizedFileFilter
{
public:
    ASCII85Decode(std::shared_ptr<FileInterface> inner, ValueDict options)
        : SizedFileFilter(
            std::move(inner),
            std::move(options),
            &Base85Decoder::decode,
            5, 4
        )
    {}

protected:
    bool marks_end(char ch) override
    {
        if ( ch != '~' )
            return false;

        auto d = inner->get_char();
        if ( d && *d != '>' )
            inner->unget_char(*d);

        return true;
    }
};

class ASCII85Encode : public SizedFileFilter
{
public:
    ASCII85Encode(std::shared_ptr<FileInterface> inner, ValueDict options)
        : SizedFileFilter(
            std::move(inner),
            std::move(options),
            &Base85Encoder::encode,
            4, 5
        )
    {}

protected:
    void on_close() override
    {
        inner->write("~>");
    }
};


} // namespace glaxnimate::ps
