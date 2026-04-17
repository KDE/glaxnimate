/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <random>

#include "ps_stack.hpp"
#include "ps_gstate.hpp"

namespace glaxnimate::ps {

class Interpreter;

enum LevelBits
{
// Bit manipulation stuff
    Level1 = 1,
    Level2 = 3,
    Level3 = 7,
    LevelMask = 0x0f,

    Encapsulated = 0x00,
    NotEncapsulated = 0x10,
    EncapsulatedMask = 0xf0,
};

enum class Level
{
// Actual levels
    PS1 = Level1|NotEncapsulated,
    PS2 = Level2|NotEncapsulated,
    PS3 = Level3|NotEncapsulated,

    EPS1 = Level1|Encapsulated,
    EPS2 = Level2|Encapsulated,
    EPS3 = Level3|Encapsulated,
};

inline constexpr int level_number(Level level) { return (int(level) & 0xf) / 3 + 1; }
inline constexpr bool level_is_encapsulated(Level level) { return int(level) & LevelBits::Encapsulated; }
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

struct ArgumentType
{
    ArgumentType() {}
    ArgumentType(Value::Type t, int attrs = 0) : overloads{t}, attrs(attrs) {}
    ArgumentType(std::initializer_list<Value::Type> t) : overloads(t) {}

    static ArgumentType proc() { return ArgumentType(Value::Array, Value::Executable); }
    static ArgumentType any() { return {}; }
    static ArgumentType number() { return {Value::Integer, Value::Real}; }

    bool matches(const Value& val) const
    {
        if ( !val.has_attribute(attrs) )
            return false;
        return overloads.empty() || std::find(overloads.begin(), overloads.end(), val.type()) != overloads.end();
    }

    QString to_string() const;

    std::vector<Value::Type> overloads;
    int attrs = 0;
};

struct Command
{
    Level level;
    std::vector<ArgumentType> arg_types;
    std::function<void(ValueArray, Interpreter&)> func;
    bool needs_point = false;

    bool collect_arguments(Stack& stack, std::vector<std::pair<int, int>>& errors, ValueArray& args) const;
};

class CommandSet
{
public:
    void def(QByteArray key, Command cmd);
    /**
     * @brief Creates an alias
     * @param key New command
     * @param other Existing command
     */
    void alias(QByteArray key, QByteArray other);

    static const CommandSet& builtins();

    auto find(const QByteArray& key) const { return commands.equal_range(key); }

    void populate_dict(ValueDict& value) const;

private:
    static void populate_builtins(CommandSet& builtins);
    std::unordered_multimap<QByteArray, glaxnimate::ps::Command> commands;
};



struct ExecutionMemory
{
    Stack operand_stack;

    std::minstd_rand prng;

    const CommandSet* builtins = nullptr;

    ValueDict userdict;
    ValueDict globaldict;
    ValueDict systemdict;
    std::deque<ValueDict> dict_stack;

    GraphicsState gstate;
    std::deque<GraphicsState> gstate_stack;

    ValueDict* current_dict()
    {
        if ( dict_stack.empty() )
            return &userdict;
        return &dict_stack.back();
    }

    ValueDict& loaded_systemdict();

    /**
     * @brief Helper for `load` operator
     * @param key Key to search in the dicts
     * @param out Value to be written to
     * @return true on success
     */
    bool load(const ValueDict::key_type &key, Value& out, bool search_system);

    /**
     * @brief Helper for `store` operator
     * @param key Key to search in the dicts
     * @param out New value
     * @return true on success, will only fail if key is in systemdict
     */
    bool store(const ValueDict::key_type &key, const Value& val);

    /**
     * @brief Returns the dictionary with the given key (or nullptr)
     */
    ValueDict* where(const ValueDict::key_type& key);
};

class Interpreter
{
public:
    Interpreter();
    virtual ~Interpreter();

    ExecutionMemory &memory();
    Stack &stack();

    void execute(QIODevice* device, bool reset_pos = false);
    void execute(Value proc);

    void print(const QString& text);
    void error(const QString& error);
    void warning(const QString& error);

    Value pop(Value::Type type);

    Level level() const;
    void set_level(Level level);
    void set_level_autodetect(bool enable);

    std::map<QString, QString>& document_metadata();
    std::map<QString, QString>& page_metadata();

    /**
     * @brief Whether execution is halted (globally)
     */
    bool is_halted() const;
    void halt();
    /**
     * @brief Whether a procedure has been stopped
     */
    bool is_stopped() const;
    void set_stopped(bool stopped);
    /**
     * @brief Whether a procedure must stop execution
     */
    bool procedure_must_exit() const;
    /**
     * @brief Whether a loop operation must exit
     * @param count Current iteration to impose a max limit on iterations
     */
    bool loop_must_exit(int count);
    /**
     * @brief Signals the interpreter the current loop must exit
     * @post loop_must_exit() == true
     */
    void loop_exit();


    int file_row() const;
    int file_column() const;
    const QByteArray& current_command();

    void fill();

protected:
    virtual void on_print(const QString& text) = 0;
    virtual void on_error(const QString& text) = 0;
    virtual void on_warning(const QString& text) = 0;
    virtual void on_comment(const QString& text) = 0;
    virtual void on_fill(const GraphicsState& gstate) = 0;

private:
    void execute_command(const Value &name);

    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::ps
QDebug operator<<(QDebug d, glaxnimate::ps::Interpreter& interp);
