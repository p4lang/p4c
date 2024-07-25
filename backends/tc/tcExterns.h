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

namespace P4::TC {

using namespace ::P4::literals;

class ControlBodyTranslatorPNA;
class ConvertToBackendIR;

class EBPFCounterPNA : public EBPF::EBPFCounterPSA {
    const IR::Declaration_Instance *di;
    cstring tblname;

 public:
    EBPFCounterPNA(const EBPF::EBPFProgram *program, const IR::Declaration_Instance *di,
                   cstring name, EBPF::CodeGenInspector *codeGen, cstring tblname)
        : EBPF::EBPFCounterPSA(program, di, name, codeGen) {
        this->tblname = tblname;
        this->di = di;
    }
    EBPFCounterPNA(const EBPF::EBPFProgram *program, const IR::Declaration_Instance *di,
                   cstring name, EBPF::CodeGenInspector *codeGen)
        : EBPF::EBPFCounterPSA(program, di, name, codeGen) {
        this->di = di;
    }

    void emitDirectMethodInvocation(EBPF::CodeBuilder *builder, const P4::ExternMethod *method,
                                    const ConvertToBackendIR *tcIR);
    void emitMethodInvocation(EBPF::CodeBuilder *builder, const P4::ExternMethod *method,
                              ControlBodyTranslatorPNA *translator);
    virtual void emitCounterUpdate(EBPF::CodeBuilder *builder, const ConvertToBackendIR *tcIR);
    virtual void emitCount(EBPF::CodeBuilder *builder, const P4::ExternMethod *method,
                           ControlBodyTranslatorPNA *translator);
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
            ::P4::error(ErrorType::ERR_MODEL, "Missing specialization: %1%", di);
            return;
        }
        auto ts = di->type->to<IR::Type_Specialized>();

        if (ts->arguments->size() != PARAM_INDEX_2) {
            ::P4::error(ErrorType::ERR_MODEL, "Expected a type specialized with two arguments: %1%",
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
            ::P4::error(ErrorType::ERR_UNEXPECTED,
                        "%1%: not a DirectCounter, see declaration of %2%", pe, decl);
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

class InternetChecksumAlgorithmPNA : public EBPF::EBPFHashAlgorithmPSA {
 protected:
    cstring stateVar;
    cstring csumVar;

    void updateChecksum(EBPF::CodeBuilder *builder, const ArgumentsList &arguments, bool addData);

 public:
    InternetChecksumAlgorithmPNA(const EBPF::EBPFProgram *program, cstring name)
        : EBPF::EBPFHashAlgorithmPSA(program, name) {}

    void emitVariables(EBPF::CodeBuilder *builder, const IR::Declaration_Instance *decl) override;

    void emitClear(EBPF::CodeBuilder *builder) override;
    void emitAddData(EBPF::CodeBuilder *builder, const ArgumentsList &arguments) override;
    void emitGet(EBPF::CodeBuilder *builder) override;

    void emitSubtractData(EBPF::CodeBuilder *builder, const ArgumentsList &arguments) override;

    void emitGetInternalState(EBPF::CodeBuilder *builder) override;
    void emitSetInternalState(EBPF::CodeBuilder *builder,
                              const IR::MethodCallExpression *expr) override;
    cstring getConvertByteOrderFunction(unsigned widthToEmit, cstring byte_order);
};

class EBPFChecksumPNA : public EBPF::EBPFChecksumPSA {
 protected:
    void init(const EBPF::EBPFProgram *program, cstring name, int type);

 public:
    EBPFChecksumPNA(const EBPF::EBPFProgram *program, const IR::Declaration_Instance *block,
                    cstring name)
        : EBPF::EBPFChecksumPSA(program, block, name) {
        auto di = block->to<IR::Declaration_Instance>();
        if (di->arguments->size() != 1) {
            ::P4::error(ErrorType::ERR_UNEXPECTED, "Expected exactly 1 argument %1%", block);
            return;
        }
        int type = di->arguments->at(0)->expression->checkedTo<IR::Constant>()->asInt();
        init(program, name, type);
    }

    EBPFChecksumPNA(const EBPF::EBPFProgram *program, const IR::Declaration_Instance *block,
                    cstring name, int type)
        : EBPF::EBPFChecksumPSA(program, block, name, type) {
        init(program, name, type);
    }
};

class EBPFInternetChecksumPNA : public EBPFChecksumPNA {
 public:
    EBPFInternetChecksumPNA(const EBPF::EBPFProgram *program, const IR::Declaration_Instance *block,
                            cstring name)
        : EBPFChecksumPNA(program, block, name,
                          EBPF::EBPFHashAlgorithmPSA::HashAlgorithm::ONES_COMPLEMENT16) {}

    void processMethod(EBPF::CodeBuilder *builder, cstring method,
                       const IR::MethodCallExpression *expr, Visitor *visitor) override;
};

}  // namespace P4::TC

#endif /* BACKENDS_TC_TCEXTERNS_H_ */
