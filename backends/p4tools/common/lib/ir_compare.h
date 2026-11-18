/*
 * SPDX-FileCopyrightText: 2024 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_P4TOOLS_COMMON_LIB_IR_COMPARE_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_IR_COMPARE_H_

#include "ir/ir.h"

namespace P4::IR {

/// Equality of state references, ignoring types.
struct StateVariableEqual {
    bool operator()(const IR::StateVariable *s1, const IR::StateVariable *s2) const {
        return *s1 == *s2;
    }
    bool operator()(const IR::StateVariable &s1, const IR::StateVariable &s2) const {
        return s1 == s2;
    }
};

/// Ordering of state references, ignoring types.
struct StateVariableLess {
    bool operator()(const IR::StateVariable *s1, const IR::StateVariable *s2) const {
        return *s1 < *s2;
    }
    bool operator()(const IR::StateVariable &s1, const IR::StateVariable &s2) const {
        return s1 < s2;
    }
};

/// Hash of state references, ignoring types. Use with StateVariableEqual.
struct StateVariableHash {
    size_t operator()(const IR::StateVariable *s1) const { return s1->hash(); }

    size_t operator()(const IR::StateVariable &s1) const { return s1.hash(); }
};

}  // namespace P4::IR

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_IR_COMPARE_H_ */
