/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "python_registrar.hpp"



using namespace glaxnimate::plugin::python;

std::string glaxnimate::plugin::python::fix_type(QByteArray ba)
{
    if ( ba.endsWith('*') || ba.endsWith('&') )
        ba.remove(ba.size()-1, 1);

    if ( ba.startsWith("const ") )
        ba.remove(0, 6);

    if ( ba == "QString" )
        return "str";
    else if ( ba == "QVariantList" )
        return "list";
    if ( ba == "QStringList" )
        return "List[str]";
    else if ( ba == "double" )
        return "float";
    else if ( ba == "void" )
        return "None";
    else if ( ba == "QImage" )
        return "PIL.Image.Image";

    return ba.toStdString();
}
