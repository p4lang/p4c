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

    // init_apply is called (once) when apply is called on an IR tree
    // it expects to allocate a profile record which will be destroyed
    // when the traversal completes.  Visitor subclasses may extend this
    // to do any additional initialization they need.  They should call their
    // parent's init_apply to do further initialization
    virtual profile_t init_apply(const IR::Node *root);

    // apply_visitor is the main traversal function that manages the
    // depth-first recursive traversal.  `visit` is a convenience function
    // that calls it on a variable.
    virtual const IR::Node *apply_visitor(const IR::Node *n, const char *name = 0) = 0;
    void visit(const IR::Node *&n, const char *name = 0) { n = apply_visitor(n, name); }
    void visit(const IR::Node *const &n, const char *name = 0) {
        auto t = apply_visitor(n, name);
        if (t != n) visitor_const_error(); }
#define DECLARE_VISIT_FUNCTIONS(CLASS, BASE)                            \
    void visit(const IR::CLASS *&n, const char *name = 0);              \
    void visit(const IR::CLASS *const &n, const char *name = 0);
    IRNODE_ALL_SUBCLASSES(DECLARE_VISIT_FUNCTIONS)
#undef DECLARE_VISIT_FUNCTIONS

    // Functions for IR visit_children to call for ControlFlowVisitors.
    virtual Visitor &flow_clone() { return *this; }
    virtual void flow_merge(Visitor &) { }

    virtual const char *name() { return typeid(*this).name(); }
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
        auto result = ctxt->original->to<T>();
        CHECK_NULL(result);
        return result; }
    const Context *getContext() const { return ctxt->parent; }
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
        return findContext<T>(c); }

 protected:
    // if visitDagOnce is set to 'false' (usually in the derived Visitor
    // class constructor), nodes that appear mulitple times in the tree
    // will be visited repeatedly.  If this is done in a Modifier or Transform
    // pass, this will result in them being duplicated if they are modified
    bool visitDagOnce = true;
    bool dontForwardChildrenBeforePreorder = false;
    void visit_children(const IR::Node *, std::function<void()> fn) { fn(); }
    class ChangeTracker;  // used by Modifier and Transform -- private to them

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
    void visitor_const_error();
 public:
    profile_t init_apply(const IR::Node *root) override;
    const IR::Node *apply_visitor(const IR::Node *n, const char *name = 0) override;
    virtual bool preorder(IR::Node *) { return true; }
    virtual void postorder(IR::Node *) {}
#define DECLARE_VISIT_FUNCTIONS(CLASS, BASE)                            \
    virtual bool preorder(IR::CLASS *);                                 \
    virtual void postorder(IR::CLASS *);
    IRNODE_ALL_SUBCLASSES(DECLARE_VISIT_FUNCTIONS)
#undef DECLARE_VISIT_FUNCTIONS
};

class Inspector : public virtual Visitor {
    typedef unordered_map<const IR::Node *, bool>       visited_t;
    visited_t   *visited = nullptr;
 public:
    profile_t init_apply(const IR::Node *root) override;
    const IR::Node *apply_visitor(const IR::Node *, const char *name = 0) override;
    virtual bool preorder(const IR::Node *) { return true; }  // return 'false' to prune
    virtual void postorder(const IR::Node *) {}
#define DECLARE_VISIT_FUNCTIONS(CLASS, BASE)                            \
    virtual bool preorder(const IR::CLASS *);                           \
    virtual void postorder(const IR::CLASS *);
    IRNODE_ALL_SUBCLASSES(DECLARE_VISIT_FUNCTIONS)
#undef DECLARE_VISIT_FUNCTIONS
};

class Transform : public virtual Visitor {
    ChangeTracker       *visited = nullptr;
    bool prune_flag = false;
    void visitor_const_error();
 public:
    profile_t init_apply(const IR::Node *root) override;
    const IR::Node *apply_visitor(const IR::Node *, const char *name = 0) override;
    virtual const IR::Node *preorder(IR::Node *n) {return n;}
    virtual const IR::Node *postorder(IR::Node *n) {return n;}
#define DECLARE_VISIT_FUNCTIONS(CLASS, BASE)                            \
    virtual const IR::Node *preorder(IR::CLASS *);                      \
    virtual const IR::Node *postorder(IR::CLASS *);
    IRNODE_ALL_SUBCLASSES(DECLARE_VISIT_FUNCTIONS)
#undef DECLARE_VISIT_FUNCTIONS
 protected:
    // can only be called usefully from 'preorder' function
    void prune() { prune_flag = true; }
    const IR::Node *transform_child(const IR::Node *child) {
        auto *rv = apply_visitor(child);
        prune_flag = true;
        return rv; }
};

class ControlFlowVisitor : public virtual Visitor {
 protected:
    virtual ControlFlowVisitor *clone() const = 0;
    ControlFlowVisitor &flow_clone() override { return *clone(); }
};

class Backtrack : public virtual Visitor {
 public:
    struct trigger {
        enum type_t { OK, OTHER }       type;
        explicit trigger(type_t t) : type(t) {}
    };
    virtual bool backtrack(trigger &trig) = 0;
};

class P4WriteContext : public virtual Visitor {
 public:
    bool isWrite();
};

#endif /* _IR_VISITOR_H_ */
