/*
Copyright 2020 VMware, Inc.

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

#ifndef _FRONTENDS_P4_SPECIALIZEGENERICFUNCTIONS_H_
#define _FRONTENDS_P4_SPECIALIZEGENERICFUNCTIONS_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

/// Describes how a generic function is specialized.
struct FunctionSpecialization {
    /// Name to use for specialized function.
    cstring name;
    /// Function that is being specialized
    const IR::Function*                specialized;
    /// Invocation which causes this specialization.
    const IR::MethodCallExpression*    invocation;

    FunctionSpecialization(cstring name,
                           const IR::MethodCallExpression* invocation,
                           const IR::Function* function):
            name(name), specialized(function), invocation(invocation)
    { CHECK_NULL(invocation); }
};

struct FunctionSpecializationMap {
    ReferenceMap* refMap;
    TypeMap* typeMap;
    ordered_map<const IR::MethodCallExpression*, FunctionSpecialization*> map;

    void add(const IR::MethodCallExpression* mce, const IR::Function* func) {
        cstring name = refMap->newName(func->name);
        FunctionSpecialization* fs = new FunctionSpecialization(name, mce, func);
        map.emplace(mce, fs);
    }
    FunctionSpecialization* get(const IR::MethodCallExpression* mce) const {
        return ::get(map, mce);
    }
};

/**
 * Find all generic function invocations and their type arguments.
 */
class FindFunctionSpecializations : public Inspector {
    FunctionSpecializationMap* specMap;
 public:
    explicit FindFunctionSpecializations(FunctionSpecializationMap* specMap) :
            specMap(specMap) {
        CHECK_NULL(specMap);
        setName("FindFunctionSpecializations");
    }

    bool preorder(const IR::MethodCallExpression* call) override;
};

/** @brief Specializes each generic function by substituting type parameters.
 *
 * For example:
 *
```
T f<T>(in T data) { return data; }
...
bit<32> b = f(32w0);
```
 *
 * generates the following extra code:
 *
```
bit<32> f_0(in bit<32> data) { return data; }
...
bit<32> b = f_0(32w0);
```
 * A different specialization is made for each function invocation.
 */
class SpecializeFunctions : public Transform {
    FunctionSpecializationMap* specMap;
 public:
    explicit SpecializeFunctions(FunctionSpecializationMap* specMap) : specMap(specMap)
    { CHECK_NULL(specMap); setName("SpecializeFunctions"); }
    const IR::Node* postorder(IR::Function* function) override;
    const IR::Node* postorder(IR::MethodCallExpression*) override;
};

class SpecializeGenericFunctions : public PassManager {
    FunctionSpecializationMap specMap;
 public:
    SpecializeGenericFunctions(ReferenceMap* refMap, TypeMap* typeMap) {
        passes.emplace_back(new TypeChecking(refMap, typeMap));
        passes.emplace_back(new FindFunctionSpecializations(&specMap));
        passes.emplace_back(new SpecializeFunctions(&specMap));
        specMap.refMap = refMap;
        specMap.typeMap = typeMap;
        setName("SpecializeGenericFunctions");
    }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_SPECIALIZEGENERICFUNCTIONS_H_ */
