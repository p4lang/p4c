#ifndef BACKENDS_P4TOOLS_COMMON_LIB_IR_COMPARE_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_IR_COMPARE_H_

#include "ir/ir.h"

namespace P4::IR {

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

/// Hash for StateVariable pointers. We only hash the label.
struct StateVariableHash {
    size_t operator()(const IR::StateVariable *s1) const {
        return Util::Hasher<uint64_t>()(s1->hash());
    }

    size_t operator()(const IR::StateVariable &s1) const {
        return Util::Hasher<uint64_t>()(s1.hash());
    }
};

}  // namespace P4::IR

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_IR_COMPARE_H_ */
