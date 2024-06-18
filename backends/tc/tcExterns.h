/*
Copyright (C) 2024 Intel Corporation
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

#ifndef BACKENDS_TC_TCEXTERNS_H_
#define BACKENDS_TC_TCEXTERNS_H_

#include "backend.h"
#include "ebpfCodeGen.h"

namespace TC {

using namespace P4::literals;

class ControlBodyTranslatorPNA;
class ConvertToBackendIR;

class EBPFCounterPNA : public EBPF::EBPFCounterPSA {
    cstring tblname;

 public:
    EBPFCounterPNA(const EBPF::EBPFProgram *program, const IR::Declaration_Instance *di,
                   cstring name, EBPF::CodeGenInspector *codeGen, cstring tblname)
        : EBPF::EBPFCounterPSA(program, di, name, codeGen) {
        this->tblname = tblname;
    }
    void emitDirectMethodInvocation(EBPF::CodeBuilder *builder, const P4::ExternMethod *method,
                                    const ConvertToBackendIR *tcIR);
    virtual void emitCounterUpdate(EBPF::CodeBuilder *builder, const ConvertToBackendIR *tcIR);
};

class EBPFRegisterPNA : public EBPF::EBPFTableBase {
 protected:
    cstring instanceName;
    const IR::Type *keyArg;
    const IR::Type *valueArg;
    EBPF::EBPFType *keyType;
    EBPF::EBPFType *valueType;

 public:
    EBPFRegisterPNA(const EBPF::EBPFProgram *program, cstring instanceName,
                    const IR::Declaration_Instance *di, EBPF::CodeGenInspector *codeGen)
        : EBPF::EBPFTableBase(program, instanceName, codeGen) {
        CHECK_NULL(di);
        this->instanceName = di->toString();
        if (!di->type->is<IR::Type_Specialized>()) {
            ::error(ErrorType::ERR_MODEL, "Missing specialization: %1%", di);
            return;
        }
        auto ts = di->type->to<IR::Type_Specialized>();

        if (ts->arguments->size() != PARAM_INDEX_2) {
            ::error(ErrorType::ERR_MODEL, "Expected a type specialized with two arguments: %1%",
                    ts);
            return;
        }

        this->valueArg = ts->arguments->at(0);
        this->keyArg = ts->arguments->at(1);

        this->keyType = EBPF::EBPFTypeFactory::instance->create(keyArg);
        this->valueType = EBPF::EBPFTypeFactory::instance->create(valueArg);
    }
    void emitRegisterRead(EBPF::CodeBuilder *builder, const P4::ExternMethod *method,
                          ControlBodyTranslatorPNA *translator,
                          const IR::Expression *leftExpression);
    void emitRegisterWrite(EBPF::CodeBuilder *builder, const P4::ExternMethod *method,
                           ControlBodyTranslatorPNA *translator);
    void emitInitializer(EBPF::CodeBuilder *builder, const P4::ExternMethod *method,
                         ControlBodyTranslatorPNA *translator);
};

class EBPFTablePNADirectCounterPropertyVisitor : public EBPF::EBPFTablePsaPropertyVisitor {
 public:
    explicit EBPFTablePNADirectCounterPropertyVisitor(EBPF::EBPFTablePSA *table)
        : EBPF::EBPFTablePsaPropertyVisitor(table) {}

    bool preorder(const IR::PathExpression *pe) override {
        auto decl = table->program->refMap->getDeclaration(pe->path, true);
        auto di = decl->to<IR::Declaration_Instance>();
        CHECK_NULL(di);
        if (EBPF::EBPFObject::getSpecializedTypeName(di) != "DirectCounter") {
            ::error(ErrorType::ERR_UNEXPECTED, "%1%: not a DirectCounter, see declaration of %2%",
                    pe, decl);
            return false;
        }
        auto counterName = EBPF::EBPFObject::externalName(di);
        auto tblname = table->table->container->name.originalName;
        auto ctr = new EBPFCounterPNA(table->program, di, counterName, table->codeGen, tblname);
        table->counters.emplace_back(std::make_pair(counterName, ctr));
        return false;
    }

    void visitTableProperty() {
        EBPF::EBPFTablePsaPropertyVisitor::visitTableProperty("pna_direct_counter"_cs);
    }
};

}  // namespace TC

#endif /* BACKENDS_TC_TCEXTERNS_H_ */
