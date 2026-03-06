/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "log_model.hpp"

namespace {
    enum Columns
    {
        Time,
        Source,
        SourceDetail,
        Message,

        Count
    };
} // namespace

glaxnimate::gui::LogModel::LogModel()
{
    connect(&log::Logger::instance(), &log::Logger::logged, this, &LogModel::on_line);
}

int glaxnimate::gui::LogModel::rowCount(const QModelIndex &) const
{
    return lines.size();
}

int glaxnimate::gui::LogModel::columnCount(const QModelIndex &) const
{
    return Count;
}

QVariant glaxnimate::gui::LogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ( orientation == Qt::Horizontal )
    {
        if ( role == Qt::DisplayRole )
        {
            switch ( section )
            {
                case Time:
                    return tr("Time");
                case Source:
                    return tr("Source");
                case SourceDetail:
                    return tr("Details");
                case Message:
                    return tr("Message");
            }
        }
    }
    else
    {
        if ( role == Qt::DecorationRole )
        {
            switch ( lines[section].severity )
            {
                case log::Info: return QIcon::fromTheme("emblem-information");
                case log::Warning: return QIcon::fromTheme("emblem-warning");
                case log::Error: return QIcon::fromTheme("emblem-error");
            }
        }
        else if ( role == Qt::ToolTipRole )
        {
            return log::Logger::severity_name(lines[section].severity);
        }
    }

    return {};
}

QVariant glaxnimate::gui::LogModel::data(const QModelIndex & index, int role) const
{
    if ( !index.isValid() )
        return {};

    const log::LogLine& line = lines[index.row()];
    if ( role == Qt::DisplayRole )
    {
        switch ( index.column() )
        {
            case Time:
                return line.time.toString(Qt::ISODate);
            case Source:
                return line.source;
            case SourceDetail:
                return line.source_detail;
            case Message:
                return line.message;
        }
    }
    else if ( role == Qt::ToolTipRole )
    {
        switch ( index.column() )
        {
            case Time:
                return line.time.toString();
            case SourceDetail:
                return line.source_detail;
        }
    }

    return {};
}

void glaxnimate::gui::LogModel::on_line(const log::LogLine& line)
{
    beginInsertRows(QModelIndex(), lines.size(), lines.size());
    lines.push_back(line);
    endInsertRows();
}

void glaxnimate::gui::LogModel::populate(const std::vector<log::LogLine>& lines)
{
    beginResetModel();
    this->lines = lines;
    endResetModel();
}
