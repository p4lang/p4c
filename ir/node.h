/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef IR_NODE_H_
#define IR_NODE_H_

#include <iosfwd>

#include "ir/copy_on_write_ptr.h"
#include "ir/gen-tree-macro.h"
#include "ir/inode.h"
#include "ir/ir-tree-macros.h"
#include "lib/cstring.h"
#include "lib/source_file.h"

namespace P4 {
class Visitor;
struct Visitor_Context;
class Inspector;
class Modifier;
class COWModifier;
class Transform;
class COWTransform;
class JSONGenerator;
class JSONLoader;
}  // namespace P4

namespace P4::Util {
class JsonObject;
}  // namespace P4::Util

namespace P4::IR {

using namespace P4::literals;

class Node;
class Annotation;  // IWYU pragma: keep
template <class T>
class Vector;  // IWYU pragma: keep
template <class T>
class IndexedVector;  // IWYU pragma: keep

class Node : public virtual INode {
 public:
    virtual bool apply_visitor_preorder(Modifier &v);
    virtual void apply_visitor_postorder(Modifier &v);
    virtual void apply_visitor_revisit(Modifier &v, const Node *n) const;
    virtual void apply_visitor_loop_revisit(Modifier &v) const;
    virtual bool apply_visitor_preorder(COWNode_info *, COWModifier &v) const;
    virtual void apply_visitor_postorder(COWNode_info *, COWModifier &v) const;
    virtual void apply_visitor_revisit(COWModifier &v, const Node *n) const;
    virtual void apply_visitor_loop_revisit(COWModifier &v) const;
    virtual bool apply_visitor_preorder(Inspector &v) const;
    virtual void apply_visitor_postorder(Inspector &v) const;
    virtual void apply_visitor_revisit(Inspector &v) const;
    virtual void apply_visitor_loop_revisit(Inspector &v) const;
    virtual const Node *apply_visitor_preorder(Transform &v);
    virtual const Node *apply_visitor_postorder(Transform &v);
    virtual void apply_visitor_revisit(Transform &v, const Node *n) const;
    virtual void apply_visitor_loop_revisit(Transform &v) const;
    virtual const Node *apply_visitor_preorder(COWNode_info *, COWTransform &v) const;
    virtual const Node *apply_visitor_postorder(COWNode_info *, COWTransform &v) const;
    virtual void apply_visitor_revisit(COWTransform &v, const Node *n) const;
    virtual void apply_visitor_loop_revisit(COWTransform &v) const;
    Node &operator=(const Node &) = default;
    Node &operator=(Node &&) = default;

 protected:
    static int currentId;
    void traceVisit(const char *visitor) const;
    friend class ::P4::Visitor;
    friend class ::P4::Inspector;
    friend class ::P4::Modifier;
    friend class ::P4::COWModifier;
    friend class ::P4::Transform;
    friend class ::P4::COWTransform;
    cstring prepareSourceInfoForJSON(Util::SourceInfo &si, unsigned *lineNumber,
                                     unsigned *columnNumber) const;

 public:
    Util::SourceInfo srcInfo;
    int id;        // unique id for each node
    int clone_id;  // unique id this node was cloned from (recursively)
    void traceCreation() const;
    Node() : id(currentId++), clone_id(id) { traceCreation(); }
    explicit Node(Util::SourceInfo si) : srcInfo(si), id(currentId++), clone_id(id) {
        traceCreation();
    }
    Node(const Node &other) : srcInfo(other.srcInfo), id(currentId++), clone_id(other.clone_id) {
        traceCreation();
    }
    virtual ~Node() {}
    const Node *apply(Visitor &v, const Visitor_Context *ctxt = nullptr) const;
    const Node *apply(Visitor &&v, const Visitor_Context *ctxt = nullptr) const {
        return apply(v, ctxt);
    }
    virtual Node *clone() const = 0;
    void dbprint(std::ostream &out) const override;
    virtual void dump_fields(std::ostream &) const {}
    const Node *getNode() const final { return this; }
    Node *getNode() final { return this; }
    Util::SourceInfo getSourceInfo() const override { return srcInfo; }
    cstring node_type_name() const override { return "Node"_cs; }
    static cstring static_type_name() { return "Node"_cs; }
    virtual int num_children() { return 0; }
    explicit Node(JSONLoader &json);
    cstring toString() const override { return node_type_name(); }
    void toJSON(JSONGenerator &json) const override;
    void sourceInfoToJSON(JSONGenerator &json) const;
    void sourceInfoFromJSON(JSONLoader &json);
    Util::JsonObject *sourceInfoJsonObj() const;
    /* operator== does a 'shallow' comparison, comparing two Node subclass objects for equality,
     * and comparing pointers in the Node directly for equality */
    virtual bool operator==(const Node &a) const { return this->typeId() == a.typeId(); }
    /* 'equiv' does a deep-equals comparison, comparing all non-pointer fields and recursing
     * though all Node subclass pointers to compare them with 'equiv' as well. */
    virtual bool equiv(const Node &a) const { return this->typeId() == a.typeId(); }
#define DEFINE_OPEQ_FUNC(CLASS, BASE) \
    virtual bool operator==(const CLASS &) const { return false; }
    IRNODE_ALL_SUBCLASSES(DEFINE_OPEQ_FUNC)
#undef DEFINE_OPEQ_FUNC
    virtual void visit_children(Visitor &, const char * /*name*/ = nullptr) {}
    virtual void visit_children(Visitor &, const char * /*name*/ = nullptr) const {}
    virtual void COWref_visit_children(COWNode_info *, Visitor &, const char *) const {}

    bool operator!=(const Node &n) const { return !operator==(n); }

    /// Helper to simplify usage of nodes in Abseil functions (e.g. StrCat / StrFormat, etc.)
    /// without explicit string_view conversion. Note that this calls Node::toString() so might
    /// not be always appropriate for user-visible messages / strings.
    template <typename Sink>
    friend void AbslStringify(Sink &sink, const IR::Node *n) {
        sink.Append(n->toString());
    }

    union COWref {
        COWfieldref<Node, Util::SourceInfo, &Node::srcInfo> srcInfo;
        COWref(COWNode_info *i) { _info = i; }
        COWref *operator->() { return this; }
        void visit_children(Visitor &, const char * /*name*/ = nullptr) {}

     private:
        COWNode_info *_info;
    };

    DECLARE_TYPEINFO_WITH_TYPEID(Node, NodeKind::Node, INode);
};

// simple version of dbprint
cstring dbp(const INode *node);

inline bool equal(const Node *a, const Node *b) { return a == b || (a && b && *a == *b); }
inline bool equal(const INode *a, const INode *b) {
    return a == b || (a && b && *a->getNode() == *b->getNode());
}
inline bool equiv(const Node *a, const Node *b) { return a == b || (a && b && a->equiv(*b)); }
inline bool equiv(const INode *a, const INode *b) {
    return a == b || (a && b && a->getNode()->equiv(*b->getNode()));
}
// NOLINTBEGIN(bugprone-macro-parentheses)
/* common things that ALL Node subclasses must define */
#define IRNODE_SUBCLASS(T)                             \
 public:                                               \
    T *clone() const override { return new T(*this); } \
    IRNODE_COMMON_SUBCLASS(T)
#define IRNODE_ABSTRACT_SUBCLASS(T) \
 public:                            \
    T *clone() const override = 0;  \
    IRNODE_COMMON_SUBCLASS(T)

// NOLINTEND(bugprone-macro-parentheses)
#define IRNODE_COMMON_SUBCLASS(T)                                                        \
 public:                                                                                 \
    using Node::operator==;                                                              \
    bool apply_visitor_preorder(Modifier &v) override;                                   \
    void apply_visitor_postorder(Modifier &v) override;                                  \
    void apply_visitor_revisit(Modifier &v, const Node *n) const override;               \
    void apply_visitor_loop_revisit(Modifier &v) const override;                         \
    bool apply_visitor_preorder(COWNode_info *, COWModifier &v) const override;          \
    void apply_visitor_postorder(COWNode_info *, COWModifier &v) const override;         \
    void apply_visitor_revisit(COWModifier &v, const Node *n) const override;            \
    void apply_visitor_loop_revisit(COWModifier &v) const override;                      \
    bool apply_visitor_preorder(Inspector &v) const override;                            \
    void apply_visitor_postorder(Inspector &v) const override;                           \
    void apply_visitor_revisit(Inspector &v) const override;                             \
    void apply_visitor_loop_revisit(Inspector &v) const override;                        \
    const Node *apply_visitor_preorder(Transform &v) override;                           \
    const Node *apply_visitor_postorder(Transform &v) override;                          \
    void apply_visitor_revisit(Transform &v, const Node *n) const override;              \
    void apply_visitor_loop_revisit(Transform &v) const override;                        \
    const Node *apply_visitor_preorder(COWNode_info *, COWTransform &v) const override;  \
    const Node *apply_visitor_postorder(COWNode_info *, COWTransform &v) const override; \
    void apply_visitor_revisit(COWTransform &v, const Node *n) const override;           \
    void apply_visitor_loop_revisit(COWTransform &v) const override;

/* only define 'apply' for a limited number of classes (those we want to call
 * visitors directly on), as defining it and making it virtual would mean that
 * NO Transform could transform the class into a sibling class */
#define IRNODE_DECLARE_APPLY_OVERLOAD(T)                                       \
    const T *apply(Visitor &v, const Visitor_Context *ctxt = nullptr) const;   \
    const T *apply(Visitor &&v, const Visitor_Context *ctxt = nullptr) const { \
        return apply(v, ctxt);                                                 \
    }
#define IRNODE_DEFINE_APPLY_OVERLOAD(CLASS, TEMPLATE, TT)                                    \
    TEMPLATE                                                                                 \
    const IR::CLASS TT *IR::CLASS TT::apply(Visitor &v, const Visitor_Context *ctxt) const { \
        const CLASS *tmp = this;                                                             \
        auto prof = v.init_apply(tmp, ctxt);                                                 \
        v.visit(tmp);                                                                        \
        v.end_apply(tmp);                                                                    \
        return tmp;                                                                          \
    }

}  // namespace P4::IR

#endif /* IR_NODE_H_ */
