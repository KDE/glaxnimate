#include "tooloptionsdock.h"

#include "ui_tooloptions.h"

using namespace glaxnimate::gui;

class ToolOptionsDock::Private
{
public:
    ::Ui::dock_tool_options ui;
};

ToolOptionsDock::ToolOptionsDock(GlaxnimateWindow *parent)
    : QDockWidget(parent)
    , d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
}

ToolOptionsDock::~ToolOptionsDock() = default;

void ToolOptionsDock::addSettingsWidget(QWidget *w)
{
    d->ui.tool_settings_widget->addWidget(w);
}

void ToolOptionsDock::setCurrentWidget(QWidget *w)
{
    d->ui.tool_settings_widget->setCurrentWidget(w);
}

QWidget* ToolOptionsDock::currentWidget() const
{
    return d->ui.tool_settings_widget->currentWidget();
}
