# pragma once

#include "QtColorWidgets/color_palette_model.hpp"
#include "model/shapes/stroke.hpp"
#include "widgets/dialogs/glaxnimate_window.hpp"
#include <QDockWidget>
#include <QObject>

namespace glaxnimate::gui {

class StrokeDock : public QDockWidget
{
    Q_OBJECT

public:
    StrokeDock(GlaxnimateWindow* parent);

    ~StrokeDock();

    void clear_document();
    void save_settings() const;

    glaxnimate::model::Stroke * current() const;
    void set_current(model::Stroke* stroke);

    void set_targets(std::vector<model::Stroke *> targets);

    QColor current_color() const;
    void set_color(const QColor& color);
    QPen pen_style() const;
    void set_palette_model(color_widgets::ColorPaletteModel* palette_model);

public Q_SLOTS:
    void set_gradient_stop(model::Styler* styler, int index);

Q_SIGNALS:
    void color_changed(const QColor& c);
    void pen_style_changed();

private:
    class Private;
    std::unique_ptr<Private> d;
};

}
