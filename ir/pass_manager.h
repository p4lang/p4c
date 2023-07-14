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

#ifndef IR_PASS_MANAGER_H_
#define IR_PASS_MANAGER_H_

#include <functional>
#include <initializer_list>
#include <iosfwd>
#include <type_traits>
#include <vector>

#include "ir/node.h"
#include "ir/visitor.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/safe_vector.h"

typedef std::function<void(const char *manager, unsigned seqNo, const char *pass,
                           const IR::Node *node)>
    DebugHook;

class PassManager : virtual public Visitor, virtual public Backtrack {
    bool early_exit_flag = false;
    mutable int never_backtracks_cache = -1;

 protected:
    safe_vector<DebugHook> debugHooks;  // called after each pass
    safe_vector<Visitor *> passes;
    // if true stops compilation after first pass that signals an error
    bool stop_on_error = true;
    bool running = false;
    unsigned seqNo = 0;
    void runDebugHooks(const char *visitorName, const IR::Node *node);
    profile_t init_apply(const IR::Node *root) override {
        running = true;
        return Visitor::init_apply(root);
    }

 public:
    PassManager() = default;
    PassManager(const PassManager &) = default;
    PassManager(PassManager &&) = default;
    class VisitorRef {
        Visitor *visitor;
        friend class PassManager;

     public:
        VisitorRef() : visitor(nullptr) {}
        VisitorRef(Visitor *v) : visitor(v) {}               // NOLINT(runtime/explicit)
        VisitorRef(const Visitor &v) : visitor(v.clone()) {  // NOLINT(runtime/explicit)
            BUG_CHECK(visitor->check_clone(&v), "Incorrect clone in PassManager");
        }
        explicit VisitorRef(std::function<const IR::Node *(const IR::Node *)>);
        // ambiguity resolution converters -- allow different lambda signatures
        template <class T>
        VisitorRef(
            T t,
            typename std::enable_if<
                std::is_convertible<decltype(t(nullptr)), const IR::Node *>::value, int>::type = 0)
            : VisitorRef(std::function<const IR::Node *(const IR::Node *)>(t)) {}
        template <class T>
        VisitorRef(
            T t,
            typename std::enable_if<std::is_same<decltype(t(nullptr)), void>::value, int>::type = 0)
            : VisitorRef(std::function<const IR::Node *(const IR::Node *)>(
                  [t](const IR::Node *n) -> const IR::Node * {
                      t(n);
                      return n;
                  })) {}
        template <class T>
        VisitorRef(T t,
                   typename std::enable_if<std::is_same<decltype(t()), void>::value, int>::type = 0)
            : VisitorRef(std::function<const IR::Node *(const IR::Node *)>(
                  [t](const IR::Node *n) -> const IR::Node * {
                      t();
                      return n;
                  })) {}
    };
    PassManager(const std::initializer_list<VisitorRef> &init) { addPasses(init); }
    void addPasses(const std::initializer_list<VisitorRef> &init) {
        never_backtracks_cache = -1;
        for (auto &p : init)
            if (p.visitor) passes.emplace_back(p.visitor);
    }
    void removePasses(const std::vector<cstring> &exclude);
    void listPasses(std::ostream &, cstring sep) const;
    const IR::Node *apply_visitor(const IR::Node *, const char * = 0) override;
    bool backtrack(trigger &trig) override;
    bool never_backtracks() override;
    void setStopOnError(bool stop) { stop_on_error = stop; }
    void addDebugHook(DebugHook h, bool recursive = false) {
        debugHooks.push_back(h);
        if (recursive)
            for (auto pass : passes)
                if (auto child = dynamic_cast<PassManager *>(pass))
                    child->addDebugHook(h, recursive);
    }
    void addDebugHooks(std::vector<DebugHook> hooks, bool recursive = false) {
        debugHooks.insert(debugHooks.end(), hooks.begin(), hooks.end());
        if (recursive)
            for (auto pass : passes)
                if (auto child = dynamic_cast<PassManager *>(pass))
                    child->addDebugHooks(hooks, recursive);
    }
    void early_exit() { early_exit_flag = true; }
    PassManager *clone() const override { return new PassManager(*this); }
};

template <class T>
class OnBacktrack : virtual public Visitor, virtual public Backtrack {
    std::function<void(T *)> fn;

 public:
    explicit OnBacktrack(std::function<void(T *)> f) : fn(f) {}
    const IR::Node *apply_visitor(const IR::Node *n, const char * = 0) override { return n; }
    bool backtrack(trigger &trig) override {
        if (auto *t = dynamic_cast<T *>(&trig)) {
            fn(t);
            return true;
        } else {
            return false;
        }
    }
};

// Repeat a pass until convergence (or up to a fixed number of repeats)
class PassRepeated : virtual public PassManager {
    unsigned repeats;  // 0 = until convergence
 public:
    PassRepeated() : repeats(0) {}
    PassRepeated(const std::initializer_list<VisitorRef> &init, unsigned repeats = 0)
        : PassManager(init), repeats(repeats) {}
    const IR::Node *apply_visitor(const IR::Node *, const char * = 0) override;
    PassRepeated *setRepeats(unsigned repeats) {
        this->repeats = repeats;
        return this;
    }
    PassRepeated *clone() const override { return new PassRepeated(*this); }
};

class PassRepeatUntil : virtual public PassManager {
    std::function<bool()> done;

 public:
    explicit PassRepeatUntil(std::function<bool()> done) : done(done) {}
    PassRepeatUntil(const std::initializer_list<VisitorRef> &init, std::function<bool()> done)
        : PassManager(init), done(done) {}
    const IR::Node *apply_visitor(const IR::Node *, const char * = 0) override;
    PassRepeatUntil *clone() const override { return new PassRepeatUntil(*this); }
};

class PassIf : virtual public PassManager {
    std::function<bool()> cond;

 public:
    explicit PassIf(std::function<bool()> cond) : cond(cond) {}
    PassIf(std::function<bool()> cond, const std::initializer_list<VisitorRef> &init)
        : PassManager(init), cond(cond) {}
    const IR::Node *apply_visitor(const IR::Node *, const char * = 0) override;
    PassIf *clone() const override { return new PassIf(*this); }
};

// Converts a function Node* -> Node* into a visitor
class VisitFunctor : virtual public Visitor {
    std::function<const IR::Node *(const IR::Node *)> fn;
    const IR::Node *apply_visitor(const IR::Node *n, const char * = 0) override { return fn(n); }

 public:
    explicit VisitFunctor(std::function<const IR::Node *(const IR::Node *)> f) : fn(f) {}
    explicit VisitFunctor(std::function<void()> f)
        : fn([f](const IR::Node *n) -> const IR::Node * {
              f();
              return n;
          }) {}

    VisitFunctor *clone() const override { return new VisitFunctor(*this); }
};

inline PassManager::VisitorRef::VisitorRef(std::function<const IR::Node *(const IR::Node *)> fn)
    : visitor(new VisitFunctor(fn)) {}

class DynamicVisitor : virtual public Visitor {
    Visitor *visitor;
    profile_t init_apply(const IR::Node *root) override {
        if (visitor) return visitor->init_apply(root);
        return Visitor::init_apply(root);
    }
    void end_apply(const IR::Node *root) override {
        if (visitor) visitor->end_apply(root);
    }
    const IR::Node *apply_visitor(const IR::Node *root, const char *name = 0) override {
        if (visitor) return visitor->apply_visitor(root, name);
        return root;
    }

 public:
    DynamicVisitor() : visitor(nullptr) {}
    explicit DynamicVisitor(Visitor *v) : visitor(v) {}
    void setVisitor(Visitor *v) { visitor = v; }
    DynamicVisitor *clone() const override { return new DynamicVisitor(*this); }
};

#endif /* IR_PASS_MANAGER_H_ */
