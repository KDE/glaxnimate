#include "undodock.h"

#include "ui_undo.h"

using namespace glaxnimate::gui;

class UndoDock::Private
{
public:
    ::Ui::dock_undo ui;
};

UndoDock::UndoDock(GlaxnimateWindow *parent)
    : QDockWidget(parent)
    , d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
}

UndoDock::~UndoDock() = default;

void UndoDock::setUndoGroup(QUndoGroup *group)
{
    d->ui.view_undo->setGroup(group);
}
