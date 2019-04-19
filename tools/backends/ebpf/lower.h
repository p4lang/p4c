/*
Copyright 2017 VMware, Inc.

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

#ifndef _BACKENDS_EBPF_LOWER_H_
#define _BACKENDS_EBPF_LOWER_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace EBPF {

/**
  This pass rewrites expressions which are not supported natively on EBPF.
*/
class LowerExpressions : public Transform {
    P4::TypeMap* typeMap;
    // Cannot shift with a value larger than 5 bits
    const int maxShiftWidth = 5;
    const IR::Expression* shift(const IR::Operation_Binary* expression) const;
 public:
    explicit LowerExpressions(P4::TypeMap* typeMap) : typeMap(typeMap)
    { CHECK_NULL(typeMap); setName("Lower"); }

    const IR::Node* postorder(IR::Shl* expression) override
    { return shift(expression); }
    const IR::Node* postorder(IR::Shr* expression) override
    { return shift(expression); }
    const IR::Node* postorder(IR::Expression* expression) override;
    const IR::Node* postorder(IR::Slice* expression) override;
    const IR::Node* postorder(IR::Concat* expression) override;
    const IR::Node* postorder(IR::Cast* expression) override;
};

class Lower : public PassManager {
 public:
    Lower(P4::ReferenceMap* refMap, P4::TypeMap* typeMap) {
        setName("Lower");
        passes.push_back(new P4::TypeChecking(refMap, typeMap));
        passes.push_back(new LowerExpressions(typeMap));
    }
};

}  // namespace EBPF

#endif  /* _BACKENDS_EBPF_LOWER_H_ */
