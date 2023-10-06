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

#ifndef IR_VISITOR_H_
#define IR_VISITOR_H_

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <map>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <utility>

#include "ir/gen-tree-macro.h"
#include "ir/ir-tree-macros.h"
#include "ir/node.h"
#include "ir/vector.h"
#include "lib/castable.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/null.h"
#include "lib/source_file.h"

// declare this outside of Visitor so it can be forward declared in node.h
struct Visitor_Context {
    // We maintain a linked list of Context structures on the stack
    // in the Visitor::apply_visitor functions as we do the recursive
    // descent traversal.  pre/postorder function can access this
    // context via getContext/findContext
    const Visitor_Context *parent;
    const IR::Node *node, *original;
    mutable int child_index;
    mutable const char *child_name;
    int depth;
    template <class T>
    inline const T *findContext(const Visitor_Context *&c) const {
        c = this;
        while ((c = c->parent))
            if (auto *rv = dynamic_cast<const T *>(c->node)) return rv;
        return nullptr;
    }
    template <class T>
    inline const T *findContext() const {
        const Visitor_Context *c = this;
        return findContext<T>(c);
    }
};

class SplitFlowVisit_base;

class Inspector;

class Visitor {
 public:
    typedef Visitor_Context Context;
    class profile_t {
        // for profiling -- a profile_t object is created when a pass
        // starts and destroyed when it ends.  Moveable but not copyable.
        Visitor &v;
        uint64_t start;
        explicit profile_t(Visitor &);
        profile_t() = delete;
        profile_t(const profile_t &) = delete;
        profile_t &operator=(const profile_t &) = delete;
        profile_t &operator=(profile_t &&) = delete;
        friend class Visitor;

     public:
        ~profile_t();
        profile_t(profile_t &&);
    };
    virtual ~Visitor() = default;

    mutable cstring internalName;
    // Some visitors are created and applied by other visitors.
    // This field keeps track of the caller.
    const Visitor *called_by = nullptr;

    const Visitor &setCalledBy(const Visitor *visitor) {
        called_by = visitor;
        return *this;
    }
    // init_apply is called (once) when apply is called on an IR tree
    // it expects to allocate a profile record which will be destroyed
    // when the traversal completes.  Visitor subclasses may extend this
    // to do any additional initialization they need.  They should call their
    // parent's init_apply to do further initialization
    virtual profile_t init_apply(const IR::Node *root);
    profile_t init_apply(const IR::Node *root, const Context *parent_context);
    // End_apply is called symmetrically with init_apply, after the visit
    // is completed.  Both functions will be called in the event of a normal
    // completion, but only the 0-argument version will be called in the event
    // of an exception, as there is no root in that case.
    virtual void end_apply();
    virtual void end_apply(const IR::Node *root);

    // apply_visitor is the main traversal function that manages the
    // depth-first recursive traversal.  `visit` is a convenience function
    // that calls it on a variable.
    virtual const IR::Node *apply_visitor(const IR::Node *n, const char *name = 0) = 0;
    void visit(const IR::Node *&n, const char *name = 0) { n = apply_visitor(n, name); }
    void visit(const IR::Node *const &n, const char *name = 0) {
        auto t = apply_visitor(n, name);
        if (t != n) visitor_const_error();
    }
    void visit(const IR::Node *&n, const char *name, int cidx) {
        ctxt->child_index = cidx;
        n = apply_visitor(n, name);
    }
    void visit(const IR::Node *const &n, const char *name, int cidx) {
        ctxt->child_index = cidx;
        auto t = apply_visitor(n, name);
        if (t != n) visitor_const_error();
    }
    void visit(IR::Node *&, const char * = 0, int = 0) { BUG("Can't visit non-const pointer"); }
#define DECLARE_VISIT_FUNCTIONS(CLASS, BASE)                           \
    void visit(const IR::CLASS *&n, const char *name = 0);             \
    void visit(const IR::CLASS *const &n, const char *name = 0);       \
    void visit(const IR::CLASS *&n, const char *name, int cidx);       \
    void visit(const IR::CLASS *const &n, const char *name, int cidx); \
    void visit(IR::CLASS *&, const char * = 0, int = 0) { BUG("Can't visit non-const pointer"); }
    IRNODE_ALL_SUBCLASSES(DECLARE_VISIT_FUNCTIONS)
#undef DECLARE_VISIT_FUNCTIONS
    void visit(IR::Node &n, const char *name = 0) {
        if (name && ctxt) ctxt->child_name = name;
        n.visit_children(*this);
    }
    void visit(const IR::Node &n, const char *name = 0) {
        if (name && ctxt) ctxt->child_name = name;
        n.visit_children(*this);
    }
    void visit(IR::Node &n, const char *name, int cidx) {
        if (ctxt) {
            ctxt->child_name = name;
            ctxt->child_index = cidx;
        }
        n.visit_children(*this);
    }
    void visit(const IR::Node &n, const char *name, int cidx) {
        if (ctxt) {
            ctxt->child_name = name;
            ctxt->child_index = cidx;
        }
        n.visit_children(*this);
    }
    template <class T>
    void parallel_visit(IR::Vector<T> &v, const char *name = 0) {
        if (name && ctxt) ctxt->child_name = name;
        v.parallel_visit_children(*this);
    }
    template <class T>
    void parallel_visit(const IR::Vector<T> &v, const char *name = 0) {
        if (name && ctxt) ctxt->child_name = name;
        v.parallel_visit_children(*this);
    }
    template <class T>
    void parallel_visit(IR::Vector<T> &v, const char *name, int cidx) {
        if (ctxt) {
            ctxt->child_name = name;
            ctxt->child_index = cidx;
        }
        v.parallel_visit_children(*this);
    }
    template <class T>
    void parallel_visit(const IR::Vector<T> &v, const char *name, int cidx) {
        if (ctxt) {
            ctxt->child_name = name;
            ctxt->child_index = cidx;
        }
        v.parallel_visit_children(*this);
    }

    virtual Visitor *clone() const {
        BUG("need %s::clone method", name());
        return nullptr;
    }
    virtual bool check_clone(const Visitor *a) { return typeid(*this) == typeid(*a); }

    // Functions for IR visit_children to call for ControlFlowVisitors.
    virtual Visitor &flow_clone() { return *this; }
    // all flow_clones share a split_link chain to allow stack walking
    SplitFlowVisit_base *split_link_mem = nullptr, *&split_link;
    Visitor() : split_link(split_link_mem) {}

    /** Merge the given visitor into this visitor at a joint point in the
     * control flow graph.  Should update @this and leave the other unchanged.
     */
    virtual void flow_merge(Visitor &) {}
    /** Support methods for non-local ControlFlow computations */
    virtual void flow_merge_global_to(cstring) {}
    virtual void flow_merge_global_from(cstring) {}
    virtual void erase_global(cstring) {}
    virtual bool check_global(cstring) { return false; }
    virtual void clear_globals() {}
    virtual bool has_flow_joins() const { return false; }

    static cstring demangle(const char *);
    virtual const char *name() const {
        if (!internalName) internalName = demangle(typeid(*this).name());
        return internalName.c_str();
    }
    void setName(const char *name) { internalName = name; }
    void print_context() const;  // for debugging; can be called from debugger

    // Context access/search functions.  getContext returns the context
    // that refers to the immediate parent of the node currently being
    // visited.  findContext searches up the context for a (grand)parent
    // of a specific type.  Orig versions return the original node (before
    // any cloning or modifications)
    const IR::Node *getOriginal() const { return ctxt->original; }
    template <class T>
    const T *getOriginal() const {
        CHECK_NULL(ctxt->original);
        BUG_CHECK(ctxt->original->is<T>(), "%1% does not have the expected type %2%",
                  ctxt->original, demangle(typeid(T).name()));
        return ctxt->original->to<T>();
    }
    const Context *getChildContext() const { return ctxt; }
    const Context *getContext() const { return ctxt->parent; }
    template <class T>
    const T *getParent() const {
        return ctxt->parent ? ctxt->parent->node->to<T>() : nullptr;
    }
    int getChildrenVisited() const { return ctxt->child_index; }
    int getContextDepth() const { return ctxt->depth - 1; }
    template <class T>
    inline const T *findContext(const Context *&c) const {
        if (!c) c = ctxt;
        while ((c = c->parent))
            if (auto *rv = dynamic_cast<const T *>(c->node)) return rv;
        return nullptr;
    }
    template <class T>
    inline const T *findContext() const {
        const Context *c = ctxt;
        return findContext<T>(c);
    }
    template <class T>
    inline const T *findOrigCtxt(const Context *&c) const {
        if (!c) c = ctxt;
        while ((c = c->parent))
            if (auto *rv = dynamic_cast<const T *>(c->original)) return rv;
        return nullptr;
    }
    template <class T>
    inline const T *findOrigCtxt() const {
        const Context *c = ctxt;
        return findOrigCtxt<T>(c);
    }
    inline bool isInContext(const IR::Node *n) const {
        for (auto *c = ctxt; c; c = c->parent) {
            if (c->node == n || c->original == n) return true;
        }
        return false;
    }

    /// @return the current node - i.e., the node that was passed to preorder()
    /// or postorder(). For Modifiers and Transforms, this is a clone of the
    /// node returned by getOriginal().
    const IR::Node *getCurrentNode() const { return ctxt->node; }
    template <class T>
    const T *getCurrentNode() const {
        return ctxt->node ? ctxt->node->to<T>() : nullptr;
    }

    /// True if the warning with this kind is enabled at this point.
    /// Warnings can be disabled by using the @noWarn("unused") annotation
    /// in an enclosing environment.
    bool warning_enabled(int warning_kind) const { return warning_enabled(this, warning_kind); }
    /// Static version of the above function, which can be called
    /// even if not directly in a visitor
    static bool warning_enabled(const Visitor *visitor, int warning_kind);
    template <
        class T,
        typename = typename std::enable_if<std::is_base_of<Util::IHasSourceInfo, T>::value>::type,
        class... Args>
    void warn(const int kind, const char *format, const T *node, Args... args) {
        if (warning_enabled(kind)) ::warning(kind, format, node, std::forward<Args>(args)...);
    }

    /// The const ref variant of the above
    template <
        class T,
        typename = typename std::enable_if<std::is_base_of<Util::IHasSourceInfo, T>::value>::type,
        class... Args>
    void warn(const int kind, const char *format, const T &node, Args... args) {
        if (warning_enabled(kind)) ::warning(kind, format, node, std::forward<Args>(args)...);
    }

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

    virtual void init_join_flows(const IR::Node *) {
        BUG("joinFlows only supported in ControlFlowVisitor currently");
    }

    /** If @n is a join point in the control flow graph (i.e. has multiple incoming
     * edges) and is not filtered out by `filter_join_point`, then:
     *
     *   - if this is the first time a visitor has visited @n, store a clone of the
     *   visitor with this node and return true, deferring visiting this node until
     *   all incoming edges have been visited.
     *
     *   - if this is NOT the first visitor, but also not the last, then merge this
     *   visitor into the stored visitor clone, and return true.
     *
     *   - finally, if this is the is the final visitor, merge the stored, cloned
     *   visitor---which has accumulated all previous visitors---with this one, and
     *   return false.
     *
     * `join_flows(n)` is invoked in `apply_visitor(n, name)`, and @n is only
     * visited if this method returns false.
     *
     * @return false if all upstream nodes from @n have been visited, and it's time
     * to visit @n.
     */
    virtual bool join_flows(const IR::Node *) { return false; }
    /** Called from `apply_visitor` after visiting the node (pre- and postorder) and
     *  its children after `join_flows` returned false */
    virtual void post_join_flows(const IR::Node *, const IR::Node *) {}

    void visit_children(const IR::Node *, std::function<void()> fn) { fn(); }
    class ChangeTracker;  // used by Modifier and Transform -- private to them
    // This overrides visitDagOnce for a single node -- can only be called from
    // preorder and postorder functions
    void visitOnce() const { *visitCurrentOnce = true; }
    void visitAgain() const { *visitCurrentOnce = false; }

 private:
    virtual void visitor_const_error();
    const Context *ctxt = nullptr;  // should be readonly to subclasses
    bool *visitCurrentOnce = nullptr;
    friend class Inspector;
    friend class Modifier;
    friend class Transform;
    friend class ControlFlowVisitor;
};

class Modifier : public virtual Visitor {
    std::shared_ptr<ChangeTracker> visited;
    void visitor_const_error() override;
    bool check_clone(const Visitor *) override;

 public:
    profile_t init_apply(const IR::Node *root) override;
    const IR::Node *apply_visitor(const IR::Node *n, const char *name = 0) override;
    virtual bool preorder(IR::Node *) { return true; }
    virtual void postorder(IR::Node *) {}
    virtual void revisit(const IR::Node *, const IR::Node *) {}
    virtual void loop_revisit(const IR::Node *) { BUG("IR loop detected"); }
#define DECLARE_VISIT_FUNCTIONS(CLASS, BASE)                    \
    virtual bool preorder(IR::CLASS *);                         \
    virtual void postorder(IR::CLASS *);                        \
    virtual void revisit(const IR::CLASS *, const IR::CLASS *); \
    virtual void loop_revisit(const IR::CLASS *);
    IRNODE_ALL_SUBCLASSES(DECLARE_VISIT_FUNCTIONS)
#undef DECLARE_VISIT_FUNCTIONS
    void revisit_visited();
    bool visit_in_progress(const IR::Node *) const;
};

class Inspector : public virtual Visitor {
    struct info_t {
        bool done, visitOnce;
    };
    typedef std::unordered_map<const IR::Node *, info_t> visited_t;
    std::shared_ptr<visited_t> visited;
    bool check_clone(const Visitor *) override;

 public:
    profile_t init_apply(const IR::Node *root) override;
    const IR::Node *apply_visitor(const IR::Node *, const char *name = 0) override;
    virtual bool preorder(const IR::Node *) { return true; }  // return 'false' to prune
    virtual void postorder(const IR::Node *) {}
    virtual void revisit(const IR::Node *) {}
    virtual void loop_revisit(const IR::Node *) { BUG("IR loop detected"); }
#define DECLARE_VISIT_FUNCTIONS(CLASS, BASE)   \
    virtual bool preorder(const IR::CLASS *);  \
    virtual void postorder(const IR::CLASS *); \
    virtual void revisit(const IR::CLASS *);   \
    virtual void loop_revisit(const IR::CLASS *);
    IRNODE_ALL_SUBCLASSES(DECLARE_VISIT_FUNCTIONS)
#undef DECLARE_VISIT_FUNCTIONS
    void revisit_visited();
    bool visit_in_progress(const IR::Node *n) const {
        if (visited->count(n)) return !visited->at(n).done;
        return false;
    }
};

class Transform : public virtual Visitor {
    std::shared_ptr<ChangeTracker> visited;
    bool prune_flag = false;
    void visitor_const_error() override;
    bool check_clone(const Visitor *) override;

 public:
    profile_t init_apply(const IR::Node *root) override;
    const IR::Node *apply_visitor(const IR::Node *, const char *name = 0) override;
    virtual const IR::Node *preorder(IR::Node *n) { return n; }
    virtual const IR::Node *postorder(IR::Node *n) { return n; }
    virtual void revisit(const IR::Node *, const IR::Node *) {}
    virtual void loop_revisit(const IR::Node *) { BUG("IR loop detected"); }
#define DECLARE_VISIT_FUNCTIONS(CLASS, BASE)                   \
    virtual const IR::Node *preorder(IR::CLASS *);             \
    virtual const IR::Node *postorder(IR::CLASS *);            \
    virtual void revisit(const IR::CLASS *, const IR::Node *); \
    virtual void loop_revisit(const IR::CLASS *);
    IRNODE_ALL_SUBCLASSES(DECLARE_VISIT_FUNCTIONS)
#undef DECLARE_VISIT_FUNCTIONS
    void revisit_visited();
    bool visit_in_progress(const IR::Node *) const;
    // can only be called usefully from a 'preorder' function (directly or indirectly)
    void prune() { prune_flag = true; }

 protected:
    const IR::Node *transform_child(const IR::Node *child) {
        auto *rv = apply_visitor(child);
        prune_flag = true;
        return rv;
    }
};

// turn this on for extra info tracking control joinFlows for debugging
#define DEBUG_FLOW_JOIN 0

class ControlFlowVisitor : public virtual Visitor {
    std::map<cstring, ControlFlowVisitor &> &globals;

 protected:
    ControlFlowVisitor *clone() const override = 0;
    struct flow_join_info_t {
        ControlFlowVisitor *vclone = nullptr;  // a clone to accumulate info into
        int count = 0;                         // additional parents needed -1
        bool done = false;
#if DEBUG_FLOW_JOIN
        struct ctrs_t {
            int exist = 0, visited = 0;
        };
        std::map<const IR::Node *, ctrs_t> parents;
#endif
    };
    typedef std::map<const IR::Node *, flow_join_info_t> flow_join_points_t;
    friend std::ostream &operator<<(std::ostream &, const flow_join_info_t &);
    friend std::ostream &operator<<(std::ostream &, const flow_join_points_t &);
    friend void dump(const flow_join_info_t &);
    friend void dump(const flow_join_info_t *);
    friend void dump(const flow_join_points_t &);
    friend void dump(const flow_join_points_t *);

    flow_join_points_t *flow_join_points = 0;
    class SetupJoinPoints : public Inspector {
     protected:
        flow_join_points_t &join_points;
        bool preorder(const IR::Node *n) override {
            BUG_CHECK(join_points.count(n) == 0, "oops");
#if DEBUG_FLOW_JOIN
            auto *ctxt = getContext();
            join_points[n].parents[ctxt ? ctxt->original : nullptr].exist++;
#endif
            return true;
        }
        void revisit(const IR::Node *n) override {
            ++join_points[n].count;
#if DEBUG_FLOW_JOIN
            auto *ctxt = getContext();
            join_points[n].parents[ctxt ? ctxt->original : nullptr].exist++;
#endif
        }

     public:
        explicit SetupJoinPoints(flow_join_points_t &fjp) : join_points(fjp) {}
    };
    bool BackwardsCompatibleBroken = false;  // enable old broken behavior for code that
                                             // depends on it.  Should not be set for new uses.

    virtual void applySetupJoinPoints(const IR::Node *root) {
        root->apply(SetupJoinPoints(*flow_join_points));
    }
    void init_join_flows(const IR::Node *root) override;
    bool join_flows(const IR::Node *n) override;
    void post_join_flows(const IR::Node *, const IR::Node *) override;

    /** This will be called for all nodes with multiple incoming edges, and
     * should return 'false' if the node should be considered a join point, and
     * 'true' if if should not be considered one. Nodes with only one incoming
     * edge are never join points.
     */
    virtual bool filter_join_point(const IR::Node *) { return false; }
    ControlFlowVisitor &flow_clone() override;
    void flow_merge(Visitor &) override = 0;
    virtual void flow_copy(ControlFlowVisitor &) = 0;
    ControlFlowVisitor() : globals(*new std::map<cstring, ControlFlowVisitor &>) {}

 public:
    void flow_merge_global_to(cstring key) override {
        if (globals.count(key))
            globals.at(key).flow_merge(*this);
        else
            globals.emplace(key, flow_clone());
    }
    void flow_merge_global_from(cstring key) override {
        if (globals.count(key)) flow_merge(globals.at(key));
    }
    void erase_global(cstring key) override { globals.erase(key); }
    bool check_global(cstring key) override { return globals.count(key) != 0; }
    void clear_globals() override { globals.clear(); }

    /// RAII class to ensure global key is only used in one place
    class GuardGlobal {
        Visitor &self;
        cstring key;

     public:
        GuardGlobal(Visitor &self, cstring key) : self(self), key(key) {
            BUG_CHECK(!self.check_global(key), "ControlFlowVisitor global %s in use", key);
        }
        ~GuardGlobal() { self.erase_global(key); }
    };

    bool has_flow_joins() const override { return !!flow_join_points; }
    const flow_join_info_t *flow_join_status(const IR::Node *n) const {
        if (!flow_join_points || !flow_join_points->count(n)) return nullptr;
        return &flow_join_points->at(n);
    }
};

/** SplitFlowVisit_base is base class for doing coroutine-like processing of
 * of flows in a visitor.  A visit_children method (or a preorder override) can
 * instantiate a local SplitFlowVist subclass to record the various children that
 * need to be visited, and the chain of currently in existence SplitFlowVisit objects
 * will be recorded in split_link.  When flows need to be joined (to visit a Dag
 * node with mulitple parents), processing of the node can be 'paused' and other
 * children from the SplitFlowVist can be visited, in order to visit all parents
 * before the child is visited. */
class SplitFlowVisit_base {
 protected:
    Visitor &v;
    SplitFlowVisit_base *prev;
    std::vector<Visitor *> visitors;
    int visit_next = 0, start_index = 0;
    bool paused = false;
    friend ControlFlowVisitor;

    explicit SplitFlowVisit_base(Visitor &v) : v(v) {
        prev = v.split_link;
        v.split_link = this;
    }
    ~SplitFlowVisit_base() { v.split_link = prev; }
    void *operator new(size_t);  // declared and not defined, as this class can
    // only be instantiated on the stack.  Trying to allocate one on the heap will
    // cause a linker error.

 public:
    bool finished() { return size_t(visit_next) >= visitors.size(); }
    virtual bool ready() { return !finished() && !paused; }
    void pause() { paused = true; }
    void unpause() { paused = false; }
    virtual void do_visit() = 0;
    virtual void run_visit() {
        auto *ctxt = v.getChildContext();
        start_index = ctxt ? ctxt->child_index : 0;
        paused = false;
        while (!finished()) do_visit();
        for (auto *cl : visitors) {
            if (cl && cl != &v) v.flow_merge(*cl);
        }
    }
    virtual void dbprint(std::ostream &) const = 0;
    friend void dump(const SplitFlowVisit_base *);
};

template <class N>
class SplitFlowVisit : public SplitFlowVisit_base {
    std::vector<const N **> nodes;
    std::vector<const N *const *> const_nodes;

 public:
    explicit SplitFlowVisit(Visitor &v) : SplitFlowVisit_base(v) {}
    void addNode(const N *&node) {
        BUG_CHECK(const_nodes.empty(), "Mixing const and non-const in SplitFlowVisit");
        BUG_CHECK(visitors.size() == nodes.size(), "size mismatch in SplitFlowVisit");
        BUG_CHECK(visit_next == 0, "Can't addNode to SplitFlowVisit after visiting started");
        visitors.push_back(visitors.empty() ? &v : &v.flow_clone());
        nodes.emplace_back(&node);
    }
    void addNode(const N *const &node) {
        BUG_CHECK(nodes.empty(), "Mixing const and non-const in SplitFlowVisit");
        BUG_CHECK(visitors.size() == const_nodes.size(), "size mismatch in SplitFlowVisit");
        BUG_CHECK(visit_next == 0, "Can't addNode to SplitFlowVisit after visiting started");
        visitors.push_back(visitors.empty() ? &v : &v.flow_clone());
        const_nodes.emplace_back(&node);
    }
    template <class T1, class T2, class... Args>
    void addNode(T1 &&t1, T2 &&t2, Args &&...args) {
        addNode(std::forward<T1>(t1));
        addNode(std::forward<T2>(t2), std::forward<Args>(args)...);
    }
    template <class... Args>
    explicit SplitFlowVisit(Visitor &v, Args &&...args) : SplitFlowVisit(v) {
        addNode(std::forward<Args>(args)...);
    }
    void do_visit() override {
        if (!finished()) {
            BUG_CHECK(!paused, "trying to visit paused split_flow_visitor");
            int idx = visit_next++;
            if (nodes.empty())
                visitors.at(idx)->visit(*const_nodes.at(idx), nullptr, start_index + idx);
            else
                visitors.at(idx)->visit(*nodes.at(idx), nullptr, start_index + idx);
        }
    }
    void dbprint(std::ostream &out) const override {
        out << "SplitFlowVisit processed " << visit_next << " of " << visitors.size();
    }
};

template <class N>
class SplitFlowVisitVector : public SplitFlowVisit_base {
    IR::Vector<N> *vec = nullptr;
    const IR::Vector<N> *const_vec = nullptr;
    std::vector<const IR::Node *> result;
    void init_visit(size_t size) {
        if (size > 0) visitors.push_back(&v);
        while (visitors.size() < size) visitors.push_back(&v.flow_clone());
    }

 public:
    SplitFlowVisitVector(Visitor &v, IR::Vector<N> &vec) : SplitFlowVisit_base(v), vec(&vec) {
        result.resize(vec.size());
        init_visit(vec.size());
    }
    SplitFlowVisitVector(Visitor &v, const IR::Vector<N> &vec)
        : SplitFlowVisit_base(v), const_vec(&vec) {
        init_visit(vec.size());
    }
    void do_visit() override {
        if (!finished()) {
            BUG_CHECK(!paused, "trying to visit paused split_flow_visitor");
            int idx = visit_next++;
            if (vec)
                result[idx] = visitors.at(idx)->apply_visitor(vec->at(idx));
            else
                visitors.at(idx)->visit(const_vec->at(idx), nullptr, start_index + idx);
        }
    }
    void run_visit() override {
        SplitFlowVisit_base::run_visit();
        if (vec) {
            int idx = 0;
            for (auto i = vec->begin(); i != vec->end(); ++idx) {
                if (!result[idx] && *i) {
                    i = vec->erase(i);
                } else if (result[idx] == *i) {
                    ++i;
                } else if (auto l = dynamic_cast<const IR::Vector<N> *>(result[idx])) {
                    i = vec->erase(i);
                    i = vec->insert(i, l->begin(), l->end());
                    i += l->size();
                } else if (auto v = dynamic_cast<const IR::VectorBase *>(result[idx])) {
                    if (v->empty()) {
                        i = vec->erase(i);
                    } else {
                        i = vec->insert(i, v->size() - 1, nullptr);
                        for (auto el : *v) {
                            CHECK_NULL(el);
                            if (auto e = dynamic_cast<const N *>(el))
                                *i++ = e;
                            else
                                BUG("visitor returned invalid type %s for Vector<%s>",
                                    el->node_type_name(), N::static_type_name());
                        }
                    }
                } else if (auto e = dynamic_cast<const N *>(result[idx])) {
                    *i++ = e;
                } else {
                    CHECK_NULL(result[idx]);
                    BUG("visitor returned invalid type %s for Vector<%s>",
                        result[idx]->node_type_name(), N::static_type_name());
                }
            }
        }
    }
    void dbprint(std::ostream &out) const override {
        out << "SplitFlowVisitVector processed " << visit_next << " of " << visitors.size();
    }
};

class Backtrack : public virtual Visitor {
 public:
    struct trigger : public ICastable {
        enum type_t { OK, OTHER } type;
        explicit trigger(type_t t) : type(t) {}
        virtual ~trigger();
        virtual void dbprint(std::ostream &out) const { out << demangle(typeid(*this).name()); }

     protected:
        // must call this from the constructor if a trigger subclass contains pointers
        // or references to GC objects
        void register_for_gc(size_t);
    };
    virtual bool backtrack(trigger &trig) = 0;
    virtual bool never_backtracks() { return false; }  // generally not overridden
    // returns true for passes that will never catch a trigger (backtrack() is always false)
};

class P4WriteContext : public virtual Visitor {
 public:
    bool isWrite(bool root_value = false);  // might write based on context
    bool isRead(bool root_value = false);   // might read based on context
    // note that the context might (conservatively) return true for BOTH isWrite and isRead,
    // as it might be an 'inout' access or it might be unable to decide.
};

/**
 * Invoke an inspector @function for every node of type @NodeType in the subtree
 * rooted at @root. The behavior is the same as a postorder Inspector.
 */
template <typename NodeType, typename Func>
void forAllMatching(const IR::Node *root, Func &&function) {
    struct NodeVisitor : public Inspector {
        explicit NodeVisitor(Func &&function) : function(function) {}
        Func function;
        void postorder(const NodeType *node) override { function(node); }
    };
    root->apply(NodeVisitor(std::forward<Func>(function)));
}

/**
 * Invoke a modifier @function for every node of type @NodeType in the subtree
 * rooted at @root. The behavior is the same as a postorder Modifier.
 *
 * @return the root of the new, modified version of the subtree.
 */
template <typename NodeType, typename RootType, typename Func>
const RootType *modifyAllMatching(const RootType *root, Func &&function) {
    struct NodeVisitor : public Modifier {
        explicit NodeVisitor(Func &&function) : function(function) {}
        Func function;
        void postorder(NodeType *node) override { function(node); }
    };
    return root->apply(NodeVisitor(std::forward<Func>(function)));
}

/**
 * Invoke a transform @function for every node of type @NodeType in the subtree
 * rooted at @root. The behavior is the same as a postorder Transform.
 *
 * @return the root of the new, transformed version of the subtree.
 */
template <typename NodeType, typename Func>
const IR::Node *transformAllMatching(const IR::Node *root, Func &&function) {
    struct NodeVisitor : public Transform {
        explicit NodeVisitor(Func &&function) : function(function) {}
        Func function;
        const IR::Node *postorder(NodeType *node) override { return function(node); }
    };
    return root->apply(NodeVisitor(std::forward<Func>(function)));
}

#endif /* IR_VISITOR_H_ */
