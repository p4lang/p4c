/*
 * SPDX-FileCopyrightText: 2024 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IR_COMPARE_H_
#define IR_COMPARE_H_

#include "ir/ir.h"
#include "ir/structural_compare.h"

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
        return s1->label < s2->label;
    }
    bool operator()(const IR::SymbolicVariable &s1, const IR::SymbolicVariable &s2) const {
        return s1.label < s2.label;
    }
};

/// Strict weak ordering for IR keys. Equivalent structures share a key regardless of identity.
struct StructuralLess {
    bool operator()(const IR::Node *s1, const IR::Node *s2) const {
        return IR::structuralCompare(s1, s2) < 0;
    }
    bool operator()(const IR::Node &s1, const IR::Node &s2) const {
        return s1.structuralCompare(s2) < 0;
    }
};

}  // namespace P4::IR

#endif /* IR_COMPARE_H_ */
