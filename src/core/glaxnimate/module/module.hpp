/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>
#include <vector>

#include <QString>

#include "glaxnimate/utils/iterator.hpp"
#include "glaxnimate/utils/i18n.hpp"
#include "glaxnimate/io/io_registry.hpp"

namespace glaxnimate::module {

/**
 * @brief Data about external components used by a module
 */
struct ExternalComponent
{
    QString name;
    QString description;
    QString version;
    QString website;
    const char* license;
};

class Registry;

/**
 * @brief Base class for optional modules
 */
class Module
{
public:
    virtual ~Module() {}

    virtual std::vector<ExternalComponent> components() const { return {}; }

    const QString& name() const { return name_; }
    const QString& version() const { return version_; }

protected:
    Module(const QString& name, const QString& version = {});

    template<class... Classes>
    void register_io_classes()
    {
        (glaxnimate::io::IoRegistry::instance().register_class<Classes>(), ...);
    }


    /**
     * @brief Register everything that is needed in here
     */
    virtual void initialize() = 0;
    friend Registry;
    QString name_;
    QString version_;
};


class Registry
{
public:
    using container = std::vector<std::unique_ptr<Module>>;

public:
    class iterator : public utils::RandomAccessIteratorWrapper<iterator, container::const_iterator>
    {
    public:
        const Module* operator->() const { return iter->get(); }
        const Module& operator*() const { return **iter; }

    private:
        iterator(InternalIterator it = {}) : Parent(it) {}
        friend Registry;
    };

    static Registry& instance()
    {
        static Registry instance;
        return instance;
    }

    template<class ModuleT, class... Args>
    void install(Args&&... args)
    {
        modules.push_back(std::make_unique<ModuleT>(std::forward<Args>(args)...));
        modules.back()->initialize();
    }

    iterator begin() const { return modules.begin(); }
    iterator end() const { return modules.end(); }

private:
    Registry();
    static void register_loaded_modules(Registry& reg);

    container modules;
};

/**
 * @brief Returns the module registry
 * @note Calling this initializes the default modules
 */
Registry& registry();

/**
 * @brief Initializes the default modules
 */
void initialize();


} // namespace glaxnimate::module
