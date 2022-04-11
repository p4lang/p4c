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

namespace EBPF {

ControlBodyTranslatorPSA::ControlBodyTranslatorPSA(const EBPFControlPSA* control) :
        CodeGenInspector(control->program->refMap, control->program->typeMap),
        ControlBodyTranslator(control) {}

void ControlBodyTranslatorPSA::processMethod(const P4::ExternMethod* method) {
    auto decl = method->object;
    auto declType = method->originalExternType;
    cstring name = EBPFObject::externalName(decl);

    if (declType->name.name == "Counter") {
        auto counterMap = control->getCounter(name);
        counterMap->to<EBPFCounterPSA>()->emitMethodInvocation(builder, method, this);
        return;
    } else if (declType->name.name == "Hash") {
        auto hash = control->to<EBPFControlPSA>()->getHash(name);
        hash->processMethod(builder, method->method->name.name, method->expr, this);
        return;
    }

    ControlBodyTranslator::processMethod(method);
}

bool ControlBodyTranslatorPSA::preorder(const IR::AssignmentStatement* a) {
    if (auto methodCallExpr = a->right->to<IR::MethodCallExpression>()) {
        auto mi = P4::MethodInstance::resolve(methodCallExpr,
                                              control->program->refMap,
                                              control->program->typeMap);
        auto ext = mi->to<P4::ExternMethod>();
        if (ext != nullptr) {
            if (ext->originalExternType->name.name == "Hash") {
                cstring name = EBPFObject::externalName(ext->object);
                auto hash = control->to<EBPFControlPSA>()->getHash(name);
                hash->processMethod(builder, "update", ext->expr, this);
                builder->emitIndent();
            }
        }
    }

    return CodeGenInspector::preorder(a);
}

void EBPFControlPSA::emit(CodeBuilder *builder) {
        for (auto h : hashes)
            h.second->emitVariables(builder);
        EBPFControl::emit(builder);
}

}
