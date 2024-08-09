# pragma once

#include "item_models/document_model_base.hpp"
#include "widgets/dialogs/glaxnimate_window.hpp"
#include "widgets/docks/layer_view.hpp"
#include <QDockWidget>
#include <QObject>

namespace glaxnimate::gui {

class LayersDock : public QDockWidget
{
    Q_OBJECT

public:
    LayersDock(GlaxnimateWindow* parent, item_models::DocumentModelBase* base_model);

    ~LayersDock();

    glaxnimate::gui::LayerView* layer_view();

private:
    class Private;
    std::unique_ptr<Private> d;
};

}
