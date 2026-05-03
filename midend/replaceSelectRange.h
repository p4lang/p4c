/*
 * Copyright 2020 MNK Labs & Consulting
 * SPDX-FileCopyrightText: 2020 MNK Labs & Consulting
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef MIDEND_REPLACESELECTRANGE_H_
#define MIDEND_REPLACESELECTRANGE_H_

#include <set>
#include <vector>

#include "ir/ir.h"

namespace P4 {

/**
  Find the types to replace and insert them in the range-mask map.
  Call pass after the flattenInterfaceStructs pass.
  */
class ReplaceSelectRange : public Transform {
 public:
    // number of new cases generated for ternary operations due to a range
    // Each case is a key set expression.
    const unsigned MAX_CASES;
    // An index i is in this set if selectExpression->components[i] needs to be
    // cast from int to bit. This is needed if and only if the expression at
    // position i is of int type and there is a label that has in the i-th
    // position a range expression. This is needed since we replace ranges with
    // masks and masks are only defined for signed types.
    std::set<size_t> signedIndicesToReplace;

    explicit ReplaceSelectRange(unsigned max = 100) : MAX_CASES(max) {
        setName("DoReplaceSelectRange");
    }

    const IR::Node *postorder(IR::SelectExpression *e) override;
    const IR::Node *postorder(IR::SelectCase *p) override;

    std::vector<const IR::Mask *> *rangeToMasks(const IR::Range *, size_t);
    std::vector<IR::Vector<IR::Expression>> cartesianAppend(
        const std::vector<IR::Vector<IR::Expression>> &vecs,
        const std::vector<const IR::Mask *> &masks);
    std::vector<IR::Vector<IR::Expression>> cartesianAppend(
        const std::vector<IR::Vector<IR::Expression>> &vecs, const IR::Expression *e);
};

}  // namespace P4

#endif /* MIDEND_REPLACESELECTRANGE_H_ */
