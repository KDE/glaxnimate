/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QString>
#include <QDateTime>
#include <QMetaType>

namespace glaxnimate::log {

enum Severity
{
    Info,
    Warning,
    Error,
};

struct LogLine
{
    Severity severity;
    QString source;
    QString source_detail;
    QString message;
    QDateTime time;
};


} // namespace glaxnimate::log

Q_DECLARE_METATYPE(glaxnimate::log::LogLine)
Q_DECLARE_METATYPE(glaxnimate::log::Severity)
