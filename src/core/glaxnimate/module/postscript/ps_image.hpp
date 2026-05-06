/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "ps_value.hpp"
#include "ps_gstate.hpp"
#include "ps_interpreter.hpp"

namespace glaxnimate::ps {

class ProcedureDataSource : public FileInterface
{
public:
    ProcedureDataSource(
        Interpreter* interpreter,
        Value proc
    ) : interpreter(interpreter),
        proc(std::move(proc))
    {}

    bool readable() const override { return true; }
    bool writable() const override { return false; }
    bool is_open() const override  { return true; }
    bool eof() const override  { return end_reached && read_buffer.isEmpty(); }
    bool is_filtered() const override { return false; };
    bool is_seekable() const override { return false; };
    bool seek(int) override  { return false; }
    int tell() const override  { return -1; }
    void flush() override  {}
    void reset() override  {}
    void close() override  {}

    std::optional<char> get_char() override
    {
        if ( read_buffer.isEmpty() )
        {
            if ( end_reached )
                return {};

            read_next_chunk();

            if ( read_buffer.isEmpty() )
                return {};

            std::reverse(read_buffer.begin(), read_buffer.end());
        }

        char ch = read_buffer.back();
        read_buffer.erase(read_buffer.end() - 1);
        return ch;
    }

    void unget_char(char c) override
    {
        read_buffer.push_back(c);
    }
    void put_char(char) override {}
    QByteArray read(int count) override
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
    void write(const QByteArray &) override {}
    QIODevice *get_device() const override { return nullptr; }

private:
    void read_next_chunk()
    {
        interpreter->execute(proc);
        if ( interpreter->stack().empty() )
        {
            interpreter->error(QStringLiteral("Stack underflow"));
            end_reached = true;
            return;
        }

        read_buffer = interpreter->stack().pop().cast<String>().bytes();

        if ( read_buffer.isEmpty() )
            end_reached = true;
    }

    Interpreter* interpreter;
    Value proc;
    QByteArray read_buffer;
    bool end_reached = false;
};


class ImageLoader
{
public:
    struct DataSource
    {
        File file;
        quint16 bit_buffer = 0;
        int bits_in_buffer = 0;

        void clear_bits()
        {
            bit_buffer = 0;
            bits_in_buffer = 0;
        }

        quint16 read_bits(int count)
        {
            while ( bits_in_buffer < count )
            {
                std::optional<char> byte = file.get_char();
                bit_buffer <<= 8;
                bits_in_buffer += 8;
                if ( byte )
                    bit_buffer |= quint8(*byte);
            }

            bits_in_buffer -= count;
            return (bit_buffer >> bits_in_buffer) & ((1u << count) - 1);
        }
    };

    ImageLoader(
        ColorSpace color_space,
        int width,
        int height,
        int bits,
        QTransform matrix,
        std::vector<float> decode,
        std::vector<DataSource> data_sources,
        bool interpolate
    ) : color_space(color_space),
        width(width),
        height(height),
        bits(bits),
        component_divisor((1 << bits) - 1),
        decode(std::move(decode)),
        sources(std::move(data_sources)),
        data{
            QImage(width, height, QImage::Format_ARGB32),
            std::move(matrix),
            interpolate
        }
    {
    }

    static DataSource data_source(const Value& datasrc, Interpreter& interpreter);

    static std::optional<ImageLoader> from_args(const ValueArray& args, Interpreter& interpreter);

    static std::optional<ImageLoader> from_args_colorimage(const ValueArray& args, Interpreter& interpreter);

    static std::optional<ImageLoader> from_dict(const ValueDict& dict, Interpreter& interpreter);

    const ImageData& load();

private:
    quint32 color_to_argb32(const std::vector<float>& components)
    {
        QColor color;
        color_space.make_color(components, &color);
        return color.rgba();
    }

    std::vector<float> read_components()
    {
        std::vector<float> comps;
        comps.resize(color_space.component_count());
        for ( int i = 0; i < color_space.component_count(); i++ )
            comps[i] = read_component(i);
        return comps;
    }

    float read_component(int comp)
    {
        int source_index = sources.size() == 1 ? 0 : comp;
        float input = double(sources[source_index].read_bits(bits)) / component_divisor;
        float min = decode[comp * 2];
        float max = decode[comp * 2 + 1];
        return min + (input * (max - min));
    }

    ColorSpace color_space;
    int width;
    int height;
    int bits;
    double component_divisor;
    std::vector<float> decode;
    std::vector<DataSource> sources;
    ImageData data;
};



} // namespace glaxnimate::ps
