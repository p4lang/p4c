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

#ifndef BACKENDS_TC_TCEXTERNS_H_
#define BACKENDS_TC_TCEXTERNS_H_

#include "backend.h"
#include "ebpfCodeGen.h"

namespace TC {

class ControlBodyTranslatorPNA;

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

        if (ts->arguments->size() != 2) {
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

}  // namespace TC

#endif /* BACKENDS_TC_TCEXTERNS_H_ */
