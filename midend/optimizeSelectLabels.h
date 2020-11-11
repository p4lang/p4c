/*
Copyright 2017 VMware, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http:// www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _MIDEND_OPTIMIZESELECTLABELS_H_
#define _MIDEND_OPTIMIZESELECTLABELS_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

/**
   This class contains methods that optimize select labels.
   If some labels overlaping or one contains the other, user will get warning
   and code will be optimized.
*/

// This struct define node in binarry tree
struct SelectLabel {
    enum {ConstantLabel, RangeLabel} type;
    union {
      big_int constValue;
      std::pair<big_int, big_int> rangeValue;
    };

    int base;
    const IR::Type *typeInfo;
    const IR::Type *rangeConstantTypeInfo;

    SelectLabel(big_int value, int base, const IR::Type *typeInfo) :
                type(SelectLabel::ConstantLabel), constValue(value), base(base),
                typeInfo(typeInfo) {}
    SelectLabel(std::pair<big_int, big_int> value, int base, const IR::Type *typeInfo,
                const IR::Type *rangeConstantTypeInfo) :
                type(SelectLabel::RangeLabel), rangeValue(value), base(base),
                typeInfo(typeInfo), rangeConstantTypeInfo(rangeConstantTypeInfo) {}

    SelectLabel *left, *right;
};

class DoOptimizeSelectLabels : public Transform {
    ReferenceMap* refMap;
    TypeMap* typeMap;

 public:
    explicit DoOptimizeSelectLabels(ReferenceMap* refMap, TypeMap* typeMap) :
                refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap);
        setName("DoOptimizeSelectLabels");
    }
    // Check if range represent range or constant value
    big_int getValidRange(std::pair<big_int, big_int> range);
    // Getter for IR::Range object from std::pair<> value
    IR::Range* getRange(SelectLabel *range);
    SelectLabel* insertValues(SelectLabel* root, big_int value);
    SelectLabel* insertValues(SelectLabel* root, std::pair<big_int, big_int> value);
    // Get all leaf nodes from tree of parsed range and puts in vector elementsOfRange
    void getElementsOfParsedRange(SelectLabel* root, std::vector<SelectLabel*> &elementsOfRange);
    // Creates new SelectCasses and puts them in vector newVecCasses
    void getNewSelectCases(std::vector<SelectLabel*> &elementsOfRange,
                          IR::Vector<IR::SelectCase> &newVecCasses,
                          std::vector<SelectLabel*> &vecOfLabels,
                          const IR::SelectCase* currentExpression);
    // Handler for all optimizations of Select Casses
    const IR::Node* postorder(IR::SelectExpression* expression) override;
};

class OptimizeSelectLabels : public PassManager {
 public:
    explicit OptimizeSelectLabels(ReferenceMap* refMap, TypeMap* typeMap) {
        passes.push_back(new DoOptimizeSelectLabels(refMap, typeMap));
        passes.push_back(new ResolveReferences(refMap));
        passes.push_back(new TypeInference(refMap, typeMap));
        setName("OptimizeSelectLabels");
    }
};

}  // namespace P4

#endif  /* _MIDEND_OPTIMIZESELECTLABELS_H_ */
