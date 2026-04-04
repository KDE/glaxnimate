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


enum class Level
{
// Bit manipulation stuff
    Level1 = 1,
    Level2 = 3,
    Level3 = 7,
    LevelMask = 0x0f,

    Encapsulated = 0x00,
    NotEncapsulated = 0x10,
    EncapsulatedMask = 0xf0,

// Actual levels
    PS1 = Level1|NotEncapsulated,
    PS2 = Level2|NotEncapsulated,
    PS3 = Level3|NotEncapsulated,

    EPS1 = Level1|Encapsulated,
    EPS2 = Level2|Encapsulated,
    EPS3 = Level3|Encapsulated,
};

inline constexpr int level_number(Level level) { return (int(level) & 0xf) / 3 + 1; }
inline constexpr bool level_is_encapsulated(Level level) { return int(level) & int(Level::Encapsulated); }
QString level_string(Level level);

/**
 * @brief Checks if two PS levels are compatible, specifically if a command can be executed
 * @param allowed  Maximum level allowed
 * @param to_check Level that needs to conform
 * @return true if /p to_check is allowed within /p allowed restrictions
 */
inline bool level_is_compatible(Level allowed, Level to_check)
{
    return (int(allowed) & int(to_check)) == int(to_check);
}

struct Command
{
    Level level;
    std::vector<Value::Type> args;
    std::function<void(ValueArray, Interpreter&)> func;
};

class Interpreter
{
public:
    Interpreter();
    virtual ~Interpreter();

    ExecutionMemory &memory();
    Stack &stack();

    void execute(QIODevice* device);
    void execute(const Value& proc);

    void print(const QString& text);
    void error(const QString& error, bool critical);

    Value pop(Value::Type type);
    static Command* command_from_name(const QString& name);

    Level level() const;
    void set_level(Level level);

protected:
    virtual void on_print(const QString& text) = 0;
    virtual void on_error(const QString& text) = 0;
    virtual void on_comment(const QString& text) = 0;

private:
    void execute_command(const QString& name);
    Value procedure_value();

    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::ps
