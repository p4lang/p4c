/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
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

namespace P4::EBPF {

/// Base class for EBPF objects.
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

    DECLARE_TYPEINFO(EBPFObject);
};

}  // namespace P4::EBPF

#endif /* BACKENDS_EBPF_EBPFOBJECT_H_ */
