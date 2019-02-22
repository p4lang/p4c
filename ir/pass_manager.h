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

#ifndef _IR_PASS_MANAGER_H_
#define _IR_PASS_MANAGER_H_

#include "visitor.h"

typedef std::function<void(const char* manager, unsigned seqNo,
                           const char* pass, const IR::Node* node)> DebugHook;

class PassManager : virtual public Visitor, virtual public Backtrack {
    bool early_exit_flag;
    mutable int never_backtracks_cache = -1;

 protected:
    safe_vector<DebugHook>   debugHooks;  // called after each pass
    safe_vector<Visitor *>   passes;
    // if true stops compilation after first pass that signals an error
    bool                stop_on_error = true;
    bool                running = false;
    unsigned            seqNo = 0;
    void runDebugHooks(const char* visitorName, const IR::Node* node);
    profile_t init_apply(const IR::Node *root) override {
        running = true;
        return Visitor::init_apply(root); }

 public:
    PassManager() = default;
    PassManager(const std::initializer_list<Visitor *> &init)
    { addPasses(init); }
    void addPasses(const std::initializer_list<Visitor *> &init) {
        never_backtracks_cache = -1;
        for (auto p : init) if (p) passes.emplace_back(p); }
    const IR::Node *apply_visitor(const IR::Node *, const char * = 0) override;
    bool backtrack(trigger &trig) override;
    bool never_backtracks() override;
    void setStopOnError(bool stop) { stop_on_error = stop; }
    void addDebugHook(DebugHook h, bool recursive = false) {
        debugHooks.push_back(h);
        if (recursive)
            for (auto pass : passes)
                if (auto child = dynamic_cast<PassManager *>(pass))
                    child->addDebugHook(h, recursive); }
    void addDebugHooks(std::vector<DebugHook> hooks, bool recursive = false) {
        debugHooks.insert(debugHooks.end(), hooks.begin(), hooks.end());
        if (recursive)
            for (auto pass : passes)
                if (auto child = dynamic_cast<PassManager *>(pass))
                    child->addDebugHooks(hooks, recursive); }
    void early_exit() { early_exit_flag = true; }
};

// Repeat a pass until convergence (or up to a fixed number of repeats)
class PassRepeated : virtual public PassManager {
    unsigned            repeats;  // 0 = until convergence
 public:
    PassRepeated() : repeats(0) {}
    PassRepeated(const std::initializer_list<Visitor *> &init) :
            PassManager(init), repeats(0) {}
    const IR::Node *apply_visitor(const IR::Node *, const char * = 0) override;
    PassRepeated *setRepeats(unsigned repeats) { this->repeats = repeats; return this; }
};

class PassRepeatUntil : virtual public PassManager {
    std::function<bool()>       done;
 public:
    explicit PassRepeatUntil(std::function<bool()> done) : done(done) {}
    PassRepeatUntil(const std::initializer_list<Visitor *> &init,
                    std::function<bool()> done)
    : PassManager(init), done(done) {}
    const IR::Node *apply_visitor(const IR::Node *, const char * = 0) override;
};

// Converts a function Node* -> Node* into a visitor
class VisitFunctor : virtual public Visitor {
    std::function<const IR::Node *(const IR::Node *)>       fn;
    const IR::Node *apply_visitor(const IR::Node *n, const char * = 0) override { return fn(n); }
 public:
    explicit VisitFunctor(std::function<const IR::Node *(const IR::Node *)> f) : fn(f)
    { setName("VisitFunctor"); }
    explicit VisitFunctor(std::function<void()> f)
    : fn([f](const IR::Node *n)->const IR::Node *{ f(); return n; }) { setName("VisitFunctor"); }
};

class DynamicVisitor : virtual public Visitor {
    Visitor     *visitor;
    profile_t init_apply(const IR::Node *root) override {
        if (visitor) return visitor->init_apply(root);
        return Visitor::init_apply(root); }
    void end_apply(const IR::Node *root) override {
        if (visitor) visitor->end_apply(root); }
    const IR::Node *apply_visitor(const IR::Node *root, const char *name = 0) override {
        if (visitor) return visitor->apply_visitor(root, name);
        return root; }
 public:
    DynamicVisitor() : visitor(nullptr) {}
    explicit DynamicVisitor(Visitor *v) : visitor(v) {}
    void setVisitor(Visitor *v) { visitor = v; }
};

#endif /* _IR_PASS_MANAGER_H_ */
