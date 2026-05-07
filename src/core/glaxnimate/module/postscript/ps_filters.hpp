/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ps_value.hpp"
#include "base85.hpp"


namespace glaxnimate::ps {


class FilteredFile : public FileInterface
{
public:
    FilteredFile(std::shared_ptr<FileInterface> inner);

    bool readable() const override;
    bool writable() const override;
    bool is_open() const override;
    bool eof() const override;
    bool is_filtered() const override;
    bool is_seekable() const override;
    bool seek(int pos) override;
    int tell() const override;
    void flush() override;
    void reset() override;
    void close() override;
    QByteArray read(int count) override;
    void write(const QByteArray &data) override;
    QIODevice* get_device() const override;
    void unget_char(char c) override;

protected:
    virtual void on_close();

    std::shared_ptr<FileInterface> inner;
    bool end_reached = false;
    QByteArray read_buffer;
    QByteArray write_buffer;
};

class SimpleFileFilter : public FilteredFile
{
public:
    using FilterFunc = QByteArray (*)(const QByteArray&);

    SimpleFileFilter(
        std::shared_ptr<FileInterface> inner,
        FilterFunc convert_func,
        int input_size,
        int output_size
    ) : FilteredFile(std::move(inner)),
        convert_func(convert_func),
        input_size(input_size),
        output_size(output_size)
    {}

    void flush() override;
    std::optional<char> get_char() override;
    void put_char(char c) override;
    std::vector<std::uintptr_t> comparator() const override;

protected:
    virtual bool skip(char ch) const;
    virtual bool marks_end(char ch);

    void filter_write(const QByteArray& data);

    virtual bool needs_more(const QByteArray& data) const
    {
        return data.size() % input_size != 0;
    }

    QByteArray convert(const QByteArray& data) const
    {
        return convert_func(data);
    }

    FilterFunc convert_func;
    int input_size;
    int output_size;
};

class ASCIIHexDecode : public SimpleFileFilter
{
public:
    ASCIIHexDecode(std::shared_ptr<FileInterface> inner, ValueDict)
        : SimpleFileFilter(
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

class ASCIIHexEncode : public SimpleFileFilter
{
public:
    ASCIIHexEncode(std::shared_ptr<FileInterface> inner, ValueDict)
        : SimpleFileFilter(
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


class ASCII85Decode : public SimpleFileFilter
{
public:
    ASCII85Decode(std::shared_ptr<FileInterface> inner, ValueDict)
        : SimpleFileFilter(
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

class ASCII85Encode : public SimpleFileFilter
{
public:
    ASCII85Encode(std::shared_ptr<FileInterface> inner, ValueDict)
        : SimpleFileFilter(
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
    LZWEncode(std::shared_ptr<FileInterface> inner, const ValueDict& optdict)
        : FilteredFile(std::move(inner)),
          options(LZWOptions::from_dict(optdict)),
          code_length(options.unit_length + 1),
          next_code(options.first_entry)
    {}

    std::optional<char> get_char() override { return {}; }
    void unget_char(char) override {}
    void put_char(char ch) override;

protected:
    std::vector<std::uintptr_t> comparator() const override
    {
        static int marker;
        return {std::uintptr_t(inner.get()), std::uintptr_t(&marker)};
    }

    void on_close() override
    {
        finish();
    }

    void reset_dict();

    void write_code(quint16 code);

    void finish();


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
    LZWDecode(std::shared_ptr<FileInterface> inner, const ValueDict& optdict)
        : FilteredFile(std::move(inner)),
          options(LZWOptions::from_dict(optdict)),
          code_length(options.unit_length + 1)
    {}

    std::optional<char> get_char() override;

    void put_char(char) override {}

protected:
    quint16 next_code();
    void fill_buffer();

    std::vector<std::uintptr_t> comparator() const override
    {
        static int marker;
        return {std::uintptr_t(inner.get()), std::uintptr_t(&marker)};
    }

    void on_close() override
    {
    }

    void reset_dict();

    LZWOptions options;
    quint16 code_length;
    std::vector<QByteArray> dict;
    quint32 accum = 0;
    int bit_count = 0;
    bool first_code = true;
    QByteArray prev;
};

class GlobalFilter : public FileInterface
{
public:
    GlobalFilter(std::shared_ptr<FileInterface> inner, const ValueDict&)
        : inner(std::move(inner))
    {}

    bool is_open() const override
    {
        return inner->is_open();
    }

    bool eof() const override
    {
        return processed && pos >= data.size();
    }

    bool is_filtered() const override
    {
        return true;
    }

    bool is_seekable() const override
    {
        return true;
    }

    bool seek(int pos) override
    {
        this->pos = pos;
        return true;
    }

    int tell() const override
    {
        return pos;
    }

    void flush() override
    {
        if ( writable() )
            inner->write(process(data));
    }

    void reset() override
    {
        data = {};
    }

    void close() override
    {
        flush();
        inner->close();
    }

    std::optional<char> get_char() override
    {
        processed_read_data();
        if ( pos >= data.size() )
            return {};
        return data[pos++];
    }

    void unget_char(char) override
    {
        pos--;
    }

    void put_char(char c) override
    {
        data.push_back(c);
    }

    QByteArray read(int count) override
    {
        auto chunk = data.slice(pos, count);
        pos += count;
        return chunk;
    }

    void write(const QByteArray &data) override
    {
        this->data = data;
    }

    QByteArray read_all() override
    {
        return data;
    }

    QIODevice *get_device() const override
    {
        return inner->get_device();
    }

protected:
    virtual QByteArray process(const QByteArray& source) = 0;
    virtual QByteArray process_file(FileInterface* inner)
    {
        return process(inner->read_all());
    }

    const QByteArray& processed_read_data()
    {
        if ( !processed )
        {
            processed = true;
            data = process_file(inner.get());
        }

        return data;
    }

    QByteArray data;
    int pos = 0;
    bool processed = false;
    std::shared_ptr<FileInterface> inner;
};

class DCTDecode : public GlobalFilter
{
public:
    DCTDecode(std::shared_ptr<FileInterface> inner, const ValueDict& opts)
        : GlobalFilter(std::move(inner), opts)
    {}

    bool readable() const override { return true; }
    bool writable() const override { return false; }

    bool is_image_source() const override { return true; }
    bool read_into_image(QImage& out)  override;

protected:
    void extracted() const;
    QByteArray process(const QByteArray &) override { return {}; }
    QByteArray process_file(FileInterface* inner) override;

};

} // namespace glaxnimate::ps
