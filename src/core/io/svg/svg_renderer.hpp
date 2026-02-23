/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QIODevice>
#include <QDomDocument>

#include "model/shapes/shape.hpp"

namespace glaxnimate::model {
    class EmbeddedFont;
} // namespace glaxnimate::model

namespace glaxnimate::io::svg {

enum AnimationType
{
    NotAnimated,
    SMIL
};

enum class CssFontType
{
    None,
    Embedded,
    FontFace,
    Link,
};

class SvgRenderer
{
public:
    SvgRenderer(AnimationType animated, CssFontType font_type);
    ~SvgRenderer();

    void write_composition(model::Composition* comp, model::FrameTime t);
    void write_main(model::Composition* comp, model::FrameTime t);
    void write_shape(model::ShapeElement* shape, model::FrameTime t);
    void write_node(model::DocumentNode* node, model::FrameTime t);

    QDomDocument dom() const;

    void write(QIODevice* device, bool indent);

    static CssFontType suggested_type(model::EmbeddedFont* font);
private:
    class Private;
    std::unique_ptr<Private> d;
};

/**
 * \brief Converts a multi bezier into path data
 * \returns pair of [path data, sodipodi nodetypes]
 */
std::pair<QString, QString> path_data(const math::bezier::MultiBezier& shape);

} // namespace glaxnimate::io::svg
