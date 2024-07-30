#ifndef BACKENDS_P4TOOLS_COMMON_LIB_IR_COMPARE_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_IR_COMPARE_H_

#include "ir/ir.h"

namespace IR {

/// Equals for StateVariable pointers. We only compare the label.
struct StateVariableEqual {
    bool operator()(const IR::StateVariable *s1, const IR::StateVariable *s2) const {
        return s1->equiv(*s2);
    }
    bool operator()(const IR::StateVariable &s1, const IR::StateVariable &s2) const {
        return s1.equiv(s2);
    }
};

/// Less for StateVariable pointers. We only compare the label.
struct StateVariableLess {
    bool operator()(const IR::StateVariable *s1, const IR::StateVariable *s2) const {
        return *s1 < *s2;
    }
    bool operator()(const IR::StateVariable &s1, const IR::StateVariable &s2) const {
        return s1 < s2;
    }
};

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
        return *s1 < *s2;
    }
    bool operator()(const IR::SymbolicVariable &s1, const IR::SymbolicVariable &s2) const {
        return s1 < s2;
    }
};

}  // namespace IR
#endif /* BACKENDS_P4TOOLS_COMMON_LIB_IR_COMPARE_H_ */
