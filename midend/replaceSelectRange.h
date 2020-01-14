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
#ifndef _MIDEND_REPLACESELECTRANGE_H_
#define _MIDEND_REPLACESELECTRANGE_H_

#include <iostream>
#include <iomanip>
#include <vector>
#include <utility>
#include "../lib/gmputil.h"

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

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

    explicit DoReplaceSelectRange(uint max) : MAX_CASES(max) {
        setName("DoReplaceSelectRange");
    }
    const IR::Node* postorder(IR::SelectCase* p) override;
    std::vector<const IR::Mask *> rangeToMasks(const IR::Range *);
    std::vector<IR::Vector<IR::Expression>> cartesianAppend(
               const std::vector<IR::Vector<IR::Expression>>& vecs,
               const std::vector<const IR::Mask *>& masks);
    std::vector<IR::Vector<IR::Expression>> cartesianAppend(
               const std::vector<IR::Vector<IR::Expression>>& vecs,
               const IR::Expression *e);
};

class ReplaceSelectRange final : public PassManager {
 public:
        ReplaceSelectRange(ReferenceMap* refMap, TypeMap* typeMap) {
            passes.push_back(new TypeChecking(refMap, typeMap));
            passes.push_back(new DoReplaceSelectRange(100));
            setName("ReplaceSelectRange");
        }
};

}    // namespace P4
#endif /* _MIDEND_REPLACESELECTRANGE_H_ */
