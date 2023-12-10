/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QUndoCommand>

#include "model/property/object_list_property.hpp"

namespace glaxnimate::command {

template<class ItemT, class PropT = model::ObjectListProperty<ItemT>>
class AddObject : public QUndoCommand
{
public:
    AddObject(
        PropT* object_parent,
        std::unique_ptr<ItemT> object,
        int position = -1,
        QUndoCommand* parent = nullptr,
        const QString& name = {}
    )
        : QUndoCommand(name.isEmpty() ? i18n("Create %1", object->object_name()) : name, parent),
          object_parent(object_parent),
          object_(std::move(object)),
          position(position == -1 ? object_parent->size() : position)
    {}

    void undo() override
    {
        object_ = object_parent->remove(position);
    }

    void redo() override
    {
        object_parent->insert(std::move(object_), position);
    }

    ItemT* object() const
    {
        if ( object_ )
            return object_.get();
        return (*object_parent)[position];
    }

private:
    PropT* object_parent;
    std::unique_ptr<ItemT> object_;
    int position;
};


template<class ItemT, class PropT = model::ObjectListProperty<ItemT>>
class RemoveObject : public QUndoCommand
{
public:
    RemoveObject(ItemT* object, PropT* object_parent, QUndoCommand* parent = nullptr)
        : QUndoCommand(i18n("Remove %1", object->object_name()), parent),
          object_parent(object_parent),
          position(object_parent->index_of(object, -1))
    {}

    RemoveObject(int index, PropT* object_parent, QUndoCommand* parent = nullptr)
        : QUndoCommand(i18n("Remove %1", (*object_parent)[index]->object_name()), parent),
          object_parent(object_parent),
          position(index)
    {}


    void undo() override
    {
        object_parent->insert(std::move(object), position);
    }

    void redo() override
    {
        object = object_parent->remove(position);
    }

private:
    PropT* object_parent;
    std::unique_ptr<ItemT> object;
    int position;
};


template<class ItemT, class PropT = model::ObjectListProperty<ItemT>>
class MoveObject : public QUndoCommand
{
public:
    MoveObject(
        ItemT* object,
        PropT* parent_before,
        PropT* parent_after,
        int position_after,
        QUndoCommand* parent = nullptr
    )
        : QUndoCommand(i18n("Move Object"), parent),
          parent_before(parent_before),
          position_before(parent_before->index_of(object, -1)),
          parent_after(parent_after),
          position_after(position_after)
    {}

    void undo() override
    {
        if ( parent_before == parent_after )
            parent_before->move(position_before, position_after);
        else if ( auto object = parent_after->remove(position_after) )
            parent_before->insert(std::move(object), position_before);
    }

    void redo() override
    {
        if ( parent_before == parent_after )
            parent_before->move(position_before, position_after);
        else if ( auto object = parent_before->remove(position_before) )
            parent_after->insert(std::move(object), position_after);
    }

private:
    PropT* parent_before;
    int position_before;
    PropT* parent_after;
    int position_after;
};


} // namespace glaxnimate::command
