/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>
#include <QUndoCommand>

#include "model/document_node.hpp"
#include "model/shapes/shape.hpp"

namespace glaxnimate::command {

class DeferredCommandBase : public QUndoCommand
{
public:
    ~DeferredCommandBase();

    void undo() override;
    void redo() override;

    bool has_action() const;

protected:
    DeferredCommandBase(const QString& name, std::unique_ptr<QUndoCommand> d = {})
    : QUndoCommand(name), d(std::move(d))
    {}

    std::unique_ptr<QUndoCommand> d;
};

class ReorderCommand : public DeferredCommandBase
{
public:
    enum SpecialPosition
    {
        MoveUp = -1,
        MoveDown = -2,
        MoveTop = -3,
        MoveBottom = -4,
    };

    static bool resolve_position(model::ShapeElement* node, int& position);

    ReorderCommand(model::ShapeElement* node, int new_position);

private:
    static QString name(model::DocumentNode* node);
};

} // namespace glaxnimate::command
