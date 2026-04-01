/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/command/shape_commands.hpp"
#include "glaxnimate/model/shapes/composable/group.hpp"
#include "glaxnimate/model/shapes/composable/precomp_layer.hpp"
#include "glaxnimate/model/assets/composition.hpp"
#include "glaxnimate/model/assets/assets.hpp"
#include "glaxnimate/model/document.hpp"
#include "glaxnimate/command/undo_macro_guard.hpp"
#include "glaxnimate/model/simple_visitor.hpp"

using namespace glaxnimate;

namespace {

/**
 * \returns The parent node for \p shape
 */
model::VisualNode* shape_parent(model::ShapeElement* shape)
{
    return static_cast<model::VisualNode*>(shape->owner()->object());
}

/**
 * \returns The parent node for \p shape
 */
model::VisualNode* shape_parent(model::VisualNode* shape)
{
    if ( auto se = qobject_cast<model::ShapeElement*>(shape) )
        return shape_parent(se);
    return nullptr;
}

/**
 * \brief Represents a sequence of nested nodes to reach
 */
struct PathToLayer
{
    PathToLayer() = default;

    explicit PathToLayer(model::VisualNode* node)
    {
        composition = nullptr;
        while ( node && !composition )
        {
            composition = qobject_cast<model::Composition*>(node);
            if ( composition )
                break;

            if ( auto group = qobject_cast<model::Group*>(node) )
            {
                steps.push_back(group);
                node = shape_parent(group);
            }
            else
            {
                return;
            }
        }
    }

    std::vector<model::Group*> steps;
    model::Composition* composition = nullptr;

    model::ShapeListProperty* lowest() const
    {
        if ( !steps.empty() )
            return &steps.front()->shapes;
        return &composition->shapes;
    }

    model::ShapeListProperty* combine(const PathToLayer& other)
    {
        if ( other.composition != composition )
            return nullptr;

        int i = 0;
        for ( int e = std::min(steps.size(), other.steps.size()); i < e; i++ )
            if ( steps[i] != other.steps[i] )
                break;

        if ( i < int(steps.size()) )
            steps.erase(steps.begin()+i, steps.end());

        return lowest();
    }
};

} // namespace

command::GroupShapes::Data command::GroupShapes::collect_shapes(const std::vector<model::VisualNode *>& selection)
{
    if ( selection.empty() )
        return {};

    Data data;
    PathToLayer collected;

    int i = 0;
    for ( ; i < int(selection.size()) && !data.parent; i++ )
    {
        collected = PathToLayer(shape_parent(selection[i]));
        data.parent = collected.lowest();
    }

    for ( ; i < int(selection.size()) && data.parent; i++ )
    {
        data.parent = collected.combine(PathToLayer(shape_parent(selection[i])));
        if ( !data.parent )
            return {};
    }

    data.elements.reserve(selection.size());
    for ( auto n : selection )
        data.elements.push_back(static_cast<model::ShapeElement*>(n));
    return data;
}

command::GroupShapes::GroupShapes(const command::GroupShapes::Data& data)
    : detail::RedoInCtor(i18n("Group Shapes"))
{
    if ( data.parent )
    {
        std::unique_ptr<model::Group> grp = std::make_unique<model::Group>(data.parent->object()->document());
        group = grp.get();
        data.parent->object()->document()->set_best_name(group);
        (new AddShape(data.parent, std::move(grp), data.parent->size(), this))->redo();

        for ( int i = 0; i < int(data.elements.size()); i++ )
        {
            (new MoveShape(data.elements[i], data.elements[i]->owner(), &group->shapes, i, this))->redo();
        }
    }
}

void command::detail::RedoInCtor::redo()
{
    if ( !did )
    {
        QUndoCommand::redo();
        did = true;
    }
}

void command::detail::RedoInCtor::undo()
{
    QUndoCommand::undo();
    did = false;
}


command::UngroupShapes::UngroupShapes(model::Group* group)
    : detail::RedoInCtor(i18n("Ungroup Shapes"))
{
    int pos = group->owner()->index_of(group);
    (new RemoveShape(group, group->owner(), this))->redo();
    for ( int i = 0, e = group->shapes.size(); i < e; i++ )
    {
        (new MoveShape(group->shapes[0], group->shapes[0]->owner(), group->owner(), pos+i, this))->redo();
    }
}


command::AddShape * command::duplicate_shape ( model::ShapeElement* shape )
{
    std::unique_ptr<model::ShapeElement> new_shape (
        static_cast<model::ShapeElement*>(shape->clone().release())
    );
    new_shape->refresh_uuid();
    new_shape->recursive_rename();
    new_shape->set_time(shape->docnode_parent()->time());

    return new command::AddShape(
        shape->owner(),
        std::move(new_shape),
        shape->owner()->index_of(shape)+1,
        nullptr,
        i18n("Duplicate %1", shape->object_name())
    );
}


void command::convert_to_path(const std::vector<model::ShapeElement*>& shapes, std::vector<model::ShapeElement*>* out)
{

    if ( shapes.empty() )
        return;

    QString macro_name = i18n("Convert to path");
    if ( shapes.size() == 1 )
        macro_name = i18n("Convert %1 to path", (*shapes.begin())->name.get());

    std::unordered_map<model::Layer*, model::Layer*> converted_layers;

    model::Document* doc = shapes[0]->document();
    command::UndoMacroGuard guard(macro_name, doc, false);
    for ( auto shape : shapes )
    {
        auto path = shape->to_path();

        if ( out )
            out->push_back(path.get());

        if ( path )
        {
            if ( auto lay = shape->cast<model::Layer>() )
                converted_layers[lay] = static_cast<model::Layer*>(path.get());

            guard.start();
            doc->push_command(
                new command::AddObject<model::ShapeElement>(
                    shape->owner(),
                    std::move(path),
                    shape->position()
                )
            );
            doc->push_command(
                new command::RemoveObject<model::ShapeElement>(shape, shape->owner())
            );
        }
    }

    // Maintain parenting of layers that have been converted
    for ( const auto& p : converted_layers )
    {
        if ( auto src_parent = p.first->parent.get() )
        {
            auto it = converted_layers.find(src_parent);
            if ( it != converted_layers.end() )
                p.second->parent.set(it->second);
        }
    }
}


void command::recursive_reverse_path(model::DocumentNode* node)
{
    command::UndoMacroGuard guard(i18n("Reverse path"), node->document());

    if ( auto shape = qobject_cast<model::Shape*>(node) )
    {
        shape->reversed.set_undoable(!shape->reversed.get());
    }

    for ( const auto& child : node->docnode_children() )
        recursive_reverse_path(child);
}



model::PreCompLayer* command::precompose(
    model::Composition* comp,
    const std::vector<model::VisualNode*>& objects,
    model::ObjectListProperty<model::ShapeElement>* layer_parent,
    int layer_index
)
{
    if ( objects.empty() )
        return nullptr;

    auto doc = comp->document();

    command::UndoMacroGuard guard(i18n("Precompose"), doc);

    auto ucomp = std::make_unique<model::Composition>(doc);
    model::Composition* new_comp = ucomp.get();
    new_comp->width.set(comp->width.get());
    new_comp->height.set(comp->height.get());
    new_comp->fps.set(comp->fps.get());
    new_comp->animation->first_frame.set(comp->animation->first_frame.get());
    new_comp->animation->last_frame.set(comp->animation->last_frame.get());
    if ( objects.size() > 1 || objects[0]->name.get().isEmpty() )
        doc->set_best_name(new_comp);
    else
        new_comp->name.set(objects[0]->name.get());
    doc->push_command(new command::AddObject(&doc->assets()->compositions->values, std::move(ucomp)));


    for ( auto node : objects )
    {
        if ( auto shape = node->cast<model::ShapeElement>() )
            doc->push_command(new command::MoveShape(
                shape, shape->owner(), &new_comp->shapes, new_comp->shapes.size()
            ));
    }

    auto pcl = std::make_unique<model::PreCompLayer>(doc);
    pcl->composition.set(new_comp);
    pcl->size.set(new_comp->size());
    doc->set_best_name(pcl.get());
    auto pcl_ptr = pcl.get();
    doc->push_command(new command::AddShape(layer_parent, std::move(pcl), layer_index));
    return pcl_ptr;
}

void command::trim_end_time(model::VisualNode *node, model::FrameTime last_frame)
{
    if ( auto comp = node->cast<model::Composition>() )
    {
        auto comp_last = comp->animation->last_frame.get();
        if ( last_frame == comp_last )
            return;

        command::UndoMacroGuard guard(i18n("Set End Time"), comp->document());
        comp->animation->last_frame.set_undoable(last_frame);

        if ( last_frame < comp_last )
        {
            // Trim
            model::simple_visit<model::Layer>(comp, true, [last_frame](model::Layer* layer){
                if ( layer->animation->last_frame.get() >= last_frame )
                    layer->animation->last_frame.set_undoable(last_frame);
            });
        }
        else
        {
            // Expand
            model::simple_visit<model::Layer>(comp, true, [last_frame, comp_last](model::Layer* layer){
                if ( layer->animation->last_frame.get() >= comp_last )
                    layer->animation->last_frame.set_undoable(last_frame);
            });
        }

    }
    else if ( auto layer = node->cast<model::Layer>() )
    {
        if ( last_frame == layer->animation->last_frame.get() )
            return;
        layer->animation->last_frame.set_undoable(last_frame);
    }
}

void command::trim_start_time(model::VisualNode *node, model::FrameTime start_time)
{
    if ( auto comp = node->cast<model::Composition>() )
    {
        auto comp_first = comp->animation->first_frame.get();
        if ( comp_first == start_time )
            return;

        command::UndoMacroGuard guard(i18n("Set Start Time"), comp->document());
        comp->animation->first_frame.set_undoable(start_time);

        if ( start_time > comp_first )
        {
            // Trim
            model::simple_visit<model::Layer>(comp, true, [start_time](model::Layer* layer){
                if ( layer->animation->first_frame.get() <= start_time )
                    layer->animation->first_frame.set_undoable(start_time);
            });
        }
        else
        {
            // Expand
            model::simple_visit<model::Layer>(comp, true, [start_time, comp_first](model::Layer* layer){
                if ( layer->animation->first_frame.get() <= comp_first )
                    layer->animation->first_frame.set_undoable(start_time);
            });
        }
    }
    else if ( auto layer = node->cast<model::Layer>() )
    {
        if ( start_time == layer->animation->first_frame.get() )
            return;
        layer->animation->first_frame.set_undoable(start_time);
    }
}
