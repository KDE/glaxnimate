#include "propertiesdock.h"

#include "ui_properties.h"

using namespace glaxnimate::gui;

class PropertiesDock::Private
{
public:
    ::Ui::dock_properties ui;
};

PropertiesDock::PropertiesDock(GlaxnimateWindow* parent, item_models::PropertyModelSingle* property_model, style::PropertyDelegate* property_delegate)
    : QDockWidget(parent)
    , d(std::make_unique<Private>())
{
    d->ui.setupUi(this);

    d->ui.view_properties->setModel(property_model);

    d->ui.view_properties->setItemDelegateForColumn(item_models::PropertyModelSingle::ColumnValue, property_delegate);
    d->ui.view_properties->header()->setSectionResizeMode(item_models::PropertyModelSingle::ColumnName, QHeaderView::ResizeToContents);
    d->ui.view_properties->header()->setSectionResizeMode(item_models::PropertyModelSingle::ColumnValue, QHeaderView::Stretch);
}

PropertiesDock::~PropertiesDock() = default;

void PropertiesDock::expandAll()
{
    d->ui.view_properties->expandAll();
}
