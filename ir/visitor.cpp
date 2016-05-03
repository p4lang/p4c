#include <time.h>
#include "ir.h"
#include "lib/log.h"

class Visitor::ChangeTracker {
    // FIXME -- this code is really incomprehensible due to all the pairs/first/second stuff
    // unfortunatelly maps use pairs all over the place, which is where they come from.
    struct info_t {
        bool            done;   // not currently visiting this node or its children
        bool            visitOnce;
        const IR::Node  *changed;
        info_t(bool d, bool v, const IR::Node *c) : done(d), visitOnce(v), changed(c) {} };
    typedef unordered_map<const IR::Node *, info_t>  visited_t;
    visited_t           visited;

 public:
    struct change_t {
        bool            valid;
        info_t          *state;
        change_t() : valid(false) {}
        change_t(visited_t *visited, bool visitOnce, const IR::Node *n) : valid(true) {
            auto t = visited->emplace(n, info_t(false, visitOnce, n));
            state = &t.first->second;
            if (!t.second && !state->done)
                BUG("IR loop detected "); }
        explicit operator bool() const { return valid; }
        bool *visitCurrentOnce() const { return &state->visitOnce; }
        bool done() const { return state->done && state->visitOnce; }
        const IR::Node *result() const { return state->changed; }
    };
    bool done(const IR::Node *n) const {
        auto it = visited.find(n);
        return it != visited.end() && it->second.done && !it->second.visitOnce; }
    const IR::Node *result(IR::Node *n) const { return visited.at(n).changed; }
    change_t track(const IR::Node *n, bool visitOnce) { return change_t(&visited, visitOnce, n); }
    void start(change_t &change) { change.state->done = false; }
    bool finish(change_t &change, const IR::Node *orig, const IR::Node *final) {
        if (!change.valid || change.state != &visited.find(orig)->second)
            BUG("visitor state tracker corrupted");
        change.state->done = true;
        if (!final || *final != *orig) {
            change.state->changed = final;
            visited.emplace(final, info_t(true, true, final));
            return true;
        } else {
            return false; } }
    const IR::Node *result(const IR::Node *n) const {
        auto it = visited.find(n);
        if (it == visited.end() || !it->second.visitOnce)
            return n;
        if (!it->second.done) BUG("IR loop detected");
        return it->second.changed; }
};

Visitor::profile_t Visitor::init_apply(const IR::Node *) {
    if (ctxt) BUG("previous use of visitor did not clean up properly");
    ctxt = nullptr;
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
    start = ts.tv_sec*1000000000UL + ts.tv_nsec;
    ++profile_indent;
}
Visitor::profile_t::profile_t(profile_t &&a) : v(a.v), start(a.start) {
    a.start = 0;
}
Visitor::profile_t::~profile_t() {
    --profile_indent;
    if (start) {
        struct timespec ts;
#ifdef CLOCK_MONOTONIC
        clock_gettime(CLOCK_MONOTONIC, &ts);
#else
        // FIXME -- figure out how to do this on OSX/Mach
        ts.tv_sec = ts.tv_nsec = 0;
#endif
        uint64_t end = ts.tv_sec*1000000000UL + ts.tv_nsec;
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
        out << ctx->node << std::endl;
        ctx = ctx->parent;
    }
}

void Visitor::visitor_const_error() {
    BUG("const Visitor wants to change IR"); }
void Modifier::visitor_const_error() {
    BUG("Modifier called const visit function -- missing template "
                            "instantiation in ir-tree-macro.h?"); }
void Transform::visitor_const_error() {
    BUG("Transform called const visit function -- missing template "
                            "instantiation in ir-tree-macro.h?"); }

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
        return visited.result(n); }
 public:
    explicit ForwardChildren(const ChangeTracker &v) : visited(v) {}
};
}

const IR::Node *Modifier::apply_visitor(const IR::Node *n, const char *name) {
    if (ctxt) ctxt->child_name = name;
    if (n) {
        PushContext local(ctxt, n);
        auto track = visited->track(n, visitDagOnce);
        if (track.done()) {
            n = track.result();
        } else {
            visited->start(track);
            IR::Node *copy = n->clone();
            local.current.node = copy;
            if (!dontForwardChildrenBeforePreorder) {
                ForwardChildren forward_children(*visited);
                copy->visit_children(forward_children); }
            visitCurrentOnce = track.visitCurrentOnce();
            if (copy->apply_visitor_preorder(*this)) {
                copy->visit_children(*this);
                visitCurrentOnce = track.visitCurrentOnce();
                copy->apply_visitor_postorder(*this); }
            if (visited->finish(track, n, copy))
                (n = copy)->validate(); } }
    if (ctxt)
        ctxt->child_index++;
    else
        visited = nullptr;
    return n;
}

const IR::Node *Inspector::apply_visitor(const IR::Node *n, const char *name) {
    if (ctxt) ctxt->child_name = name;
    if (n) {
        PushContext local(ctxt, n);
        auto vp = visited->emplace(n, info_t{false, visitDagOnce});
        if (!vp.second && !vp.first->second.done)
            BUG("IR loop detected");
        if (!vp.second && vp.first->second.visitOnce) {
            // do nothing
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
        auto track = visited->track(n, visitDagOnce);
        if (track.done()) {
            n = track.result();
        } else {
            visited->start(track);
            auto copy = n->clone();
            local.current.node = copy;
            if (!dontForwardChildrenBeforePreorder) {
                ForwardChildren forward_children(*visited);
                copy->visit_children(forward_children); }
            prune_flag = false;
            visitCurrentOnce = track.visitCurrentOnce();
            auto preorder_result = copy->apply_visitor_preorder(*this);
            ChangeTracker::change_t preorder_result_track;
            assert(preorder_result != n);  // should never happen
            auto final = preorder_result;
            if (!preorder_result) {
                prune_flag = true;
            } else if (preorder_result != copy) {
                if (visited->done(preorder_result)) {
                    final = visited->result(preorder_result);
                    prune_flag = true;
                } else {
                    preorder_result_track = visited->track(preorder_result, visitDagOnce);
                    visited->start(preorder_result_track);
                    local.current.node = copy = preorder_result->clone(); } }
            if (!prune_flag) {
                copy->visit_children(*this);
                visitCurrentOnce = track.visitCurrentOnce();
                final = copy->apply_visitor_postorder(*this); }
            if (final && final != preorder_result && *final == *preorder_result)
                final = preorder_result;
            if (visited->finish(track, n, final) && (n = final))
                final->validate();
            if (preorder_result_track)
                visited->finish(preorder_result_track, preorder_result, final); } }
    if (ctxt)
        ctxt->child_index++;
    else
        visited = nullptr;
    return n;
}

#define DEFINE_VISIT_FUNCTIONS(CLASS, BASE)                                             \
bool Modifier::preorder(IR::CLASS *n) {                                                 \
    return preorder(static_cast<IR::BASE *>(n)); }                                      \
void Modifier::postorder(IR::CLASS *n) {                                                \
    postorder(static_cast<IR::BASE *>(n)); }                                            \
bool Inspector::preorder(const IR::CLASS *n) {                                          \
    return preorder(static_cast<const IR::BASE *>(n)); }                                \
void Inspector::postorder(const IR::CLASS *n) {                                         \
    postorder(static_cast<const IR::BASE *>(n)); }                                      \
const IR::Node *Transform::preorder(IR::CLASS *n) {                                     \
    return preorder(static_cast<IR::BASE *>(n)); }                                      \
const IR::Node *Transform::postorder(IR::CLASS *n) {                                    \
    return postorder(static_cast<IR::BASE *>(n)); }                                     \

IRNODE_ALL_SUBCLASSES(DEFINE_VISIT_FUNCTIONS)

