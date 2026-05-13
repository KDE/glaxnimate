/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ps_image.hpp"

using namespace glaxnimate::ps;

ImageLoader::DataSource ImageLoader::data_source(const Value &datasrc, Interpreter &interpreter)
{
    switch ( datasrc.type() )
    {
        case Value::File:
            return {datasrc.cast<File>()};
        case Value::Array:
        case Value::String:
            return {File(std::make_shared<ProcedureDataSource>(&interpreter, datasrc))};
        default:
            return {};
    }

}

std::optional<ImageLoader> ImageLoader::from_args(const ValueArray &args, Interpreter &interpreter)
{
    auto matrix =  array_to_matrix(args[3].cast<ValueArray>());
    if ( !matrix )
        return {};

    return ImageLoader(
        ColorSpace(ColorSpaceType::DeviceGray),
        args[0].cast<int>(),
        args[1].cast<int>(),
        args[2].cast<int>(),
        *matrix,
        {0, 1},
        {data_source(args[4], interpreter)},
        true
        );
}

std::optional<ImageLoader> ImageLoader::from_args_colorimage(const ValueArray &args, Interpreter &interpreter)
{
    int ncomp = args[1].cast<int>();
    ColorSpaceType space_type = ColorSpaceType::DeviceGray;
    if ( ncomp == 1 )
        space_type = ColorSpaceType::DeviceGray;
    else if ( ncomp == 3 )
        space_type = ColorSpaceType::DeviceRGB;
    else if ( ncomp == 4 )
        space_type = ColorSpaceType::DeviceCMYK;
    else
        return {};

    ColorSpace space(space_type);

    bool multi = args[0].cast<bool>();

    int source_count = multi ? ncomp : 1;
    if ( interpreter.stack().size() < source_count + 4 )
        return {};

    std::vector<DataSource> data_sources(source_count);
    for ( int i = 0; i < source_count; i++ )
        data_sources[source_count - i - 1] = data_source(interpreter.stack().pop(), interpreter);

    auto matrix = array_to_matrix(interpreter.stack().pop().cast<ValueArray>());
    if ( !matrix )
        return {};

    int bits = interpreter.stack().pop().cast<int>();
    int h = interpreter.stack().pop().cast<int>();
    int w = interpreter.stack().pop().cast<int>();
    std::vector<float> decode(ncomp * 2, 0);
    for ( int i = 1; i < ncomp * 2; i += 2 )
        decode[i] = 1;

    return ImageLoader(
        space,
        w,
        h,
        bits,
        std::move(*matrix),
        std::move(decode),
        std::move(data_sources),
        true
    );
}

std::optional<ImageLoader> ImageLoader::from_dict(const ValueDict &dict, Interpreter &interpreter)
{
    int image_type = dict.get_default("ImageType", 0);
    if ( image_type != 1 )
        return {}; // TODO other types

    int w = dict.get_default("Width", -1);
    int h = dict.get_default("Height", -1);
    std::vector<DataSource> data_sources;
    bool multiple_sources = dict.get_default("MultipleDataSources", false);
    auto it = dict.find("DataSource");
    if ( it == dict.end() || w < 1 || h < 1 )
        return {};

    int component_count = interpreter.memory().gstate.color_space.component_count();

    if ( multiple_sources )
    {
        if ( it->second.type() != Value::Array )
            return {};
        auto sources = it->second.cast<ValueArray>();
        if ( sources.size() < component_count )
            return {};

        data_sources.reserve(sources.size());
        for ( const auto& source: sources )
            data_sources.push_back(data_source(source, interpreter));
    }
    else
    {
        data_sources.push_back(data_source(it->second, interpreter));
    }

    it = dict.find("Decode");
    if ( it == dict.end() || it->second.type() != Value::Array )
        return {};

    auto decode_v = it->second.cast<ValueArray>();
    if ( decode_v.size() < component_count * 2 )
        return {};

    std::vector<float> decode(component_count * 2);
    for ( int i = 0; i < component_count * 2; i++ )
        decode[i] = decode_v[i].cast<float>();

    auto tf = array_to_matrix(dict.get_default("ImageMatrix", ValueArray{}));
    if ( !tf )
        return {};

    return ImageLoader(
        interpreter.memory().gstate.color_space,
        w,
        h,
        dict.get_default("BitsPerComponent", 1),
        std::move(*tf),
        std::move(decode),
        std::move(data_sources),
        dict.get_default("Interpolate", true)
    );
}

const ImageData &ImageLoader::load()
{
    if ( sources.size() == 1 && sources[0].file.is_image_source() )
    {
        sources[0].file.read_into_image(data.image);
        return data;
    }

    for ( int y = 0; y < height; y++ )
    {
        QRgb* line = reinterpret_cast<QRgb*>(data.image.scanLine(y));
        for ( auto& src : sources )
            src.clear_bits();

        for ( int x = 0; x < width; x++ )
        {
            line[x] = color_to_argb32(read_components());
        }
    }

    return data;
}
