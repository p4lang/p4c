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

#include <time.h>
#include "ir.h"
#include "lib/log.h"

/** @class Visitor::ChangeTracker
 *  @brief Assists visitors in traversing the IR.

 *  A ChangeTracker object assists visitors traversing the IR by tracking each
 *  node.  The `start` method begins tracking, and `finish` ends it.  The
 *  `done` method determines whether the node has been visited, and `result`
 *  returns the new IR if it changed.
 */
class Visitor::ChangeTracker {
    struct visit_info_t {
        bool            visit_in_progress;
        bool            visitOnce;
        const IR::Node  *result;
    };
    typedef std::unordered_map<const IR::Node *, visit_info_t>  visited_t;
    visited_t           visited;

 public:
    /** Begin tracking @n during a visiting pass.  Use `finish(@n)` to mark @n as
     * visited once the pass completes.
     */
    void start(const IR::Node *n, bool defaultVisitOnce) {
        // Initialization
        visited_t::iterator visited_it;
        bool inserted;
        bool visit_in_progress = true;
        std::tie(visited_it, inserted) =
            visited.emplace(n, visit_info_t{visit_in_progress, defaultVisitOnce, n});

        // Sanity check for IR loops
        bool already_present = !inserted;
        visit_info_t *visit_info = &(visited_it->second);
        if (already_present && visit_info->visit_in_progress)
            BUG("IR loop detected ");
    }

    /** Mark the process of visiting @orig as finished, with @final being the
     * final state of the node, or nullptr if the node was removed from the
     * tree.  `done(@orig)` will return true, and `result(@orig)` will return
     * the resulting node, if any.
     *
     * If @final is a new node, that node is marked as finished as well, as if
     * `start(@final); finish(@final);` were invoked.
     *
     * @return true if the node has changed or been removed.
     * 
     * @exception Util::CompilerBug This method fails if `start(@orig)` has not
     * previously been invoked.
     */
    bool finish(const IR::Node *orig, const IR::Node *final) {
        auto it = visited.find(orig);
        if (it == visited.end())
            BUG("visitor state tracker corrupted");

        visit_info_t *orig_visit_info = &(it->second);
        orig_visit_info->visit_in_progress = false;
        if (!final) {
            orig_visit_info->result = final;
            return true;
        } else if (final != orig && *final != *orig) {
            orig_visit_info->result = final;
            visited.emplace(final, visit_info_t{false, orig_visit_info->visitOnce, final});
            return true;
        } else {
            // FIXME -- not safe if the visitor resurrects the node (which it shouldn't)
            // if (final && final->id == IR::Node::currentId - 1)
            //     --IR::Node::currentId;
            return false; } }

    /** Return a pointer to the visitOnce flag for node @n so that it can be changed
     */
    bool *refVisitOnce(const IR::Node *n) {
        if (!visited.count(n))
            BUG("visitor state tracker corrupted");
        return &visited.at(n).visitOnce;
    }

    /** Forget nodes that have already been visited, allowing them to be visited
     * again. */
    void revisit_visited() {
        for (auto it = visited.begin(); it != visited.end();) {
            if (!it->second.visit_in_progress)
                it = visited.erase(it);
            else
                ++it; } }

    /** Determine whether @n has been visited and the visitor has finished
     *  and we don't want to visit @n again the next time we see it.
     * That is, `start(@n)` has been invoked, followed by `finish(@n)`,
     * and the visitOnce field is true.
     * 
     * @return true if @n has been visited and the visitor is finished and visitOnce is true
     */
    bool done(const IR::Node *n) const {
        auto it = visited.find(n);
        return it != visited.end() && !it->second.visit_in_progress && it->second.visitOnce;
    }

    /** Produce the result of visiting @n.
     *
     * @return The result of visiting @n, or the intermediate result of
     * visiting @n if `start(@n)` has been invoked but not `finish(@n)`, or @n
     * if `start(@n)` has not been invoked.
     */
    const IR::Node *result(const IR::Node *n) const {
        if (!visited.count(n))
            return n;
        return visited.at(n).result;
    }
};

Visitor::profile_t Visitor::init_apply(const IR::Node *root) {
    if (ctxt) BUG("previous use of visitor did not clean up properly");
    ctxt = nullptr;
    if (joinFlows) init_join_flows(root);
    return profile_t(*this);
}
Visitor::profile_t Modifier::init_apply(const IR::Node *root) {
    auto rv = Visitor::init_apply(root);
    visited = new ChangeTracker();
    return rv; }
Visitor::profile_t Inspector::init_apply(const IR::Node *root) {
    auto rv = Visitor::init_apply(root);
    visited = new visited_t();
    return rv; }
Visitor::profile_t Transform::init_apply(const IR::Node *root) {
    auto rv = Visitor::init_apply(root);
    visited = new ChangeTracker();
    return rv; }
void Visitor::end_apply() {}
void Visitor::end_apply(const IR::Node*) {}

static indent_t profile_indent;
Visitor::profile_t::profile_t(Visitor &v_) : v(v_) {
    struct timespec ts;
#ifdef CLOCK_MONOTONIC
    clock_gettime(CLOCK_MONOTONIC, &ts);
#else
    // FIXME -- figure out how to do this on OSX/Mach
    ts.tv_sec = ts.tv_nsec = 0;
#endif
    start = ts.tv_sec*1000000000UL + ts.tv_nsec + 1;
    assert(start);
    ++profile_indent;
}
Visitor::profile_t::profile_t(profile_t &&a) : v(a.v), start(a.start) {
    a.start = 0;
}
Visitor::profile_t::~profile_t() {
    if (start) {
        v.end_apply();
        --profile_indent;
        struct timespec ts;
#ifdef CLOCK_MONOTONIC
        clock_gettime(CLOCK_MONOTONIC, &ts);
#else
        // FIXME -- figure out how to do this on OSX/Mach
        ts.tv_sec = ts.tv_nsec = 0;
#endif
        uint64_t end = ts.tv_sec*1000000000UL + ts.tv_nsec + 1;
        LOG1(profile_indent << v.name() << ' ' << (end-start)/1000.0 << " usec"); }
}

void Visitor::print_context() const {
    std::ostream &out = std::cout;
    out << "Context:" << std::endl;
    auto ctx = getContext();
    if (ctx == nullptr) {
        out << "<nullptr>" << std::endl;
        return;
    }

    while (ctx != nullptr) {
        out << ctx->node << " (" << ctx->original << ")" << std::endl;
        ctx = ctx->parent;
    }
}

void Visitor::visitor_const_error() {
    BUG("const Visitor wants to change IR"); }
void Modifier::visitor_const_error() {
    BUG("Modifier called const visit function -- missing template "
                            "instantiation in gen-tree-macro.h?"); }
void Transform::visitor_const_error() {
    BUG("Transform called const visit function -- missing template "
                            "instantiation in gen-tree-macro.h?"); }

struct PushContext {
    Visitor::Context current;
    const Visitor::Context *&stack;
    PushContext(const Visitor::Context *&stck, const IR::Node *node) : stack(stck) {
        current.parent = stack;
        current.node = current.original = node;
        current.child_index = 0;
        current.depth = stack ? stack->depth+1 : 1;
        assert(current.depth < 10000);    // stack overflow?
        stack = &current; }
    ~PushContext() { stack = current.parent; }
};

namespace {
class ForwardChildren : public Visitor {
    const ChangeTracker &visited;
    const IR::Node *apply_visitor(const IR::Node *n, const char * = 0) {
        if (visited.done(n))
            return visited.result(n);
        return n; }
 public:
    explicit ForwardChildren(const ChangeTracker &v) : visited(v) {}
};
}    // namespace

const IR::Node *Modifier::apply_visitor(const IR::Node *n, const char *name) {
    if (ctxt) ctxt->child_name = name;
    if (n) {
        PushContext local(ctxt, n);
        if (visited->done(n)) {
            n->apply_visitor_revisit(*this, visited->result(n));
            n = visited->result(n);
        } else {
            visited->start(n, visitDagOnce);
            IR::Node *copy = n->clone();
            local.current.node = copy;
            if (!dontForwardChildrenBeforePreorder) {
                ForwardChildren forward_children(*visited);
                copy->visit_children(forward_children); }
            visitCurrentOnce = visited->refVisitOnce(n);
            if (copy->apply_visitor_preorder(*this)) {
                copy->visit_children(*this);
                visitCurrentOnce = visited->refVisitOnce(n);
                copy->apply_visitor_postorder(*this); }
            if (visited->finish(n, copy))
                (n = copy)->validate(); } }
    if (ctxt)
        ctxt->child_index++;
    else
        visited = nullptr;
    return n;
}

const IR::Node *Inspector::apply_visitor(const IR::Node *n, const char *name) {
    if (ctxt) ctxt->child_name = name;
    if (n && !join_flows(n)) {
        PushContext local(ctxt, n);
        auto vp = visited->emplace(n, info_t{false, visitDagOnce});
        if (!vp.second && !vp.first->second.done)
            BUG("IR loop detected");
        if (!vp.second && vp.first->second.visitOnce) {
            n->apply_visitor_revisit(*this);
        } else {
            vp.first->second.done = false;
            visitCurrentOnce = &vp.first->second.visitOnce;
            if (n->apply_visitor_preorder(*this)) {
                n->visit_children(*this);
                visitCurrentOnce = &vp.first->second.visitOnce;
                n->apply_visitor_postorder(*this); }
            if (vp.first != visited->find(n))
                BUG("visitor state tracker corrupted");
            vp.first->second.done = true; } }
    if (ctxt)
        ctxt->child_index++;
    else
        visited = nullptr;
    return n;
}

const IR::Node *Transform::apply_visitor(const IR::Node *n, const char *name) {
    if (ctxt) ctxt->child_name = name;
    if (n) {
        PushContext local(ctxt, n);
        if (visited->done(n)) {
            n->apply_visitor_revisit(*this, visited->result(n));
            n = visited->result(n);
        } else {
            visited->start(n, visitDagOnce);
            auto copy = n->clone();
            local.current.node = copy;
            if (!dontForwardChildrenBeforePreorder) {
                ForwardChildren forward_children(*visited);
                copy->visit_children(forward_children); }
            bool save_prune_flag = prune_flag;
            prune_flag = false;
            visitCurrentOnce = visited->refVisitOnce(n);
            bool extra_clone = false;
            const IR::Node *preorder_result = copy->apply_visitor_preorder(*this);
            assert(preorder_result != n);  // should never happen
            const IR::Node *final_result = preorder_result;
            if (preorder_result != copy) {
                // FIXME -- not safe if the visitor resurrects the node (which it shouldn't)
                // if (copy->id == IR::Node::currentId - 1)
                //     --IR::Node::currentId;
                if (!preorder_result) {
                    prune_flag = true;
                } else if (visited->done(preorder_result)) {
                    final_result = visited->result(preorder_result);
                    prune_flag = true;
                } else {
                    extra_clone = true;
                    visited->start(preorder_result, *visitCurrentOnce);
                    local.current.node = copy = preorder_result->clone(); } }
            if (!prune_flag) {
                copy->visit_children(*this);
                visitCurrentOnce = visited->refVisitOnce(n);
                final_result = copy->apply_visitor_postorder(*this); }
            prune_flag = save_prune_flag;
            if (final_result == copy
                && final_result != preorder_result
                && *final_result == *preorder_result)
                final_result = preorder_result;
            if (visited->finish(n, final_result) && (n = final_result))
                final_result->validate();
            if (extra_clone)
                visited->finish(preorder_result, final_result); } }
    if (ctxt)
        ctxt->child_index++;
    else
        visited = nullptr;
    return n;
}

void Inspector::revisit_visited() {
    for (auto it = visited->begin(); it != visited->end();) {
        if (it->second.done)
            it = visited->erase(it);
        else
            ++it; }
}
void Modifier::revisit_visited() {
    visited->revisit_visited();
}
void Transform::revisit_visited() {
    visited->revisit_visited();
}

#define DEFINE_VISIT_FUNCTIONS(CLASS, BASE)                                             \
bool Modifier::preorder(IR::CLASS *n) {                                                 \
    return preorder(static_cast<IR::BASE *>(n)); }                                      \
void Modifier::postorder(IR::CLASS *n) {                                                \
    postorder(static_cast<IR::BASE *>(n)); }                                            \
void Modifier::revisit(const IR::CLASS *o, const IR::CLASS *n) {                        \
    revisit(static_cast<const IR::BASE *>(o), static_cast<const IR::BASE *>(n)); }      \
bool Inspector::preorder(const IR::CLASS *n) {                                          \
    return preorder(static_cast<const IR::BASE *>(n)); }                                \
void Inspector::postorder(const IR::CLASS *n) {                                         \
    postorder(static_cast<const IR::BASE *>(n)); }                                      \
void Inspector::revisit(const IR::CLASS *n) {                                           \
    revisit(static_cast<const IR::BASE *>(n)); }                                        \
const IR::Node *Transform::preorder(IR::CLASS *n) {                                     \
    return preorder(static_cast<IR::BASE *>(n)); }                                      \
const IR::Node *Transform::postorder(IR::CLASS *n) {                                    \
    return postorder(static_cast<IR::BASE *>(n)); }                                     \
void Transform::revisit(const IR::CLASS *o, const IR::Node *n) {                        \
    return revisit(static_cast<const IR::BASE *>(o), n); }                              \

IRNODE_ALL_SUBCLASSES(DEFINE_VISIT_FUNCTIONS)
#undef DEFINE_VISIT_FUNCTIONS

class SetupJoinPoints : public Inspector {
    std::map<const IR::Node *, std::pair<ControlFlowVisitor *, int>> &join_points;
    bool preorder(const IR::Node *n) override {
        return ++join_points[n].second == 1; }
 public:
    explicit SetupJoinPoints(decltype(join_points) &fjp)
    : join_points(fjp) { visitDagOnce = false; }
};

void ControlFlowVisitor::init_join_flows(const IR::Node *root) {
    if (!dynamic_cast<Inspector *>(static_cast<Visitor *>(this)))
        BUG("joinFlows only works for Inspector passes currently, not Modifier or Transform");
    if (flow_join_points)
        flow_join_points->clear();
    else
        flow_join_points = new std::remove_reference<decltype(*flow_join_points)>::type;
    root->apply(SetupJoinPoints(*flow_join_points));
    for (auto it = flow_join_points->begin(); it != flow_join_points->end(); ) {
        if (it->second.second > 1 && !filter_join_point(it->first)) {
            ++it;
        } else {
            it = flow_join_points->erase(it); } }
}

bool ControlFlowVisitor::join_flows(const IR::Node *n) {
    if (flow_join_points && flow_join_points->count(n)) {
        auto &status = flow_join_points->at(n);
        // Decrement the number of upstream edges yet to be traversed.  If none
        // remain, merge and return false to visit this node.
        if (!--status.second) {
            flow_merge(*status.first);
            return false;
        } else if (status.first) {
            // If there are still unvisited upstream edges but this is not the
            // first time this node has been reached, merge this visitor with
            // the accumulator (status.first) and return true to defer visiting
            // this node.
            status.first->flow_merge(*this);
            return true;
        } else {
            // Otherwise, this is the first time this node has been visited.
            // Clone this visitor and store it as the initial accumulator
            // value.
            status.first = clone();
            return true; } }
    return false;
}

bool Inspector::check_clone(const Visitor *v) {
    auto *t = dynamic_cast<const Inspector *>(v);
    BUG_CHECK(t && t->visited == visited, "Clone failed to copy base object");
    return true;
}
bool Modifier::check_clone(const Visitor *v) {
    auto *t = dynamic_cast<const Modifier *>(v);
    BUG_CHECK(t && t->visited == visited, "Clone failed to copy base object");
    return true;
}
bool Transform::check_clone(const Visitor *v) {
    auto *t = dynamic_cast<const Transform *>(v);
    BUG_CHECK(t && t->visited == visited, "Clone failed to copy base object");
    return true;
}

ControlFlowVisitor &ControlFlowVisitor::flow_clone() {
    auto *rv = clone();
    assert(rv->check_clone(this));
    return *rv;
}

IRNODE_ALL_NON_TEMPLATE_CLASSES(DEFINE_APPLY_FUNCTIONS, , , )

#define DEFINE_VISIT_FUNCTIONS(CLASS, BASE)                                             \
    void Visitor::visit(const IR::CLASS *&n, const char *name) {                 \
        auto t = apply_visitor(n, name);                                                \
        n = dynamic_cast<const IR::CLASS *>(t);                                         \
        if (t && !n)                                                                    \
            BUG("visitor returned non-" #CLASS " type: %1%", t); }                      \
    void Visitor::visit(const IR::CLASS *const &n, const char *name) {           \
        /* This function needed solely due to order of declaration issues */            \
        visit(static_cast<const IR::Node *const &>(n), name); }                         \
    void Visitor::visit(const IR::CLASS *&n, const char *name, int cidx) {       \
        ctxt->child_index = cidx;                                                       \
        auto t = apply_visitor(n, name);                                                \
        n = dynamic_cast<const IR::CLASS *>(t);                                         \
        if (t && !n)                                                                    \
            BUG("visitor returned non-" #CLASS " type: %1%", t); }                      \
    void Visitor::visit(const IR::CLASS *const &n, const char *name, int cidx) { \
        /* This function needed solely due to order of declaration issues */            \
        visit(static_cast<const IR::Node *const &>(n), name, cidx); }
    IRNODE_ALL_SUBCLASSES(DEFINE_VISIT_FUNCTIONS)
#undef DEFINE_VISIT_FUNCTIONS

std::ostream &operator<<(std::ostream &out, const IR::Vector<IR::Expression> *v) {
    return v ? out << *v : out << "<null>"; }

#if HAVE_CXXABI_H
#include <cxxabi.h>

cstring Visitor::demangle(const char *str) {
    int status;
    cstring rv;
    if (char *n = abi::__cxa_demangle(str, 0, 0, &status)) {
        rv = n;
        free(n);
    } else {
        rv = str;
    }
    return rv;
}
#else
#warning "No name demangling available; class names in logs will be mangled"
cstring Visitor::demangle(const char *str) {
    return str;
}
#endif
