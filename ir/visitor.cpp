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

class Visitor::ChangeTracker {
    // FIXME -- this code is really incomprehensible due to all the pairs/first/second stuff
    // unfortunatelly maps use pairs all over the place, which is where they come from.
    typedef unordered_map<const IR::Node *, std::pair<bool, const IR::Node *>>  visited_t;
    visited_t           visited;

 public:
    struct change_t {
        bool                                    valid;
        std::pair<visited_t::iterator, bool>    state;  // result of visited.emplace
        change_t() : valid(false) {}
        change_t(visited_t *visited, const IR::Node *n) : valid(true),
            state(visited->emplace(n, std::make_pair(false, n))) {
                if (!state.second && !state.first->second.first)
                    BUG("IR loop detected "); }
        explicit operator bool() { return valid; }
        bool done() { return !state.second; }
        const IR::Node *result() { return state.first->second.second; }
    };
    bool done(const IR::Node *n) const { return visited.count(n) && visited.at(n).first; }
    const IR::Node *result(IR::Node *n) const { return visited.at(n).second; }
    change_t track(const IR::Node *n) { return change_t(&visited, n); }
    void start(change_t &change) { change.state.first->second.first = false; }
    bool finish(change_t &change, const IR::Node *orig, const IR::Node *final) {
        if (!change.valid || (change.state.first = visited.find(orig)) == visited.end())
            BUG("visitor state tracker corrupted");
        change.state.first->second.first = true;
        if (!final || *final != *orig) {
            change.state.first->second.second = final;
            visited.emplace(final, std::make_pair(true, final));
            return true;
        } else {
            return false; } }
    const IR::Node *result(const IR::Node *n) const {
        auto it = visited.find(n);
        if (it == visited.end())
            return n;
        if (!it->second.first) BUG("IR loop detected");
        return it->second.second; }
    void revisit_visited() {
        for (auto it = visited.begin(); it != visited.end();) {
            if (it->second.first)
                it = visited.erase(it);
            else
                ++it; } }
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
        return visited.result(n); }
 public:
    explicit ForwardChildren(const ChangeTracker &v) : visited(v) {}
};
}

const IR::Node *Modifier::apply_visitor(const IR::Node *n, const char *name) {
    if (ctxt) ctxt->child_name = name;
    if (n) {
        PushContext local(ctxt, n);
        auto track = visited->track(n);
        if (track.done() && visitDagOnce) {
            n = track.result();
        } else {
            visited->start(track);
            IR::Node *copy = n->clone();
            local.current.node = copy;
            if (visitDagOnce && !dontForwardChildrenBeforePreorder) {
                ForwardChildren forward_children(*visited);
                copy->visit_children(forward_children); }
            if (copy->apply_visitor_preorder(*this)) {
                copy->visit_children(*this);
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
    if (n && !join_flows(n)) {
        PushContext local(ctxt, n);
        auto vp = visited->emplace(n, false);
        if (!vp.second && !vp.first->second)
            BUG("IR loop detected");
        if (!vp.second && visitDagOnce) {
            // do nothing
        } else {
            vp.first->second = false;
            if (n->apply_visitor_preorder(*this)) {
                n->visit_children(*this);
                n->apply_visitor_postorder(*this); }
            vp.first = visited->find(n);  // iterator may have been invalidated
            if (vp.first == visited->end())
                BUG("visitor state tracker corrupted");
            vp.first->second = true; } }
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
        auto track = visited->track(n);
        if (track.done() && visitDagOnce) {
            n = track.result();
        } else {
            visited->start(track);
            auto copy = n->clone();
            local.current.node = copy;
            if (visitDagOnce && !dontForwardChildrenBeforePreorder) {
                ForwardChildren forward_children(*visited);
                copy->visit_children(forward_children); }
            prune_flag = false;
            auto preorder_result = copy->apply_visitor_preorder(*this);
            ChangeTracker::change_t preorder_result_track;
            assert(preorder_result != n);  // should never happen
            auto final = preorder_result;
            if (!preorder_result) {
                prune_flag = true;
            } else if (preorder_result != copy) {
                if (visited->done(preorder_result) && visitDagOnce) {
                    final = visited->result(preorder_result);
                    prune_flag = true;
                } else {
                    preorder_result_track = visited->track(preorder_result);
                    visited->start(preorder_result_track);
                    local.current.node = copy = preorder_result->clone(); } }
            if (!prune_flag) {
                copy->visit_children(*this);
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

void Inspector::revisit_visited() {
    for (auto it = visited->begin(); it != visited->end();) {
        if (it->second)
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
bool Inspector::preorder(const IR::CLASS *n) {                                          \
    return preorder(static_cast<const IR::BASE *>(n)); }                                \
void Inspector::postorder(const IR::CLASS *n) {                                         \
    postorder(static_cast<const IR::BASE *>(n)); }                                      \
const IR::Node *Transform::preorder(IR::CLASS *n) {                                     \
    return preorder(static_cast<IR::BASE *>(n)); }                                      \
const IR::Node *Transform::postorder(IR::CLASS *n) {                                    \
    return postorder(static_cast<IR::BASE *>(n)); }                                     \

IRNODE_ALL_SUBCLASSES(DEFINE_VISIT_FUNCTIONS)

class SetupJoinPoints : public Inspector {
    map<const IR::Node *, std::pair<ControlFlowVisitor *, int>> &join_points;
    bool preorder(const IR::Node *n) override {
        return ++join_points[n].second == 1; }
 public:
    explicit SetupJoinPoints(decltype(join_points) &fjp)
    : join_points(fjp) { visitDagOnce = false; }
};

void ControlFlowVisitor::init_join_flows(const IR::Node *root) {
    if (!dynamic_cast<Inspector *>(this))
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
        if (!--status.second) {
            flow_merge(*status.first);
            return false;
        } else if (status.first) {
            status.first->flow_merge(*this);
            return true;
        } else {
            status.first = clone();
            return true; } }
    return false;
}
