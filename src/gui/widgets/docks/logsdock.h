# pragma once

#include "app/log/log_model.hpp"
#include "widgets/dialogs/glaxnimate_window.hpp"
#include <QDockWidget>
#include <QObject>

namespace glaxnimate::gui {

class LogsDock : public QDockWidget
{
    Q_OBJECT

public:
    LogsDock(GlaxnimateWindow* parent, app::log::LogModel *logModel);

    ~LogsDock();

private:
    class Private;
    std::unique_ptr<Private> d;
};

}
