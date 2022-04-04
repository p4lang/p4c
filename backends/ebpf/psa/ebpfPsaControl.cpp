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
    } else if (declType->name.name == "Meter") {
        auto meter = control->to<EBPFControlPSA>()->getMeter(name);
        meter->emitExecute(builder, method, this);
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
            if (ext->originalExternType->name.name == "Meter") {
                cstring name = EBPFObject::externalName(ext->object);
                auto msgStr = Util::printf_format("Executing meter: %s", name);
                builder->target->emitTraceMessage(builder, msgStr.c_str());
            }
        }
    }

    return CodeGenInspector::preorder(a);
}

bool EBPFControlPSA::build() {
    auto params = p4Control->type->applyParams;
    if (params->size() != 4) {
        ::error(ErrorType::ERR_EXPECTED,
                "Expected control block to have exactly 4 parameters");
        return false;
    }

    auto it = params->parameters.begin();
    headers = *it;

    codeGen = new ControlBodyTranslatorPSA(this);
    codeGen->substitute(headers, parserHeaders);

    return ::errorCount() == 0;
}

void EBPFControlPSA::emit(CodeBuilder *builder) {
    hitVariable = program->refMap->newName("hit");
    auto hitType = EBPFTypeFactory::instance->create(IR::Type_Boolean::get());
    builder->emitIndent();
    hitType->declare(builder, hitVariable, false);
    builder->endOfStatement(true);
    for (auto a : p4Control->controlLocals)
        emitDeclaration(builder, a);
    builder->emitIndent();
    codeGen->setBuilder(builder);
    p4Control->body->apply(*codeGen);
    builder->newline();
}

void EBPFControlPSA::emitTableTypes(CodeBuilder *builder) {
    EBPFControl::emitTableTypes(builder);

    for (auto it : meters)
        it.second->emitKeyType(builder);

    //  Value type for any indirect meter is the same
    if (!meters.empty()) {
        meters.begin()->second->emitValueType(builder);
    }
}

void EBPFControlPSA::emitTableInstances(CodeBuilder* builder) {
    for (auto it : tables)
        it.second->emitInstance(builder);
    for (auto it : counters)
        it.second->emitInstance(builder);
    for (auto it : meters)
        it.second->emitInstance(builder);
}

void EBPFControlPSA::emitTableInitializers(CodeBuilder* builder) {
    for (auto it : tables) {
        it.second->emitInitializer(builder);
    }
}

}