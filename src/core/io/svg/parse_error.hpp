/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QString>
#include <QDomDocument>
#include <exception>

namespace glaxnimate::io::svg {

class SvgParseError : public std::exception
{
public:
    SvgParseError() {}

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    SvgParseError(const QDomDocument::ParseResult& result)
    : message(result.errorMessage),
        line(result.errorLine),
        column(result.errorColumn)
    {}
#endif

    QString formatted(const QString& filename) const
    {
        return QString("%1:%2:%3: XML Parse Error: %4")
            .arg(filename)
            .arg(line)
            .arg(column)
            .arg(message)
        ;
    }

    explicit operator bool() const
    {
        return line != -1;
    }

    QString message;
    int line = -1;
    int column = -1;

};

} // namespace glaxnimate::io::svg
