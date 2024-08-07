# pragma once

#include "widgets/dialogs/glaxnimate_window.hpp"
#include "widgets/timeline/frame_controls_widget.hpp"
#include <QDockWidget>
#include <QObject>

namespace glaxnimate::gui {

class UndoDock : public QDockWidget
{
    Q_OBJECT

public:
    UndoDock(GlaxnimateWindow* parent);

    ~UndoDock();

    void setUndoGroup(QUndoGroup *group);

private:
    class Private;
    std::unique_ptr<Private> d;
};

}
