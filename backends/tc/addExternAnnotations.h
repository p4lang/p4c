/*
Copyright (C) 2023 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions
and limitations under the License.
*/

#ifndef BACKENDS_TC_ADDEXTERNANNOTATIONS_H_
#define BACKENDS_TC_ADDEXTERNANNOTATIONS_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace TC {

class AddExternAnnotations : public Transform {
 public:
    struct Extern_field {
        cstring field_name;
        cstring annotation;
        unsigned size;
        Extern_field(cstring name, cstring type, unsigned sz) {
            field_name = name;
            annotation = type;
            size = sz;
        }
    };
    struct PNA_extern_struct {
        cstring struct_name;
        safe_vector<struct Extern_field *> fields;
        PNA_extern_struct(cstring name, safe_vector<struct Extern_field *> f) {
            struct_name = name;
            fields = f;
        }
    };
    safe_vector<PNA_extern_struct *> new_pna_extern_structs;
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;

 public:
    AddExternAnnotations(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : refMap(refMap), typeMap(typeMap) {}

    const IR::Node *postorder(IR::P4Program *program) override;
    const IR::Node *postorder(IR::Type_Extern *te) override;
    const IR::Type_Method *GetAnnotatedType(const IR::Type_Method *type);
    void GetExternStructures();
};

}  // namespace TC

#endif /* BACKENDS_TC_ADDEXTERNANNOTATIONS_H_ */
