/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <deque>
#include <QTransform>
#include <QMetaEnum>
#include "glaxnimate/math/bezier/bezier.hpp"
#include "glaxnimate/model/shapes/style/stroke.hpp"

namespace glaxnimate::ps {

Q_NAMESPACE

enum class ColorSpaceType
{
    // Level 2
    DeviceGrey,
    DeviceRGB,
    DeviceCYMK,
    /*CIEBasedABC,
    CIEBasedA,
    Pattern,
    Indexed,
    Separation,
    // Level 3
    CIEBasedDEF,
    CIEBasedDEFG,
    DeviceN,*/

    // Out of specs
    CustomHSV,

};

Q_ENUM_NS(ColorSpaceType);

class ColorSpace
{
public:
    ColorSpace(ColorSpaceType type)
        : type_(type),
          component_count_(color_space_components(type))
    {
    }

    bool from_name(const QByteArray& name)
    {
        bool ok = false;
        auto space = ColorSpaceType(QMetaEnum::fromType<ColorSpaceType>().keyToValue(name.data(), &ok));
        if ( !ok )
            return false;
        type_ = space;
        component_count_ = color_space_components(type_);
        return true;
    }

    QByteArray name() const
    {
        return QMetaEnum::fromType<ColorSpaceType>().valueToKey(int(type_));
    }

    static int color_space_components(ColorSpaceType t)
    {
        switch ( t )
        {
            case glaxnimate::ps::ColorSpaceType::DeviceGrey: return 1;
            case glaxnimate::ps::ColorSpaceType::CustomHSV:
            case glaxnimate::ps::ColorSpaceType::DeviceRGB:
                return 3;
            case glaxnimate::ps::ColorSpaceType::DeviceCYMK: return 4;
        }
        return 0;
    }

    bool make_color(const std::vector<float>& comps, QColor* out) const
    {
        if ( int(comps.size()) < component_count_ )
            return false;

        switch ( type_ )
        {
            case ColorSpaceType::DeviceGrey:
                *out = QColor::fromRgbF(comps[0], comps[0], comps[0]);
                return true;
            case ColorSpaceType::DeviceRGB:
                *out = QColor::fromRgbF(comps[0], comps[1], comps[2]);
                return true;
            case ColorSpaceType::DeviceCYMK:
                *out = QColor::fromCmykF(comps[0], comps[1], comps[2], comps[3]);
                return true;
            case ColorSpaceType::CustomHSV:
                *out = QColor::fromHsvF(comps[0], comps[1], comps[2]);
                return true;
        }

        return false;
    }

    std::vector<float> components(const QColor& col) const
    {
        switch ( type_ )
        {
            case ColorSpaceType::DeviceGrey:
                return stabilize_components({col.greenF()});
            case ColorSpaceType::DeviceRGB:
                return stabilize_components({col.redF(), col.greenF(), col.blueF()});
            case ColorSpaceType::DeviceCYMK:
                return stabilize_components({col.cyanF(), col.yellowF(), col.magentaF(), col.blackF()});
            case ColorSpaceType::CustomHSV:
                return stabilize_components({col.hueF(), col.saturationF(), col.valueF()});
        }

        return {};
    }

    int component_count() const { return component_count_; }

    ColorSpaceType type() const { return type_; }

private:
    static std::vector<float> stabilize_components(std::vector<float>&& vec)
    {
        for ( float& v : vec )
        {
            v = qRound(v * 8192) / 8192.f;
        }

        return vec;
    }

    ColorSpaceType type_ = ColorSpaceType::DeviceGrey;
    int component_count_ = 1;
};


inline std::vector<float> matrix_elements(const QTransform& transform)
{
    return {
        float(transform.m11()),
        float(transform.m12()),
        float(transform.m21()),
        float(transform.m22()),
        float(transform.m31()),
        float(transform.m32()),
    };
}

inline QTransform matrix_from_elements(const std::vector<float>& elems)
{
    return QTransform(elems[0], elems[1], elems[2], elems[3], elems[4], elems[5]);
}

struct GraphicsState
{
    QTransform transform;

    math::bezier::MultiBezier path;
    math::bezier::MultiBezier clip;
    std::deque<math::bezier::MultiBezier> clip_stack;
    ColorSpace color_space = ColorSpaceType::DeviceGrey;
    QColor color = Qt::black;
    // todo font
    float line_width = 1;
    model::Stroke::Cap line_cap = model::Stroke::ButtCap;
    model::Stroke::Join line_join = model::Stroke::MiterJoin;
    float miter_limit = 10;
    std::vector<float> dash_pattern;
    float dash_offset = 0;

    // Device dependent
    float flatness = 10;


    enum InverseTransformState
    {
        Clean, Dirty, Invalid,
    };
    QTransform inverse_transform_matrix;
    InverseTransformState inverse_transform_state = Clean;


    bool position_is_defined()
    {
        if ( path.empty() || path.back().empty() )
            return false;

        inverse_transform();
        return inverse_transform_state == Clean;
    }

    QPointF device_position()
    {
        return path.back().back().pos;
    }

    QPointF position()
    {
        return inverse_transform().map(device_position());
    }

    void mark_transform_changed()
    {
        inverse_transform_state = Dirty;
    }

    const QTransform& inverse_transform()
    {
        if ( inverse_transform_state == Dirty )
        {
            bool ok = false;
            inverse_transform_matrix = transform.inverted(&ok);
            inverse_transform_state = ok ? Clean : Invalid;
        }
        return inverse_transform_matrix;
    }

    static model::Stroke::Cap convert_cap(int ps_val)
    {
        switch ( ps_val )
        {
            default:
            case 0: return model::Stroke::ButtCap;
            case 1: return model::Stroke::RoundCap;
            case 2: return model::Stroke::SquareCap;
        }
    }

    static int convert_cap(model::Stroke::Cap cap)
    {
        switch ( cap )
        {
            case model::Stroke::ButtCap: return 0;
            case model::Stroke::RoundCap: return 1;
            case model::Stroke::SquareCap: return 2;
        }
        return 0;
    }

    static model::Stroke::Join convert_join(int ps_val)
    {
        switch ( ps_val )
        {
            default:
            case 0: return model::Stroke::MiterJoin;
            case 1: return model::Stroke::RoundJoin;
            case 2: return model::Stroke::BevelJoin;
        }
    }

    static int convert_join(model::Stroke::Join join)
    {
        switch ( join )
        {
            case model::Stroke::MiterJoin: return 0;
            case model::Stroke::RoundJoin: return 1;
            case model::Stroke::BevelJoin: return 2;
        }
        return 0;
    }

    void flatten_path()
    {
        // TODO
    }

    QBrush to_brush() const
    {
        return color;
    }

    QPen to_pen()
    {
        QPen pen(to_brush(), line_width, Qt::SolidLine, Qt::PenCapStyle(line_cap), Qt::PenJoinStyle(line_join));
        pen.setMiterLimit(miter_limit);
        return pen;
    }


    void apply_clip(bool evenodd)
    {
        apply_clip(evenodd, clip);
    }

    void apply_clip(bool evenodd, const math::bezier::MultiBezier& clip)
    {
        QPainterPath shape = path.painter_path();
        QPainterPath clip_path = clip.painter_path();
        if ( evenodd )
        {
            shape.setFillRule(Qt::OddEvenFill);
            clip_path.setFillRule(Qt::OddEvenFill);
        }
        else
        {
            shape.setFillRule(Qt::WindingFill);
            clip_path.setFillRule(Qt::WindingFill);
        }

        path = shape;
    }

    void add_arc(const QPointF center, float radius, float angle_start, float angle_end);
    void add_arc_radians(const QPointF center, float radius, float angle_start, float angle_end);

};


} // namespace glaxnimate::ps
