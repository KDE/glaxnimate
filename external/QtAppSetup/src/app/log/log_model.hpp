/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QAbstractTableModel>
#include <QIcon>

#include "app/log/logger.hpp"

namespace app::log {

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

    void populate(const std::vector<LogLine>& lines);

    int rowCount(const QModelIndex &) const override;

    int columnCount(const QModelIndex &) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    QVariant data(const QModelIndex & index, int role) const override;

private Q_SLOTS:
    void on_line(const LogLine& line);

private:
    std::vector<LogLine> lines;
};

} // namespace app::log
