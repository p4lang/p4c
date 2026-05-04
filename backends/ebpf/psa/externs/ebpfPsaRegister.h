/*
 * SPDX-FileCopyrightText: 2022 Open Networking Foundation
 * SPDX-FileCopyrightText: 2022 Orange
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef BACKENDS_EBPF_PSA_EXTERNS_EBPFPSAREGISTER_H_
#define BACKENDS_EBPF_PSA_EXTERNS_EBPFPSAREGISTER_H_

#include "backends/ebpf/ebpfTable.h"
#include "backends/ebpf/ebpfType.h"

namespace P4::EBPF {

class ControlBodyTranslatorPSA;

class EBPFRegisterPSA : public EBPFTableBase {
 protected:
    size_t size;
    /// Initial value for Register cells.
    /// It can be nullptr if an initial value is not provided or not IR::Constant.
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

}  // namespace P4::EBPF

#endif  // BACKENDS_EBPF_PSA_EXTERNS_EBPFPSAREGISTER_H_
