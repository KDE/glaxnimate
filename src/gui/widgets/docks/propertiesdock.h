# pragma once

#include "item_models/property_model_single.hpp"
#include "style/property_delegate.hpp"
#include "widgets/dialogs/glaxnimate_window.hpp"
#include <QDockWidget>
#include <QObject>

namespace glaxnimate::gui {

class PropertiesDock : public QDockWidget
{
    Q_OBJECT

public:
    PropertiesDock(GlaxnimateWindow* parent, item_models::PropertyModelSingle* property_model, style::PropertyDelegate* property_delegate);

    ~PropertiesDock();

    void expandAll();

private:
    class Private;
    std::unique_ptr<Private> d;
};

}
