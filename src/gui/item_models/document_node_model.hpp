#pragma once


#include <QAbstractItemModel>

#include "model/document.hpp"

namespace item_models {

template<class, class>
class ProxyBase;

class DocumentNodeModel : public QAbstractItemModel
{
public:
    enum ColumnTypes
    {
        ColumnColor = 0,
        ColumnName,
        ColumnVisible,
        ColumnLocked,
        ColumnUsers,

        ColumnCount
    };

    int rowCount ( const QModelIndex & parent ) const override;
    int columnCount ( const QModelIndex & parent ) const override;
    bool moveRows ( const QModelIndex & sourceParent, int sourceRow, int count, const QModelIndex & destinationParent, int destinationChild ) override;
    bool removeRows ( int row, int count, const QModelIndex & parent ) override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QVariant data ( const QModelIndex & index, int role ) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags ( const QModelIndex & index ) const override;
    QModelIndex parent ( const QModelIndex & child ) const override;
    bool setData ( const QModelIndex & index, const QVariant & value, int role ) override;

    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    Qt::DropActions supportedDropActions() const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    void clear_document();
    void set_document(model::Document* doc);
    model::DocumentNode* node(const QModelIndex& index) const;
    model::VisualNode* visual_node(const QModelIndex& index) const;
    QModelIndex node_index(model::DocumentNode* node) const;

private:
    void connect_node(model::DocumentNode* node);
    void disconnect_node(model::DocumentNode* node);

    model::Document* document = nullptr;

    template<class A, class B>
    friend class ProxyBase;
};


} // namespace item_models
