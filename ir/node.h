#ifndef _IR_NODE_H_
#define _IR_NODE_H_

#include <memory>
#include "std.h"
#include "lib/cstring.h"
#include "lib/stringify.h"
#include "lib/indent.h"
#include "lib/source_file.h"
#include "ir-tree-macros.h"
#include "lib/log.h"

class Visitor;
class Inspector;
class Modifier;
class Transform;

namespace IR {

class Node;

// node interface
class INode : public Util::IHasSourceInfo, public Util::IHasDbPrint {
 public:
    virtual const Node* getNode() const = 0;
    virtual Node* getNode() = 0;
    virtual void dbprint(std::ostream &out) const = 0;  // for debugging
    virtual cstring toString() const = 0;  // for user consumption
    virtual cstring node_type_name() const = 0;
    virtual void validate() const {}
    template<typename T> bool is() const;
    template<typename T> const T* to() const;
};

class Node : public virtual INode {
 public:
    virtual bool apply_visitor_preorder(Modifier &v);
    virtual void apply_visitor_postorder(Modifier &v);
    virtual bool apply_visitor_preorder(Inspector &v) const;
    virtual void apply_visitor_postorder(Inspector &v) const;
    virtual const Node *apply_visitor_preorder(Transform &v);
    virtual const Node *apply_visitor_postorder(Transform &v);

 protected:
    static int currentId;
    void traceVisit(const char* visitor) const;
    virtual void visit_children(Visitor &) { }
    virtual void visit_children(Visitor &) const { }
    friend class ::Visitor;
    friend class ::Inspector;
    friend class ::Modifier;
    friend class ::Transform;

 public:
    Util::SourceInfo    srcInfo;
    int id;  // unique id for each node
    void traceCreation() const { LOG5("Created node " << id); }
    Node() : id(currentId++) { traceCreation(); }
    explicit Node(Util::SourceInfo si) : srcInfo(si), id(currentId++)
    { traceCreation(); }
    Node(const Node& other) : srcInfo(other.srcInfo), id(currentId++)
    { traceCreation(); }
    virtual ~Node() {}
    const Node *apply(Visitor &v) const;
    const Node *apply(Visitor &&v) const { return apply(v); }
    virtual Node *clone() const = 0;
    virtual void dbprint(std::ostream &out) const;
    virtual void dump_fields(std::ostream &) const { }
    virtual const Node* getNode() const { return this; }
    virtual Node* getNode() { return this; }
    Util::SourceInfo getSourceInfo() const override { return srcInfo; }
    cstring node_type_name() const override { return "Node"; }
    static cstring static_type_name() { return "Node"; }
    virtual int num_children() { return 0; }
    template<typename T> bool is() const { return to<T>() != nullptr; }
    template<typename T> const T* to() const {
        CHECK_NULL(this);
        return dynamic_cast<const T*>(this); }
    virtual cstring toString() const { return node_type_name(); }
    virtual bool operator==(const Node &n) const = 0;
    bool operator!=(const Node &n) const { return !operator==(n); }
};

template<typename T> bool INode::is() const { return getNode()->is<T>(); }
template<typename T> const T* INode::to() const { return getNode()->to<T>(); }

/* common things that ALL Node subclasses must define */
#define IRNODE_SUBCLASS(T)                                              \
 public:                                                                \
    T *clone() const override { return new T(*this); }                  \
    IRNODE_COMMON_SUBCLASS(T)

#define IRNODE_ABSTRACT_SUBCLASS(T)                                     \
 public:                                                                \
    T *clone() const override = 0;                                      \
    IRNODE_COMMON_SUBCLASS(T)
#define IRNODE_COMMON_SUBCLASS(T)                                       \
 public:                                                                \
    inline bool operator==(const Node &n) const override;               \
    bool apply_visitor_preorder(Modifier &v) override;                  \
    void apply_visitor_postorder(Modifier &v) override;                 \
    bool apply_visitor_preorder(Inspector &v) const override;           \
    void apply_visitor_postorder(Inspector &v) const override;          \
    const Node *apply_visitor_preorder(Transform &v) override;          \
    const Node *apply_visitor_postorder(Transform &v) override;         \

/* only define 'apply' for a limited number of classes (those we want to call
 * visitors directly on), as defining it and making it virtual would mean that
 * NO Transform could transform the class into a sibling class */
#define IRNODE_DECLARE_APPLY_OVERLOAD(T)                                \
    const T *apply(Visitor &v) const;                                   \
    const T *apply(Visitor &&v) const { return apply(v); }
#define IRNODE_DEFINE_APPLY_OVERLOAD(CLASS, TEMPLATE, TT)               \
    TEMPLATE                                                            \
    const IR::CLASS TT *IR::CLASS TT::apply(Visitor &v) const {         \
        const CLASS *tmp = this;                                        \
        auto prof = v.init_apply(tmp);                                  \
        v.visit(tmp);                                                   \
        return tmp; }

}  // namespace IR

#endif /* _IR_NODE_H_ */
