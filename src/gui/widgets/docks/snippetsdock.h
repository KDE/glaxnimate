# pragma once

#include "model/assets/gradient.hpp"
#include "model/shapes/fill.hpp"
#include "model/shapes/stroke.hpp"

#include "widgets/dialogs/glaxnimate_window.hpp"
#include <QDockWidget>
#include <QObject>
#include <QtColorWidgets/color_palette_model.hpp>

namespace glaxnimate::gui {

class SnippetsDock : public QDockWidget
{
    Q_OBJECT

public:
    SnippetsDock(GlaxnimateWindow* parent);

    ~SnippetsDock();

private:
    class Private;
    std::unique_ptr<Private> d;
};

}
