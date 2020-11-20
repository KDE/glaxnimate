#pragma once

#include <memory>
#include <QWidget>

#include "model/document.hpp"

class GlaxnimateWindow;
class CompoundTimelineWidget : public QWidget
{
    Q_OBJECT
    
public:
    CompoundTimelineWidget(QWidget* parent = nullptr);
    ~CompoundTimelineWidget();
    
    void set_active(model::DocumentNode* node);
    void set_document(model::Document* document);
    void clear_document();
    QByteArray save_state() const;
    void load_state(const QByteArray& state);
    void set_controller(GlaxnimateWindow* window);
    
protected:
    void changeEvent ( QEvent* e ) override;
    
private slots:
    void select_property(const QModelIndex& index);
    void select_animatable(model::AnimatableBase* anim);
    void select_object(model::Object* anim);
    void custom_context_menu(const QPoint& p);
    void add_keyframe();
    void remove_keyframe();
    void on_scroll(int amount);
    void keyframe_action_enter();
    void keyframe_action_exit();
    
private:
    class Private;
    std::unique_ptr<Private> d;
};
