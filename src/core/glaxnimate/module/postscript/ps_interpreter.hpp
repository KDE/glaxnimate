/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <random>

#include "ps_lexer.hpp"
#include "ps_stack.hpp"

namespace glaxnimate::ps {


struct ExecutionMemory
{
    Stack operand_stack;

    std::minstd_rand prng;
};

class Interpreter;

struct Command
{
    std::vector<Value::Type> args;
    std::function<void(ValueArray, Interpreter&)> func;
};

class Interpreter
{
public:
    Interpreter() : lexer(nullptr) {}
    virtual ~Interpreter() = default;

    ExecutionMemory& memory() { return memory_; }
    Stack& stack() { return memory_.operand_stack; }

    void execute(QIODevice* device);
    void execute(const Procedure& proc);

    void print(const QString& text);
    void error(const QString& error, bool critical);

    Value pop(Value::Type type);
    static Command* command_from_name(const QString& name);

protected:
    virtual void on_print(const QString& text) = 0;
    virtual void on_error(const QString& text) = 0;
    virtual void on_comment(const QString& text) = 0;

private:
    void run(const QString& name);

    Lexer lexer;
    ExecutionMemory memory_;
    bool has_failed = false;
    QString current_command;
};

} // namespace glaxnimate::ps
