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
        return *s1 < *s2;
    }
    bool operator()(const IR::SymbolicVariable &s1, const IR::SymbolicVariable &s2) const {
        return s1 < s2;
    }
};

/// Hash for SymbolicVariable pointers. We only hash the label.
struct SymbolicVariableHash {
    size_t operator()(const IR::SymbolicVariable *s1) const {
        return std::hash<cstring>()(s1->label);
    }

    size_t operator()(const IR::SymbolicVariable &s1) const {
        return std::hash<cstring>()(s1.label);
    }
};

}  // namespace P4::IR

#endif /* IR_COMPARE_H_ */
