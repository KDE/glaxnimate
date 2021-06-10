#pragma once

#include <QList>
#include <QUuid>
#include <QIcon>

#include "model/animation/animatable.hpp"
#include "model/object.hpp"

namespace model {

class Document;
class ReferencePropertyBase;

/**
 * \brief Base class for elements of the document tree, that need to show in the tree view etc.
 */
class DocumentNode : public Object
{
    Q_OBJECT

public:
    /**
     * @brief Unique identifier for the node
     */
    GLAXNIMATE_PROPERTY_RO(QUuid, uuid, {})
    /**
     * @brief Name of the node, used to display it in the UI
     */
    GLAXNIMATE_PROPERTY(QString, name, "", &DocumentNode::name_changed)

protected:
    template<class Base>
    class ChildRange
    {
    public:

        using get_func_t = Base* (Base::*) (int) const;
        using count_func_t = int (Base::*) () const;

        class ChildIterator
        {
        public:
            using value_type = Base*;
            using reference = value_type*;
            using pointer = value_type*;
            using difference_type = int;
            using iterator_category = std::forward_iterator_tag;

            ChildIterator& operator++() noexcept { ++index; return *this; }
            Base* operator->() const { return (parent->*get_func)(index); }
            Base* operator*() const { return (parent->*get_func)(index); }
            bool operator==(const ChildIterator& oth) const noexcept
            {
                return parent == oth.parent && index == oth.index;
            }
            bool operator!=(const ChildIterator& oth) const noexcept
            {
                return !(*this == oth );
            }

        private:
            ChildIterator(const Base* parent, int index, get_func_t get_func) noexcept
            : parent(parent), index(index), get_func(get_func) {}

            const Base* parent;
            int index;
            get_func_t get_func;
            friend ChildRange;
        };

        ChildIterator begin() const noexcept { return ChildIterator{parent, 0, get_func}; }
        ChildIterator end() const noexcept { return ChildIterator{parent, size(), get_func}; }
        int size() const { return (parent->*count_func)(); }

    private:
        ChildRange(const Base* parent, get_func_t get_func, count_func_t count_func) noexcept
        : parent(parent), get_func(get_func), count_func(count_func) {}
        const Base* parent;
        get_func_t get_func;
        count_func_t count_func;
        friend Base;
    };

public:
    explicit DocumentNode(model::Document* document);

    virtual QIcon tree_icon() const = 0;

    virtual DocumentNode* docnode_parent() const { return list_parent; }
    virtual int docnode_child_count() const = 0;
    virtual DocumentNode* docnode_child(int index) const = 0;
    virtual int docnode_child_index(DocumentNode* dn) const = 0;

    virtual QIcon instance_icon() const = 0;

    ChildRange<DocumentNode> docnode_children() const noexcept
    {
        return ChildRange<DocumentNode>{this, &DocumentNode::docnode_child, &DocumentNode::docnode_child_count};
    }

    template<class T=DocumentNode>
    T* docnode_find_by_uuid(const QUuid& uuid)
    {
        if ( this->uuid.get() == uuid && qobject_cast<T*>(this) )
            return this;
        for ( DocumentNode* child : docnode_children() )
            if ( auto found = child->docnode_find_by_uuid<T>(uuid) )
                return found;
        return nullptr;
    }

    template<class T=DocumentNode>
    T* docnode_find_by_name(const QString& name)
    {
        if ( this->name.get() == name && qobject_cast<T*>(this) )
            return this;
        for ( DocumentNode* child : docnode_children() )
            if ( auto found = child->docnode_find_by_name<T>(name) )
                return found;
        return nullptr;
    }

    template<class T=DocumentNode>
    std::vector<T*> docnode_find_by_type_name(const QString& type_name)
    {
        std::vector<T*> matches;
        docnode_find_impl(type_name, matches);
        return matches;
    }

    bool docnode_is_instance(const QString& type_name) const;

    Q_INVOKABLE model::DocumentNode* find_by_name(const QString& name) { return docnode_find_by_name(name); }
    Q_INVOKABLE model::DocumentNode* find_by_uuid(const QUuid& uuid) { return docnode_find_by_uuid(uuid); }
    Q_INVOKABLE QVariantList find_by_type_name(const QString& type_name)
    {
        auto ob = docnode_find_by_type_name(type_name);
        QVariantList ret;
        ret.reserve(ob.size());
        for ( auto o : ob )
            ret.push_back(QVariant::fromValue(o));
        return ret;
    }

    /**
     * \brief Updates the name of this node and all of its children
     *        using the "best name" document functions
     */
    void recursive_rename();

    /**
     * \brief Recursively updates uuid
     */
    void refresh_uuid();

    QString object_name() const override;

private:
    template<class T=DocumentNode>
    void docnode_find_impl(const QString& type_name, std::vector<T*>& matches)
    {
        if ( docnode_is_instance(type_name) )
            if ( auto obj = qobject_cast<T*>(this) )
                matches.push_back(obj);

        for ( DocumentNode* child : docnode_children() )
            child->docnode_find_impl<T>(type_name, matches);
    }


signals:
    void docnode_child_add_begin(int row);
    void docnode_child_add_end(DocumentNode* node, int row);

    void docnode_child_remove_begin(int row);
    void docnode_child_remove_end(DocumentNode* node, int row);

    void docnode_child_move_begin(int from, int to);
    void docnode_child_move_end(DocumentNode* node, int from, int to);

    void name_changed(const QString&);

private:
    friend class ObjectListPropertyBase;
    DocumentNode* list_parent = nullptr;
};

/**
 * \brief A document node that has a physical size and shape in the document
 */
class VisualNode : public DocumentNode
{
    Q_OBJECT

    /**
     * @brief Color of the node the tree UI to highlight grouped items
     *
     * Generally parent/child relationshitps define groups but layers can
     * be grouped with each other even if they are children of a composition
     */
    GLAXNIMATE_PROPERTY(QColor, group_color, QColor(0, 0, 0, 0), &VisualNode::on_group_color_changed)
    /**
     * \brief Visible setting for this node
     */
    GLAXNIMATE_PROPERTY(bool, visible, true, &VisualNode::on_visible_changed, {}, PropertyTraits::Visual|PropertyTraits::Hidden)
    /**
     * \brief Locked setting for this node
     */
    GLAXNIMATE_PROPERTY(bool, locked, false, &VisualNode::docnode_locked_changed)

    Q_PROPERTY(bool visible_recursive READ docnode_visible_recursive)
    Q_PROPERTY(bool locked_recursive READ docnode_locked_recursive)
    Q_PROPERTY(bool selectable READ docnode_selectable)

public:
    enum PaintMode
    {
        Canvas,         ///< Paint everything
        Render          ///< Recursive, but hide objects maked with render == false
    };

    using DocumentNode::DocumentNode;

    QColor docnode_group_color() const;

    virtual VisualNode* docnode_group_parent() const;
    virtual int docnode_group_child_count() const;
    virtual VisualNode* docnode_group_child(int index) const;
    VisualNode* docnode_fuzzy_parent() const;
    VisualNode* docnode_visual_child(int index) const;
    VisualNode* docnode_visual_parent() const;

    ChildRange<VisualNode> docnode_visual_children() const noexcept
    {
        return ChildRange<VisualNode>{this, &VisualNode::docnode_visual_child, &VisualNode::docnode_child_count};
    }

    ChildRange<VisualNode> docnode_group_children() const noexcept
    {
        return ChildRange<VisualNode>{this, &VisualNode::docnode_group_child, &VisualNode::docnode_group_child_count};
    }

    /**
     * \brief Bounding rect in local coordinates (current frame)
     */
    virtual QRectF local_bounding_rect(FrameTime t) const = 0;

    /**
     * \brief \b true iff this node and all of its ancestors are visible and unlocked
     */
    bool docnode_selectable() const;
    /**
     * \brief \b true iff this node and all of its ancestors are visible
     */
    bool docnode_visible_recursive() const;
    /**
     * \brief \b true iff this node or any of its ancestors is locked
     */
    bool docnode_locked_recursive() const;

    /**
     * \brief Transform matrix mapping points from document coordinates to local coordinates
     */
    QTransform transform_matrix(FrameTime t) const;
    QTransform group_transform_matrix(FrameTime t) const;
    /**
     * \brief Transform matrix mapping points from parent coordinates to local coordinates
     */
    virtual QTransform local_transform_matrix(FrameTime) const { return QTransform(); }


    virtual void paint(QPainter* painter, FrameTime time, PaintMode mode) const;

    QIcon instance_icon() const override;

signals:
    void docnode_visible_changed(bool);
    void docnode_locked_changed(bool);
    void docnode_visible_recursive_changed(bool);
    void docnode_group_color_changed(const QColor&);

    void bounding_rect_changed();
    void transform_matrix_changed(const QTransform& t);
    void group_transform_matrix_changed(const QTransform& t);
    void local_transform_matrix_changed(const QTransform& t);

private:
    void propagate_visible(bool visible);

    void on_group_color_changed(const QColor& color);

protected:
    void docnode_on_update_group(bool force = false);
    bool docnode_valid_color() const;
    void propagate_transform_matrix_changed(const QTransform& t_global, const QTransform& t_group);
    virtual void on_paint(QPainter*, FrameTime, PaintMode) const {}

private:
    void on_visible_changed(bool visible);

    mutable QPixmap group_icon;
};

} // namespace model

Q_DECLARE_METATYPE(std::vector<model::DocumentNode*>)
