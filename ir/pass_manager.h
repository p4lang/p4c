#ifndef _IR_PASS_MANAGER_H_
#define _IR_PASS_MANAGER_H_

#include "visitor.h"

class PassManager : virtual public Visitor {
 protected:
    vector<Visitor *>   passes;
    // if true stops compilation after first pass that signals an error
    bool                stop_on_error = false; 
 public:
    PassManager() = default;
    PassManager(const std::initializer_list<Visitor *> &init) :
        stop_on_error(false) {
        for (auto p : init) if (p) passes.emplace_back(p); }
    const IR::Node *apply_visitor(const IR::Node *, const char * = 0) override;
    void setStopOnError(bool stop) { stop_on_error = stop; }
};

// Repeat a pass until convergence (or up to a fixed number of repeats)
class PassRepeated : virtual public PassManager {
    unsigned            repeats;  // 0 = until convergence
 public:
    PassRepeated(const std::initializer_list<Visitor *> &init) :
            PassManager(init), repeats(0) { setStopOnError(true); }
    const IR::Node *apply_visitor(const IR::Node *, const char * = 0) override;
    void setRepeats(unsigned repeats) { this->repeats = repeats; }
};

class VisitFunctor : virtual public Visitor {
    std::function<void()>       fn;
    const IR::Node *apply_visitor(const IR::Node *n, const char * = 0) override {
        fn(); return n; }
 public:
    explicit VisitFunctor(std::function<void()> f) : fn(f) {}
};

#endif /* _IR_PASS_MANAGER_H_ */
