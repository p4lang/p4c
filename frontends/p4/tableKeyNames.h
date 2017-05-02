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

#ifndef _FRONTENDS_P4_TABLEKEYNAMES_H_
#define _FRONTENDS_P4_TABLEKEYNAMES_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {

/** Adds a "@name" annotation to each table key that does not have a name.
 * The string used for the name is derived from the expression itself - if
 * the expression is "simple" enough.  If the expression is not simple the
 * compiler will give an error.  Simple expressions are:
 * - .isValid(),
 * - ArrayIndex,
 * - BAnd (where at least one operand is constant),
 * - Constant,
 * - Member,
 * - PathExpression,
 * - Slice.
 *
 * Examples of control plane names generated from expressions:
 * - `arr[16w5].f` : `@name("arr[5].f")`
 * - `f & 0x3` : `@name("f")`
 * - `.foo` : `@name(".foo")`
 * - `foo.bar` : `@name("foo.bar")`
 * - `f.isValid()` : `@name("f.isValid()")`
 * - `f[3:0]` : `@name("f[3:0]")`
 *
 * @pre This must run before passes that change key expressions, eg. constant
 * folding.  Otherwise the generated control plane names may not match the
 * syntax of the original P4 program.
 * 
 * @post All key fields have `@name` annotations.
 *
 * Emit a compilation error if the program contains complex key expressions
 * without `@name` annotations.
 */
class DoTableKeyNames : public Transform {
    const TypeMap* typeMap;
 public:
    explicit DoTableKeyNames(const TypeMap* typeMap) :typeMap(typeMap)
    { CHECK_NULL(typeMap); setName("DoTableKeyNames"); }
    const IR::Node* postorder(IR::KeyElement* keyElement) override;
};

class TableKeyNames : public PassManager {
 public:
    TableKeyNames(ReferenceMap* refMap, TypeMap* typeMap) {
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(new DoTableKeyNames(typeMap));
        setName("TableKeyNames");
    }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_TABLEKEYNAMES_H_ */
