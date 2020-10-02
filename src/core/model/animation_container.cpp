#include "animation_container.hpp"
#include "model/factory.hpp"

GLAXNIMATE_OBJECT_IMPL(model::AnimationContainer)

bool model::AnimationContainer::time_visible(model::FrameTime time) const
{
    return first_frame.get() <= time && time <= last_frame.get();
}

bool model::AnimationContainer::time_visible() const
{
    return time_visible(time());
}

void model::AnimationContainer::set_time(model::FrameTime t)
{
    bool old_visible = time_visible();
    Object::set_time(t);
    bool new_visible = time_visible();
    if ( old_visible != new_visible )
        emit time_visible_changed(new_visible);
}

void model::AnimationContainer::on_first_frame_changed(int x)
{
    emit time_visible_changed(time_visible());
    emit first_frame_changed(x);
}

void model::AnimationContainer::on_last_frame_changed(int x)
{
    emit time_visible_changed(time_visible());
    emit last_frame_changed(x);
}
