/*
 * SPDX-FileCopyrightText: 2022 Orange
 * Copyright 2022-present Orange
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_EBPF_PSA_EXTERNS_EBPFPSADIGEST_H_
#define BACKENDS_EBPF_PSA_EXTERNS_EBPFPSADIGEST_H_

#include "backends/ebpf/ebpfObject.h"
#include "backends/ebpf/ebpfProgram.h"

namespace P4::EBPF {

class DeparserBodyTranslatorPSA;

class EBPFDigestPSA : public EBPFObject {
 private:
    cstring instanceName;
    const EBPFProgram *program;
    cstring valueTypeName;
    const IR::Declaration_Instance *declaration;
    /// arbitrary value for max queue size
    /// TODO: make it configurable
    int maxDigestQueueSize = 128;

 public:
    EBPFType *valueType;
    EBPFDigestPSA(const EBPFProgram *program, const IR::Declaration_Instance *di);

    void emitTypes(CodeBuilder *builder);
    void emitInstance(CodeBuilder *builder) const;
    void processMethod(CodeBuilder *builder, cstring method, const IR::MethodCallExpression *expr,
                       DeparserBodyTranslatorPSA *visitor);

    virtual void emitPushElement(CodeBuilder *builder, const IR::Expression *elem,
                                 Inspector *codegen) const;
    virtual void emitPushElement(CodeBuilder *builder, cstring elem) const;
};

}  // namespace P4::EBPF

#endif /* BACKENDS_EBPF_PSA_EXTERNS_EBPFPSADIGEST_H_ */
