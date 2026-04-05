/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <deque>
#include "ps_value.hpp"

namespace glaxnimate::ps {

/**
 * @brief PostScript stack
 * @note Values are accessed from the top of the stack (first item is at the top)
 */
class Stack
{
public:
    using container = std::deque<Value>;
    using iterator = container::reverse_iterator;
    using const_iterator = container::const_reverse_iterator;
    using value_type = container::value_type;
    using reference = container::const_reference;

    bool empty() const { return stack.empty(); }
    int size() const { return stack.size(); }
    void clear() { stack.clear(); }

    void push(Value v)
    {
        stack.emplace_back(std::move(v));
    }

    Value pop()
    {
        Value v = stack.back();
        stack.pop_back();
        return v;
    }

    Value top()
    {
        return stack.back();
    }

    bool has(Value::Type type)
    {
        return !empty() && stack.back().type() == type;
    }

    iterator begin() { return stack.rbegin(); }
    iterator end() { return stack.rend(); }

    auto rbegin() { return stack.begin(); }
    auto rend() { return stack.end(); }

    Value& operator[](int i)
    {
        return *(begin() + i);
    }

private:
    container stack;
};

} // namespace glaxnimate::ps

