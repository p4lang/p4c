/*
Copyright 2013-present Barefoot Networks, Inc.

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

#ifndef _FRONTENDS_P4_RESETHEADERS_H_
#define _FRONTENDS_P4_RESETHEADERS_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace P4 {

/** @brief Explicitly invalidate uninitialized header variables declared in
 * parser states.
 *
 * A local uninitialized variable in a parser state that represents a header
 * must be invalid every time the parser state is entered.  Hence,
 *
```
state X {
    H h;
    p.extract(h);
}
```
 *
 * becomes
 *
```
state X {
    H h;
    h.setInvalid();
    p.extract(h);
}
```
 *
 * This pass also handles header fields in variables of derived types, like
 * structs and unions.
 *
 * @pre An up-to-date TypeMap.
 *
 * @post All parser states with uninitialized header variables have explicit
 * statements that invalidate those headers.
 */
class DoResetHeaders : public Transform {
    const TypeMap* typeMap;

 public:
    static void generateResets(
        const TypeMap* typeMap, const IR::Type* type,
        const IR::Expression* expr, IR::Vector<IR::StatOrDecl>* resets);
    explicit DoResetHeaders(const TypeMap* typeMap) : typeMap(typeMap) {
        CHECK_NULL(typeMap); setName("DoResetHeaders"); }
    const IR::Node* postorder(IR::Declaration_Variable* decl) override;
};

/// Invokes TypeChecking followed by DoResetHeaders.
class ResetHeaders : public PassManager {
 public:
    ResetHeaders(ReferenceMap* refMap, TypeMap* typeMap) {
        passes.push_back(new P4::TypeChecking(refMap, typeMap));
        passes.push_back(new P4::DoResetHeaders(typeMap));
        setName("ResetHeaders");
    }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_RESETHEADERS_H_ */
