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
#ifndef BACKENDS_EBPF_PSA_EXTERNS_EBPFPSAREGISTER_H_
#define BACKENDS_EBPF_PSA_EXTERNS_EBPFPSAREGISTER_H_

#include <stddef.h>

#include "backends/ebpf/codeGen.h"
#include "backends/ebpf/ebpfProgram.h"
#include "backends/ebpf/ebpfTable.h"
#include "backends/ebpf/ebpfType.h"
#include "frontends/p4/methodInstance.h"
#include "ir/ir.h"
#include "lib/cstring.h"

namespace EBPF {

class ControlBodyTranslatorPSA;

class EBPFRegisterPSA : public EBPFTableBase {
 protected:
    size_t size;
    // initial value for Register cells.
    // It can be nullptr if an initial value is not provided or not IR::Constant.
    const IR::Constant *initialValue = nullptr;
    const IR::Type *keyArg;
    const IR::Type *valueArg;
    EBPFType *keyType;
    EBPFType *valueType;

    bool shouldUseArrayMap();

 public:
    EBPFRegisterPSA(const EBPFProgram *program, cstring instanceName,
                    const IR::Declaration_Instance *di, CodeGenInspector *codeGen);

    void emitTypes(CodeBuilder *builder);
    void emitKeyType(CodeBuilder *builder);
    void emitValueType(CodeBuilder *builder);

    void emitInitializer(CodeBuilder *builder);
    void emitInstance(CodeBuilder *builder);
    void emitRegisterRead(CodeBuilder *builder, const P4::ExternMethod *method,
                          ControlBodyTranslatorPSA *translator,
                          const IR::Expression *leftExpression);
    void emitRegisterWrite(CodeBuilder *builder, const P4::ExternMethod *method,
                           ControlBodyTranslatorPSA *translator);
};

}  // namespace EBPF

#endif  // BACKENDS_EBPF_PSA_EXTERNS_EBPFPSAREGISTER_H_
