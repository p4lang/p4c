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

#ifndef _MIDEND_SPECIALIZE_H_
#define _MIDEND_SPECIALIZE_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

// Specializes Parsers and Controls by substituting type arguments
// and constructor parameters.
// control c<T>(in T data)(bit<32> size) { ... }
// c<bit<32>>(16) c_inst;
// is converted to
// control cspec(in bit<32> data) { ... }
// cspec() c_inst;
// A different specialization is made for each constructor invocation
// and Declaration_Instance.

struct SpecializationInfo {
    const IR::Vector<IR::Type>*       typeArguments;
    const IR::Vector<IR::Expression>* constructorArguments;
    const IR::Node*                   invocation;
    cstring name;
};

struct SpecializationMap {
    const TypeMap*                     typeMap;
    const ReferenceMap*                refMap;
    // For each ConstructorCallExpression or Declaration_Instance
    // we allocate a new name
    std::map<const IR::Node*, cstring> newName;
    std::vector<SpecializationInfo*>* getSpecializations(const IR::IContainer* decl);
    bool isSpecialized(const IR::Node* node) const
    { CHECK_NULL(node); return newName.find(node) != newName.end(); }
    cstring get(const IR::Node* node) const
    { CHECK_NULL(node); return ::get(newName, node); }
    void clear() { newName.clear(); }
};

class FindSpecializations : public Inspector {
    ReferenceMap*      refMap;
    const TypeMap*     typeMap;
    SpecializationMap* specMap;
 public:
    FindSpecializations(ReferenceMap* refMap, const TypeMap* typeMap, SpecializationMap* specMap) :
            refMap(refMap), typeMap(typeMap), specMap(specMap) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap); CHECK_NULL(specMap);
        setName("FindSpecializations"); }

    bool isSimpleConstant(const IR::Expression* expression) const;
    Visitor::profile_t init_apply(const IR::Node* node) override
    { specMap->clear(); return Inspector::init_apply(node); }
    void postorder(const IR::ConstructorCallExpression* expression) override;
    void postorder(const IR::Declaration_Instance* decl) override;
};

class Specialize : public Transform {
    ReferenceMap*      refMap;
    SpecializationMap* specMap;
 public:
    Specialize(ReferenceMap* refMap, SpecializationMap* specMap) : refMap(refMap), specMap(specMap)
    { CHECK_NULL(refMap); CHECK_NULL(specMap); setName("Specialize"); }
    const IR::Node* postorder(IR::P4Parser* parser) override;
    const IR::Node* postorder(IR::P4Control* control) override;
    const IR::Node* postorder(IR::ConstructorCallExpression* expression) override;
    const IR::Node* postorder(IR::Declaration_Instance* decl) override;
};

class SpecializeAll : public PassRepeated {
    SpecializationMap specMap;
 public:
    SpecializeAll(ReferenceMap* refMap, TypeMap* typeMap, bool isv1);
};

}  // namespace P4

#endif /* _MIDEND_SPECIALIZE_H_ */
