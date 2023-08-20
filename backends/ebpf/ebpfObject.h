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

#ifndef BACKENDS_EBPF_EBPFOBJECT_H_
#define BACKENDS_EBPF_EBPFOBJECT_H_

#include "codeGen.h"
#include "ebpfModel.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "lib/castable.h"
#include "target.h"

namespace EBPF {

// Base class for EBPF objects
class EBPFObject : public ICastable {
 public:
    virtual ~EBPFObject() {}

    static cstring externalName(const IR::IDeclaration *declaration) {
        cstring name = declaration->externalName();
        return name.replace('.', '_');
    }

    static cstring getSpecializedTypeName(const IR::Declaration_Instance *di) {
        if (auto typeSpec = di->type->to<IR::Type_Specialized>()) {
            if (auto typeName = typeSpec->baseType->to<IR::Type_Name>()) {
                return typeName->path->name.name;
            }
        }

        return nullptr;
    }

    static cstring getTypeName(const IR::Declaration_Instance *di) {
        if (auto typeName = di->type->to<IR::Type_Name>()) {
            return typeName->path->name.name;
        }

        return nullptr;
    }
};

}  // namespace EBPF

#endif /* BACKENDS_EBPF_EBPFOBJECT_H_ */
