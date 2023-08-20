/*
 * Copyright 2020, MNK Labs & Consulting
 * http://mnkcg.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifndef MIDEND_REPLACESELECTRANGE_H_
#define MIDEND_REPLACESELECTRANGE_H_

#include <iomanip>
#include <iostream>
#include <utility>
#include <vector>

#include "../lib/big_int_util.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/**
  Find the types to replace and insert them in the range-mask map.
  Call pass after the flattenInterfaceStructs pass.
  */
class DoReplaceSelectRange : public Transform {
 public:
    // number of new cases generated for ternary operations due to a range
    // Each case is a key set expression.
    const uint MAX_CASES;
    // An index i is in this set if selectExpression->components[i] needs to be
    // cast from int to bit. This is needed if and only if the expression at
    // position i is of int type and there is a label that has in the i-th
    // position a range expression. This is needed since we replace ranges with
    // masks and masks are only defined for signed types.
    std::set<size_t> signedIndicesToReplace;

    explicit DoReplaceSelectRange(uint max) : MAX_CASES(max) { setName("DoReplaceSelectRange"); }

    const IR::Node *postorder(IR::SelectExpression *e) override;
    const IR::Node *postorder(IR::SelectCase *p) override;

    std::vector<const IR::Mask *> *rangeToMasks(const IR::Range *, size_t);
    std::vector<IR::Vector<IR::Expression>> cartesianAppend(
        const std::vector<IR::Vector<IR::Expression>> &vecs,
        const std::vector<const IR::Mask *> &masks);
    std::vector<IR::Vector<IR::Expression>> cartesianAppend(
        const std::vector<IR::Vector<IR::Expression>> &vecs, const IR::Expression *e);
};

class ReplaceSelectRange final : public PassManager {
 public:
    ReplaceSelectRange(ReferenceMap *refMap, TypeMap *typeMap) {
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(new DoReplaceSelectRange(100));
        setName("ReplaceSelectRange");
    }
};

}  // namespace P4
#endif /* MIDEND_REPLACESELECTRANGE_H_ */
