#pragma once

#include <QIODevice>
#include <QDomDocument>

#include "glaxnimate/core/model/shapes/shape.hpp"

namespace glaxnimate::model {
    class MainComposition;
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

    void write_document(model::Document* document);
    void write_composition(model::Composition* comp);
    void write_main(model::MainComposition* comp);
    void write_shape(model::ShapeElement* shape);
    void write_node(model::DocumentNode* node);

    QDomDocument dom() const;

    void write(QIODevice* device, bool indent);

    static CssFontType suggested_type(model::EmbeddedFont* font);
private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::io::svg
