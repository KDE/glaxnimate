/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#include <QUndoCommand>

#include "glaxnimate/model/shapes/shape.hpp"
#include "glaxnimate/command/object_list_commands.hpp"

namespace glaxnimate::model { class Group; class PreCompLayer; }

namespace glaxnimate::command {

using AddShape = AddObject<model::ShapeElement, model::ShapeListProperty>;
using RemoveShape = RemoveObject<model::ShapeElement, model::ShapeListProperty>;
using MoveShape = MoveObject<model::ShapeElement, model::ShapeListProperty>;

namespace detail {

class RedoInCtor : public QUndoCommand
{
public:
    void undo() override;
    void redo() override;

protected:
    using QUndoCommand::QUndoCommand;

private:
    bool did = true;
};

} // namespace detail

class GroupShapes : public detail::RedoInCtor
{
public:
    struct Data
    {
        std::vector<model::ShapeElement*> elements;
        model::ShapeListProperty* parent = nullptr;
    };

    GroupShapes(const Data& data);

    static Data collect_shapes(const std::vector<model::VisualNode *>& selection);


private:
    model::Group* group = nullptr;
};

class UngroupShapes : public detail::RedoInCtor
{
public:
    UngroupShapes(model::Group* group);

};

AddShape* duplicate_shape(model::ShapeElement* shape);

/**
 * \brief Converts \p shapes to path
 * \param out if not null, it will be populated with the new shapes
 * \returns The converted shapes
 */
void convert_to_path(const std::vector<model::ShapeElement*>& shapes, std::vector<model::ShapeElement*>* out = nullptr);

/**
 * \brief Recursively traverses the node and reverses the path direction of any shape
 */
void recursive_reverse_path(model::DocumentNode* node);

/**
 * \brief Precomposes the given objects into a new composition
 * \param source_comp Original composition containing the objects
 * \param objects Objects to move to the new composition
 * \param layer_parent Property to add the precomp layer to
 * \param layer_index Where in the property to insert the precomp layer to (-1 for default position)
 * \returns The created precomp layer
 * \pre All objects belong to \p source_comp
 * \pre layer_parent->object() not in \p objects
 * \post The returned layer is either null or it references the new composition
 */
model::PreCompLayer* precompose(
    model::Composition* source_comp,
    const std::vector<model::VisualNode*>& objects,
    model::ObjectListProperty<model::ShapeElement>* layer_parent,
    int layer_index
);

} // namespace glaxnimate::command
