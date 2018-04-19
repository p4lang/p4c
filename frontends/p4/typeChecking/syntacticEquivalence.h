/*
Copyright 2016 VMware, Inc.

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

#ifndef _TYPECHECKING_SYNTACTICEQUIVALENCE_H_
#define _TYPECHECKING_SYNTACTICEQUIVALENCE_H_

#include "ir/ir.h"
#include "frontends/p4/typeMap.h"
#include "frontends/common/resolveReferences/referenceMap.h"

namespace P4 {

// Check if two expressions are syntactically equivalent
class SameExpression {
    const ReferenceMap* refMap;
    const TypeMap* typeMap;
 public:
    explicit SameExpression(const ReferenceMap* refMap, const TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap) { CHECK_NULL(refMap); CHECK_NULL(typeMap); }
    bool sameType(const IR::Type* left, const IR::Type* right) const;
    bool sameExpression(const IR::Expression* left, const IR::Expression* right) const;
    bool sameExpressions(const IR::Vector<IR::Expression>* left,
                         const IR::Vector<IR::Expression>* right) const;
    bool sameExpressions(const IR::Vector<IR::Argument>* left,
                         const IR::Vector<IR::Argument>* right) const;
};

}  // namespace P4

#endif /* _TYPECHECKING_SYNTACTICEQUIVALENCE_H_ */
