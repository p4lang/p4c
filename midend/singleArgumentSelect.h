/*
Copyright 2018 VMware, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef MIDEND_SINGLEARGUMENTSELECT_H_
#define MIDEND_SINGLEARGUMENTSELECT_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/**
   Converts select(a, b, c) into select(a ++ b ++ c) &&& (maska ++ maskb ++ maskc).
   A similar transformation is done for the labels.
   @pre
   This should be run after SimplifySelectList and RemoveSelectBooleans.
   It assumes that all select arguments are scalar values of type Type_Bits.
*/
class DoSingleArgumentSelect : public Modifier {
    TypeMap *typeMap;
    const IR::Type *selectListType;

 public:
    explicit DoSingleArgumentSelect(TypeMap *typeMap) : typeMap(typeMap), selectListType(nullptr) {
        setName("DoSingleArgumentSelect");
    }

    /// A pair of expression representing an expression and a mask
    struct Pair {
        const IR::Expression *expr;
        const IR::Expression *mask;
        bool hasMask;

        Pair(const IR::Expression *source, const IR::Type *type);
    };

    // Validate that the expression contains only subexpressions
    // of supported types.
    void checkExpressionType(const IR::Expression *expression);

    bool preorder(IR::SelectCase *selCase) override;
    bool preorder(IR::SelectExpression *expression) override;
};

class SingleArgumentSelect : public PassManager {
 public:
    SingleArgumentSelect(ReferenceMap *refMap, TypeMap *typeMap,
                         TypeChecking *typeChecking = nullptr) {
        if (!typeChecking) typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoSingleArgumentSelect(typeMap));
        setName("SingleArgumentSelect");
    }
};

}  // namespace P4

#endif /* MIDEND_SINGLEARGUMENTSELECT_H_ */
