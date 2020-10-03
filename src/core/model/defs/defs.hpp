#pragma once

#include "model/object.hpp"
#include "model/property/object_list_property.hpp"
#include "named_color.hpp"
#include "bitmap.hpp"

namespace model {

class Defs : public ObjectBase<Defs, Object>
{
    GLAXNIMATE_OBJECT

    GLAXNIMATE_PROPERTY_LIST(NamedColor, colors, &Defs::on_color_added, &Defs::on_color_removed, {}, {}, {}, {})
    GLAXNIMATE_PROPERTY_LIST(Bitmap, images, {}, {}, {}, {}, {}, {})

public:
    using Ctor::Ctor;

    std::vector<ReferenceTarget*> valid_brush_styles() const;
    bool is_valid_brush_style(ReferenceTarget* style) const;


    std::vector<ReferenceTarget*> valid_images() const;
    bool is_valid_image(ReferenceTarget* style) const;

    Q_INVOKABLE model::ReferenceTarget* find_by_uuid(const QUuid& n) const;
    Q_INVOKABLE model::NamedColor* add_color(const QColor& color, const QString& name = {});
    Q_INVOKABLE model::Bitmap* add_image(const QString& filename, bool embed);

signals:
    void color_added(int position, model::NamedColor* color);
    void color_removed(int position);
    void color_changed(int position, model::NamedColor* color);

private:
    void on_color_added(NamedColor* color, int position);
    void on_color_removed(NamedColor* color, int position);
};

} // namespace model
