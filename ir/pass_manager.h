#ifndef _IR_PASS_MANAGER_H_
#define _IR_PASS_MANAGER_H_

#include "visitor.h"

typedef std::function<void(const char* manager, unsigned seqNo,
                           const char* pass, const IR::Node* node)> DebugHook;

class PassManager : virtual public Visitor {
 protected:
    vector<DebugHook>   debugHooks;  // called after each pass
    vector<Visitor *>   passes;
    // if true stops compilation after first pass that signals an error
    bool                stop_on_error = true;
    unsigned            seqNo = 0;
    void addPasses(const std::initializer_list<Visitor *> &init) {
        for (auto p : init) if (p) passes.emplace_back(p); }
    void runDebugHooks(const char* visitorName, const IR::Node* node);
 public:
    PassManager() = default;
    PassManager(const std::initializer_list<Visitor *> &init) : stop_on_error(false)
    { addPasses(init); }
    const IR::Node *apply_visitor(const IR::Node *, const char * = 0) override;
    void setStopOnError(bool stop) { stop_on_error = stop; }
    void addDebugHook(DebugHook h) { debugHooks.push_back(h); }
    void addDebugHooks(std::vector<DebugHook> hooks)
    { debugHooks.insert(debugHooks.end(), hooks.begin(), hooks.end()); }
};

// Repeat a pass until convergence (or up to a fixed number of repeats)
class PassRepeated : virtual public PassManager {
    unsigned            repeats;  // 0 = until convergence
 public:
    PassRepeated(const std::initializer_list<Visitor *> &init) :
            PassManager(init), repeats(0) { setStopOnError(true); }
    const IR::Node *apply_visitor(const IR::Node *, const char * = 0) override;
    PassRepeated *setRepeats(unsigned repeats) { this->repeats = repeats; return this; }
};

// Converts a function Node* -> Node* into a visitor
class VisitFunctor : virtual public Visitor {
    std::function<const IR::Node *(const IR::Node *)>       fn;
    const IR::Node *apply_visitor(const IR::Node *n, const char * = 0) override { return fn(n); }
 public:
    explicit VisitFunctor(std::function<const IR::Node *(const IR::Node *)> f) : fn(f) {}
    explicit VisitFunctor(std::function<void()> f)
    : fn([f](const IR::Node *n)->const IR::Node *{ f(); return n; }) {}
};

class DynamicVisitor : virtual public Visitor {
    Visitor     *visitor;
    profile_t init_apply(const IR::Node *root) {
        if (visitor) return visitor->init_apply(root);
        return Visitor::init_apply(root); }
    void end_apply(const IR::Node *root) {
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
