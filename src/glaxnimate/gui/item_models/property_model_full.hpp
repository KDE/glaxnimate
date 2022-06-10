#pragma once

#include "property_model_base.hpp"

namespace glaxnimate::gui::item_models {

class PropertyModelFull : public PropertyModelBase
{
    Q_OBJECT

public:
    enum Columns
    {
        ColumnName,
        ColumnValue,
        ColumnColor,
        ColumnVisible,
        ColumnLocked,

        ColumnCount

    };

    PropertyModelFull();

    bool setData(const QModelIndex & index, const QVariant & value, int role) override;
    QVariant data(const QModelIndex & index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex & index) const override;
    int columnCount(const QModelIndex & parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

protected:
    void on_document_reset() override;

    std::pair<model::VisualNode *, int> drop_position(const QModelIndex & parent, int row, int column) const override;

private:
    class Private;
    Private* dd() const;
};

} // namespace glaxnimate::gui::item_models

