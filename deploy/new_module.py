#!/usr/bin/env python3

import argparse
import pathlib

root = pathlib.Path(__file__).absolute().parent.parent

parser = argparse.ArgumentParser()
parser.add_argument("name")
args = parser.parse_args()

name_title = args.name
if name_title.islower():
    name_title = name_title.title()
name_lower = args.name.lower()

core = root / "src/core"

with open(core / "CMakeLists.txt", "r+") as cmake:
    all = cmake.read()
    insert = all.find("\n", all.rfind("glaxnimate_module"))

    new_row = '\nglaxnimate_module("%s" "" ON)' % name_title

    new_all = all[:insert] + new_row + all[insert:]
    cmake.seek(0)
    cmake.write(new_all)


module_path = core / "glaxnimate/module" / name_lower
module_path.mkdir(exist_ok=True)

with open(module_path / "CMakeLists.txt", "w") as cmake:
    cmake.write("""
# SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

set(SOURCES
    %(name_lower)s_module.cpp
)

add_library(Glaxnimate%(name_title)s OBJECT ${SOURCES})
kde_target_enable_exceptions(Glaxnimate%(name_title)s PUBLIC)
""".strip() % locals())



with open(module_path / (name_lower + "_module.hpp"), "w") as src:
    src.write("""
/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "glaxnimate/module/module.hpp"

namespace glaxnimate::%(name_lower)s {

class Module : public module::Module
{
public:
    Module() : module::Module(i18n("%(name_title)s")) {}

    std::vector<module::ExternalComponent> components() const override;

protected:
    void initialize() override;
};

} // namespace glaxnimate::%(name_lower)s
""".strip() % locals())



with open(module_path / (name_lower + "_module.cpp"), "w") as src:
    src.write("""
/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "%(name_lower)s_module.hpp"

using namespace glaxnimate::%(name_lower)s;

std::vector<glaxnimate::module::ExternalComponent> glaxnimate::%(name_lower)s::Module::components() const
{
    return {
    };
}

void glaxnimate::%(name_lower)s::Module::initialize()
{
    register_io_classes<>();
}
""".strip() % locals())
