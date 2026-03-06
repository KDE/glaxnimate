/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QAbstractTableModel>
#include <QIcon>

#include "glaxnimate/log/logger.hpp"

namespace glaxnimate::gui {

class LogModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Columns
    {
        Time,
        Source,
        SourceDetail,
        Message,

        Count
    };

    LogModel();

    void populate(const std::vector<log::LogLine>& lines);

    int rowCount(const QModelIndex &) const override;

    int columnCount(const QModelIndex &) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    QVariant data(const QModelIndex & index, int role) const override;

private Q_SLOTS:
    void on_line(const log::LogLine& line);

private:
    std::vector<log::LogLine> lines;
};

} // namespace glaxnimate::gui
