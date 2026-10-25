/*
 * Copyright 2019 Orange
 * SPDX-FileCopyrightText: 2019 Orange
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_UBPF_UBPFREGISTER_H_
#define BACKENDS_UBPF_UBPFREGISTER_H_

#include "ubpfTable.h"
#include "ubpfType.h"

namespace P4::UBPF {

class UBPFRegister final : public UBPFTableBase {
 public:
    UBPFRegister(const UBPFProgram *program, const IR::ExternBlock *block, cstring name,
                 EBPF::CodeGenInspector *codeGen);

    void emitInstance(EBPF::CodeBuilder *builder);
    void emitRegisterRead(EBPF::CodeBuilder *builder, const IR::MethodCallExpression *expression);
    void emitRegisterWrite(EBPF::CodeBuilder *builder, const IR::MethodCallExpression *expression);
    void emitMethodInvocation(EBPF::CodeBuilder *builder, const P4::ExternMethod *method);
    void emitKeyInstance(EBPF::CodeBuilder *builder, const IR::MethodCallExpression *expression);
    cstring emitValueInstanceIfNeeded(EBPF::CodeBuilder *builder, const IR::Argument *arg_value);
};

}  // namespace P4::UBPF

#endif /* BACKENDS_UBPF_UBPFREGISTER_H_ */
