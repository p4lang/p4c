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

#include "ir/ir.h"
#include "ir/visitor.h"

#include "frontends/p4/typeMap.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"


namespace P4 {

/**
 * Converts non-void extern functions/methods to void
 * and alters them so that the "return" value is stored
 * in the first parameter
 *
 * ie:
 *
 * extern Checksum16<D> {
 *     Checksum16();
 *     bit<16> get<D>(in D data);
 * }
 *
 * Checksum16() ck;
 * some.field = ck.get(other.field);
 *
 * -->
 *
 * extern Checksum16<D> {
 *     Checksum16();
 *     void get<D>(out bit<16> return, in D data);
 * }
 *
 * Checksum16() ck;
 * ck.get(some.field, other.field);
 */
class ConvertToVoid : public Transform {
  private:
    ReferenceMap *refMap;
    TypeMap *typeMap;
    P4CoreLibrary &corelib;

    bool isBuiltInMethod(const IR::MethodCallExpression *mce);
    bool isInCoreLib(const IR::Type_Extern *ex);

  public:
    ConvertToVoid(ReferenceMap *refMap, TypeMap *typeMap) :
            refMap(refMap),
            typeMap(typeMap),
            corelib(P4CoreLibrary::instance) {}
    const IR::Node *postorder(IR::AssignmentStatement *as) override;
    const IR::Node *postorder(IR::Method *method) override;
};

class IsolateMethodCalls : public PassManager {
  public:
    IsolateMethodCalls(ReferenceMap *refMap, TypeMap *typeMap) {
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(new ConvertToVoid(refMap, typeMap));
        setName("IsolateMethodCalls");
    }
};

} // namespace P4

