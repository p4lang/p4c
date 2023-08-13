/*
Copyright 2022-present Orange
Copyright 2022-present Open Networking Foundation

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

#include "ebpfPsaControl.h"

#include <utility>

#include "backends/ebpf/ebpfControl.h"
#include "backends/ebpf/ebpfObject.h"
#include "backends/ebpf/psa/externs/ebpfPsaChecksum.h"
#include "backends/ebpf/psa/externs/ebpfPsaCounter.h"
#include "backends/ebpf/psa/externs/ebpfPsaRandom.h"
#include "backends/ebpf/psa/externs/ebpfPsaRegister.h"
#include "backends/ebpf/target.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "ir/id.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/stringify.h"

namespace EBPF {

ControlBodyTranslatorPSA::ControlBodyTranslatorPSA(const EBPFControlPSA *control)
    : CodeGenInspector(control->program->refMap, control->program->typeMap),
      ControlBodyTranslator(control) {}

bool ControlBodyTranslatorPSA::preorder(const IR::AssignmentStatement *a) {
    if (auto methodCallExpr = a->right->to<IR::MethodCallExpression>()) {
        auto mi = P4::MethodInstance::resolve(methodCallExpr, control->program->refMap,
                                              control->program->typeMap);
        auto ext = mi->to<P4::ExternMethod>();
        if (ext == nullptr) {
            return false;
        }

        if (ext->originalExternType->name.name == "Register" && ext->method->type->name == "read") {
            cstring name = EBPFObject::externalName(ext->object);
            auto reg = control->to<EBPFControlPSA>()->getRegister(name);
            reg->emitRegisterRead(builder, ext, this, a->left);
            return false;
        } else if (ext->originalExternType->name.name == "Hash") {
            cstring name = EBPFObject::externalName(ext->object);
            auto hash = control->to<EBPFControlPSA>()->getHash(name);
            // Before assigning a value to a left expression we have to calculate a hash.
            // Then the hash value is stored in a registerVar variable.
            hash->calculateHash(builder, ext->expr, this);
            builder->emitIndent();
        } else if (ext->originalExternType->name.name == "Meter" ||
                   ext->originalExternType->name.name == "DirectMeter") {
            // It is just for trace message before meter execution
            cstring name = EBPFObject::externalName(ext->object);
            auto msgStr = Util::printf_format("Executing meter: %s", name);
            builder->target->emitTraceMessage(builder, msgStr.c_str());
        }
    }

    return CodeGenInspector::preorder(a);
}

void ControlBodyTranslatorPSA::processMethod(const P4::ExternMethod *method) {
    auto decl = method->object;
    auto declType = method->originalExternType;
    cstring name = EBPFObject::externalName(decl);

    if (declType->name.name == "Counter") {
        auto counterMap = control->getCounter(name);
        counterMap->to<EBPFCounterPSA>()->emitMethodInvocation(builder, method, this);
        return;
    } else if (declType->name.name == "Meter") {
        auto meter = control->to<EBPFControlPSA>()->getMeter(name);
        meter->emitExecute(builder, method, this);
        return;
    } else if (declType->name.name == "Hash") {
        auto hash = control->to<EBPFControlPSA>()->getHash(name);
        hash->processMethod(builder, method->method->name.name, method->expr, this);
        return;
    } else if (declType->name.name == "Random") {
        auto rand = control->to<EBPFControlPSA>()->getRandomExt(name);
        rand->processMethod(builder, method);
        return;
    } else if (declType->name.name == "Register") {
        auto reg = control->to<EBPFControlPSA>()->getRegister(name);
        if (method->method->type->name == "write") {
            reg->emitRegisterWrite(builder, method, this);
        } else if (method->method->type->name == "read") {
            ::warning(ErrorType::WARN_UNUSED, "This Register(%1%) read value is not used!", name);
            reg->emitRegisterRead(builder, method, this, nullptr);
        }
        return;
    } else {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "%1%: Unexpected method call", method->expr);
    }
}

cstring ControlBodyTranslatorPSA::getParamName(const IR::PathExpression *expr) {
    return expr->path->name.name;
}

void EBPFControlPSA::emit(CodeBuilder *builder) {
    for (auto h : hashes) h.second->emitVariables(builder);
    EBPFControl::emit(builder);
}

void EBPFControlPSA::emitTableTypes(CodeBuilder *builder) {
    EBPFControl::emitTableTypes(builder);

    for (auto it : registers) it.second->emitTypes(builder);
    for (auto it : meters) it.second->emitKeyType(builder);

    //  Value type for any indirect meter is the same
    if (!meters.empty()) {
        meters.begin()->second->emitValueType(builder);
    }
}

void EBPFControlPSA::emitTableInstances(CodeBuilder *builder) {
    for (auto it : tables) it.second->emitInstance(builder);
    for (auto it : counters) it.second->emitInstance(builder);
    for (auto it : registers) it.second->emitInstance(builder);
    for (auto it : meters) it.second->emitInstance(builder);
}

void EBPFControlPSA::emitTableInitializers(CodeBuilder *builder) {
    for (auto it : tables) {
        it.second->emitInitializer(builder);
    }
    for (auto it : registers) {
        it.second->emitInitializer(builder);
    }
}

}  // namespace EBPF
