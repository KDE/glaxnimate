/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ps_value.hpp"
#include "base85.hpp"


namespace glaxnimate::ps {

class SizedFileFilter : public FilteredFile
{
public:
    using FilterFunc = QByteArray (*)(const QByteArray&);

    SizedFileFilter(
        std::shared_ptr<FileInterface> inner,
        FilterFunc convert_func,
        int input_size,
        int output_size
    ) : FilteredFile(std::move(inner)),
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
    ASCIIHexDecode(std::shared_ptr<FileInterface> inner, ValueDict)
        : SizedFileFilter(
            std::move(inner),
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
    ASCIIHexEncode(std::shared_ptr<FileInterface> inner, ValueDict)
        : SizedFileFilter(
            std::move(inner),
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
    ASCII85Decode(std::shared_ptr<FileInterface> inner, ValueDict)
        : SizedFileFilter(
            std::move(inner),
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

    bool needs_more(const QByteArray& data) const override
    {
        if ( data.startsWith('z') )
            return false;
        return data.size() % input_size != 0;
    }
};

class ASCII85Encode : public SizedFileFilter
{
public:
    ASCII85Encode(std::shared_ptr<FileInterface> inner, ValueDict)
        : SizedFileFilter(
            std::move(inner),
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

struct LZWOptions
{
    int early_change = 1;
    int unit_length = 8;
    bool low_bit_first = false;
    int predictor = 1;
    int columns = 1;
    int colors = 1;
    int bits_per_component = 8;
    bool close = false;

    quint16 clear_table_marker = 256;
    quint16 eod_marker = 257;
    quint16 first_entry = 258;


    static constexpr int max_bits = 12;
    static constexpr int max_code = 1 << max_bits;

    void update_markers()
    {
        clear_table_marker = 1 << unit_length;
        eod_marker = clear_table_marker + 1;
        first_entry = clear_table_marker + 2;
    }


    static LZWOptions from_dict(ValueDict options)
    {
        LZWOptions opts;
        opts.early_change = options.get_default("EarlyChange", opts.early_change);
        opts.unit_length = options.get_default("UnitLength", opts.unit_length);
        opts.low_bit_first = options.get_default("LowBitFirst", opts.low_bit_first);
        opts.columns = options.get_default("Columns", opts.columns);
        opts.colors = options.get_default("Colors", opts.colors);
        opts.bits_per_component = options.get_default("Colors", opts.bits_per_component);
        opts.close = options.get_default("CloseTarget", options.get_default("CloseSource", opts.close));
        opts.update_markers();
        return opts;
    }
};

class LZWEncode : public FilteredFile
{
public:
    LZWEncode(std::shared_ptr<FileInterface> inner, ValueDict optdict)
        : FilteredFile(std::move(inner)),
          options(LZWOptions::from_dict(optdict)),
          code_length(options.unit_length + 1),
          next_code(options.first_entry)
    {}

    std::optional<char> get_char() override { return {}; }

    void put_char(char ch) override
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

protected:
    bool needs_more(const QByteArray&) const override
    {
        return false;
    }

    QByteArray convert(const QByteArray&) const override
    {
        return {};
    }

    int buffer_size_hint() const override
    {
        return 0;
    }

    std::uintptr_t filter_type_id() const override
    {
        return -1;
    }

    void on_close() override
    {
        finish();
    }

    void reset_dict()
    {
        dict.clear();
        for ( int i = 0; i < 256; i++ )
            dict[QByteArray(1, char(i))] = i;

        code_length = options.unit_length + 1;
        next_code = options.first_entry;
    }

    void write_code(quint16 code)
    {
        accum = (accum << code_length) | code;
        bit_count += code_length;
        while ( bit_count >= 8 )
        {
            bit_count -= 8;
            inner->put_char(char((accum >> bit_count) & 0xff));
        }
    }

    void finish()
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


    LZWOptions options;
    quint16 code_length;
    quint16 next_code;
    std::map<QByteArray, quint16> dict;
    QByteArray current;
    quint32 accum = 0;
    int bit_count = 0;
};


class LZWDecode : public FilteredFile
{
public:
    LZWDecode(std::shared_ptr<FileInterface> inner, ValueDict optdict)
        : FilteredFile(std::move(inner)),
          options(LZWOptions::from_dict(optdict)),
          code_length(options.unit_length + 1),
          next_code(options.first_entry)
    {}

    std::optional<char> get_char() override
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

    void put_char(char) override
    {}

protected:
    void fill_buffer()
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

    bool needs_more(const QByteArray&) const override
    {
        return false;
    }

    QByteArray convert(const QByteArray&) const override
    {
        return {};
    }

    int buffer_size_hint() const override
    {
        return 0;
    }

    std::uintptr_t filter_type_id() const override
    {
        return -2;
    }

    void on_close() override
    {
    }

    void reset_dict()
    {
        dict.resize(options.first_entry);
        for ( int i = 0; i < 256; i++ )
            dict[i] = QByteArray(1, char(i));
        code_length = options.unit_length + 1;
        next_code  = options.first_entry;
        first_code = true;
        prev.clear();
    }

    LZWOptions options;
    quint16 code_length;
    quint16 next_code;
    std::vector<QByteArray> dict;
    quint32 accum = 0;
    int bit_count = 0;
    bool first_code = true;
    QByteArray prev;
};

} // namespace glaxnimate::ps
