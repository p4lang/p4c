#ifndef IR_COMPARE_H_
#define IR_COMPARE_H_

#include "ir/ir.h"

namespace P4::IR {

/// Equals for SymbolicVariable pointers. We only compare the label.
struct SymbolicVariableEqual {
    bool operator()(const IR::SymbolicVariable *s1, const IR::SymbolicVariable *s2) const {
        return s1->label == s2->label;
    }
    bool operator()(const IR::SymbolicVariable &s1, const IR::SymbolicVariable &s2) const {
        return s1.label == s2.label;
    }
};

/// Less for SymbolicVariable pointers. We only compare the label.
struct SymbolicVariableLess {
    bool operator()(const IR::SymbolicVariable *s1, const IR::SymbolicVariable *s2) const {
        return s1->isSemanticallyLess(*s2);
    }
    bool operator()(const IR::SymbolicVariable &s1, const IR::SymbolicVariable &s2) const {
        return s1.isSemanticallyLess(s2);
    }
};

struct IsSemanticallyLessComparator {
    bool operator()(const IR::Node *s1, const IR::Node *s2) const {
        return s1->isSemanticallyLess(*s2);
    }
    bool operator()(const IR::Node &s1, const IR::Node &s2) const {
        return s1.isSemanticallyLess(s2);
    }
};

}  // namespace P4::IR

#endif /* IR_COMPARE_H_ */
