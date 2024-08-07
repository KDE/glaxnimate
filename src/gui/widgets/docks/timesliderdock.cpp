#include "timesliderdock.h"

#include "app/log/log_model.hpp"
#include "style/better_elide_delegate.hpp"
#include "ui_time_slider.h"

using namespace glaxnimate::gui;

class TimeSliderDock::Private
{
public:
    ::Ui::dock_time_slider ui;
};

TimeSliderDock::TimeSliderDock(GlaxnimateWindow *parent)
    : QDockWidget(parent)
    , d(std::make_unique<Private>())
{
    d->ui.setupUi(this);

    d->ui.play_controls_2->set_record_enabled(false);

    connect(d->ui.scroll_time, &QSlider::valueChanged, d->ui.play_controls_2, &FrameControlsWidget::frame_selected);
}

TimeSliderDock::~TimeSliderDock() = default;

glaxnimate::gui::FrameControlsWidget* TimeSliderDock::playControls()
{
    return d->ui.play_controls_2;
}

QSlider* TimeSliderDock::timeSlider()
{
    return d->ui.scroll_time;
}
