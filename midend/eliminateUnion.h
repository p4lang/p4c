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

#ifndef _MIDEND_ELIMINATEUNION_H_
#define _MIDEND_ELIMINATEUNION_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

/**
 * Replaces unions with a struct and an enum

union U {
   bit<32> b;
   bit<16> c;
}

U x;

x.b = ...;
switch (x) {
   U.b: ... ;
   ...
}

* generates the following program

enum U_Tag {
   None, b, c
}

struct U {
   U_Tag tag;
   bit<32> b;
   bit<16> c;
}

U x;
x.tag = U_Tag.None;

x.b = ...;
x.tag = U_Tag.b;
switch (x.tag) {
   UE.b: ... ;
}

 */
class DoEliminateUnion final : public Transform {
    ReferenceMap* refMap;
    const TypeMap* typeMap;
    std::map<cstring, IR::ID> tagTypeName;    // maps union name to tag enum name
    std::map<cstring, cstring> tagFieldName;  // maps union name to struct tag field
    std::map<cstring, cstring> noneTagName;   // maps union name to None tag
    IR::IndexedVector<IR::StatOrDecl> toInsert;
 public:
    explicit DoEliminateUnion(ReferenceMap* refMap, const TypeMap* typeMap):
            refMap(refMap), typeMap(typeMap)
    { setName("DoEliminateUnion"); CHECK_NULL(refMap); CHECK_NULL(typeMap); }
    const IR::Node* postorder(IR::Type_Union* type) override;
    const IR::Node* postorder(IR::Member* expression) override;
    const IR::Node* postorder(IR::SwitchStatement* statement) override;
    const IR::Node* postorder(IR::Declaration_Variable* decl) override;
    const IR::Node* postorder(IR::P4Control* control) override;
    const IR::Node* postorder(IR::P4Action* action) override;
    const IR::Node* postorder(IR::AssignmentStatement* statement) override;
};

class EliminateUnion final : public PassManager {
 public:
    EliminateUnion(ReferenceMap* refMap, TypeMap* typeMap,
                   TypeChecking* typeChecking = nullptr) {
        if (!typeChecking)
            typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoEliminateUnion(refMap, typeMap));
        passes.push_back(new ClearTypeMap(typeMap));
        setName("EliminateUnion");
    }
};

}  // namespace P4

#endif /* _MIDEND_ELIMINATEUNION_H_ */
