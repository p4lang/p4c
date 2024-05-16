/*
 * SPDX-FileCopyrightText: 2024 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IR_COMPARE_H_
#define IR_COMPARE_H_

#include "ir/ir.h"
#include "lib/hash.h"

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

/// Hash of symbolic variable labels, ignoring types. Use with SymbolicVariableEqual.
struct SymbolicVariableHash {
    size_t operator()(const IR::SymbolicVariable *s1) const { return Util::Hash{}(s1->label); }

    size_t operator()(const IR::SymbolicVariable &s1) const { return Util::Hash{}(s1.label); }
};

}  // namespace P4::IR

#endif /* IR_COMPARE_H_ */
