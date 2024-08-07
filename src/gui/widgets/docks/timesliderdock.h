# pragma once

#include "widgets/dialogs/glaxnimate_window.hpp"
#include "widgets/timeline/frame_controls_widget.hpp"
#include <QDockWidget>
#include <QObject>

namespace glaxnimate::gui {

class TimeSliderDock : public QDockWidget
{
    Q_OBJECT

public:
    TimeSliderDock(GlaxnimateWindow* parent);

    ~TimeSliderDock();

    glaxnimate::gui::FrameControlsWidget* playControls();


private:
    class Private;
    std::unique_ptr<Private> d;
};

}
