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

#ifndef _IR_VISITOR_H_
#define _IR_VISITOR_H_

#include <stdexcept>
#include "std.h"
#include "lib/cstring.h"
#include "ir/ir.h"
#include "lib/exceptions.h"

class Visitor {
 public:
    struct Context {
        // We maintain a linked list of Context structures on the stack
        // in the Visitor::apply_visitor functions as we do the recursive
        // descent traversal.  pre/postorder function can access this
        // context via getContext/findContext
        const Context   *parent;
        const IR::Node  *node, *original;
        mutable int     child_index;
        mutable const char *child_name;
        int             depth;
    };
    class profile_t {
        // for profiling -- a profile_t object is created when a pass
        // starts and destroyed when it ends.  Moveable but not copyable.
        Visitor         &v;
        uint64_t        start;
        explicit profile_t(Visitor &);
        profile_t() = delete;
        profile_t(const profile_t &) = delete;
        profile_t &operator=(const profile_t &) = delete;
        profile_t &operator= (profile_t &&) = delete;
        friend class Visitor;
     public:
        ~profile_t();
        profile_t(profile_t &&);
    };
    virtual ~Visitor() = default;

    const char* internalName = nullptr;

    // init_apply is called (once) when apply is called on an IR tree
    // it expects to allocate a profile record which will be destroyed
    // when the traversal completes.  Visitor subclasses may extend this
    // to do any additional initialization they need.  They should call their
    // parent's init_apply to do further initialization
    virtual profile_t init_apply(const IR::Node *root);
    // End_apply is called symmetrically with init_apply, after the visit
    // is completed.  Both functions will be called in the event of a normal
    // completion, but only the 0-argument version will be called in the event
    // of an exception, as there is no root in that case.
    virtual void end_apply();
    virtual void end_apply(const IR::Node* root);

    // apply_visitor is the main traversal function that manages the
    // depth-first recursive traversal.  `visit` is a convenience function
    // that calls it on a variable.
    virtual const IR::Node *apply_visitor(const IR::Node *n, const char *name = 0) = 0;
    void visit(const IR::Node *&n, const char *name = 0) { n = apply_visitor(n, name); }
    void visit(const IR::Node *const &n, const char *name = 0) {
        auto t = apply_visitor(n, name);
        if (t != n) visitor_const_error(); }
    void visit(const IR::Node *&n, const char *name, int cidx) {
        ctxt->child_index = cidx;
        n = apply_visitor(n, name); }
    void visit(const IR::Node *const &n, const char *name, int cidx) {
        ctxt->child_index = cidx;
        auto t = apply_visitor(n, name);
        if (t != n) visitor_const_error(); }
    void visit(IR::Node *&, const char * = 0, int = 0) { BUG("Can't visit non-const pointer"); }
#define DECLARE_VISIT_FUNCTIONS(CLASS, BASE)                            \
    void visit(const IR::CLASS *&n, const char *name = 0);              \
    void visit(const IR::CLASS *const &n, const char *name = 0);        \
    void visit(const IR::CLASS *&n, const char *name, int cidx);        \
    void visit(const IR::CLASS *const &n, const char *name , int cidx); \
    void visit(IR::CLASS *&, const char * = 0, int = 0) { BUG("Can't visit non-const pointer"); }
    IRNODE_ALL_SUBCLASSES(DECLARE_VISIT_FUNCTIONS)
#undef DECLARE_VISIT_FUNCTIONS

    // Functions for IR visit_children to call for ControlFlowVisitors.
    virtual Visitor &flow_clone() { return *this; }
    virtual void flow_dead() { }
    virtual void flow_merge(Visitor &) { }

    virtual const char *name() const {
        if (internalName != nullptr)
            return internalName;
        return typeid(*this).name();
    }
    void setName(const char* name) { internalName = name; }
    void print_context() const;  // for debugging; can be called from debugger

    // Context access/search functions.  getContext returns the context
    // that refers to the immediate parent of the node currently being
    // visited.  findContext searches up the context for a (grand)parent
    // of a specific type.  Orig versions return the original node (before
    // any cloning or modifications)
    const IR::Node* getOriginal() const { return ctxt->original; }
    template <class T>
    const T* getOriginal() const {
        CHECK_NULL(ctxt->original);
        BUG_CHECK(ctxt->original->is<T>(), "%1% does not have the expected type %2%",
                  ctxt->original, typeid(T).name());
        return ctxt->original->to<T>();
    }
    const Context *getContext() const { return ctxt->parent; }
    template <class T>
    const T* getParent() const {
        return ctxt->parent ? ctxt->parent->node->to<T>() : nullptr; }
    int getChildrenVisited() const { return ctxt->child_index; }
    int getContextDepth() const { return ctxt->depth - 1; }
    template <class T> inline const T *findContext(const Context *&c) const {
        if (!c) c = ctxt;
        while ((c = c->parent))
            if (auto *rv = dynamic_cast<const T *>(c->node)) return rv;
        return nullptr; }
    template <class T> inline const T *findContext() const {
        const Context *c = ctxt;
        return findContext<T>(c); }
    template <class T> inline const T *findOrigCtxt(const Context *&c) const {
        if (!c) c = ctxt;
        while ((c = c->parent))
            if (auto *rv = dynamic_cast<const T *>(c->original)) return rv;
        return nullptr; }
    template <class T> inline const T *findOrigCtxt() const {
        const Context *c = ctxt;
        return findOrigCtxt<T>(c); }

 protected:
    // if visitDagOnce is set to 'false' (usually in the derived Visitor
    // class constructor), nodes that appear multiple times in the tree
    // will be visited repeatedly.  If this is done in a Modifier or Transform
    // pass, this will result in them being duplicated if they are modified.
    bool visitDagOnce = true;
    bool dontForwardChildrenBeforePreorder = false;
    // if joinFlows is 'true', Visitor will track nodes with more than one parent and
    // flow_merge the visitor from all the parents before visiting the node and its
    // children.  This only works for Inspector (not Modifier/Transform) currently.
    bool joinFlows = false;
    virtual void init_join_flows(const IR::Node *) { assert(0); }
    virtual bool join_flows(const IR::Node *) { return false; }
    void visit_children(const IR::Node *, std::function<void()> fn) { fn(); }
    class ChangeTracker;  // used by Modifier and Transform -- private to them
#ifndef NDEBUG
    virtual  // making this non-virtual for NDEBUG builds turns it into a noop
#endif
    void check_clone(const Visitor *) {}

 private:
    virtual void visitor_const_error();
    const Context *ctxt = nullptr;  // should be readonly to subclasses
    friend class Inspector;
    friend class Modifier;
    friend class Transform;
    friend class ControlFlowVisitor;
};

class Modifier : public virtual Visitor {
    ChangeTracker       *visited = nullptr;
    void visitor_const_error() override;
    void check_clone(const Visitor *) override;
 public:
    profile_t init_apply(const IR::Node *root) override;
    const IR::Node *apply_visitor(const IR::Node *n, const char *name = 0) override;
    virtual bool preorder(IR::Node *) { return true; }
    virtual void postorder(IR::Node *) {}
    virtual void revisit(const IR::Node *, const IR::Node *) {}
#define DECLARE_VISIT_FUNCTIONS(CLASS, BASE)                            \
    virtual bool preorder(IR::CLASS *);                                 \
    virtual void postorder(IR::CLASS *);                                \
    virtual void revisit(const IR::CLASS *, const IR::CLASS *);
    IRNODE_ALL_SUBCLASSES(DECLARE_VISIT_FUNCTIONS)
#undef DECLARE_VISIT_FUNCTIONS
    void revisit_visited();
};

class Inspector : public virtual Visitor {
    typedef unordered_map<const IR::Node *, bool>       visited_t;
    visited_t   *visited = nullptr;
    void check_clone(const Visitor *) override;
 public:
    profile_t init_apply(const IR::Node *root) override;
    const IR::Node *apply_visitor(const IR::Node *, const char *name = 0) override;
    virtual bool preorder(const IR::Node *) { return true; }  // return 'false' to prune
    virtual void postorder(const IR::Node *) {}
    virtual void revisit(const IR::Node *) {}
#define DECLARE_VISIT_FUNCTIONS(CLASS, BASE)                            \
    virtual bool preorder(const IR::CLASS *);                           \
    virtual void postorder(const IR::CLASS *);                          \
    virtual void revisit(const IR::CLASS *);
    IRNODE_ALL_SUBCLASSES(DECLARE_VISIT_FUNCTIONS)
#undef DECLARE_VISIT_FUNCTIONS
    void revisit_visited();
};

class Transform : public virtual Visitor {
    ChangeTracker       *visited = nullptr;
    bool prune_flag = false;
    void visitor_const_error() override;
    void check_clone(const Visitor *) override;

 public:
    profile_t init_apply(const IR::Node *root) override;
    const IR::Node *apply_visitor(const IR::Node *, const char *name = 0) override;
    virtual const IR::Node *preorder(IR::Node *n) {return n;}
    virtual const IR::Node *postorder(IR::Node *n) {return n;}
    virtual void revisit(const IR::Node *, const IR::Node *) {}
#define DECLARE_VISIT_FUNCTIONS(CLASS, BASE)                            \
    virtual const IR::Node *preorder(IR::CLASS *);                      \
    virtual const IR::Node *postorder(IR::CLASS *);                     \
    virtual void revisit(const IR::CLASS *, const IR::Node *);
    IRNODE_ALL_SUBCLASSES(DECLARE_VISIT_FUNCTIONS)
#undef DECLARE_VISIT_FUNCTIONS
    void revisit_visited();
    // can only be called usefully from a 'preorder' function (directly or indirectly)
    void prune() { prune_flag = true; }

 protected:
    const IR::Node *transform_child(const IR::Node *child) {
        auto *rv = apply_visitor(child);
        prune_flag = true;
        return rv; }
};

class ControlFlowVisitor : public virtual Visitor {
    map<const IR::Node *, std::pair<ControlFlowVisitor *, int>> *flow_join_points = 0;
 protected:
    virtual ControlFlowVisitor *clone() const = 0;
    void init_join_flows(const IR::Node *root) override;
    bool join_flows(const IR::Node *n) override;
    virtual bool filter_join_point(const IR::Node *) { return false; }
    ControlFlowVisitor &flow_clone() override;
};

class Backtrack : public virtual Visitor {
 public:
    struct trigger {
        enum type_t { OK, OTHER }       type;
        explicit trigger(type_t t) : type(t) {}
        virtual void dbprint(std::ostream &out) const { out << typeid(*this).name(); }
        template<class T> T *to() { return dynamic_cast<T *>(this); }
        template<class T> const T *to() const { return dynamic_cast<const T *>(this); }
        template<class T> bool is() { return to<T>() != nullptr; }
        template<class T> bool is() const { return to<T>() != nullptr; }
    };
    virtual bool backtrack(trigger &trig) = 0;
    virtual bool never_backtracks() { return false; }  // generally not overridden
        // returns true for passes that will never catch a trigger (backtrack() is always false)
};

class P4WriteContext : public virtual Visitor {
 public:
    bool isWrite();
};

#endif /* _IR_VISITOR_H_ */
