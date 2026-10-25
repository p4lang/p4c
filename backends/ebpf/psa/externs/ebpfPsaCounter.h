/*
 * SPDX-FileCopyrightText: 2022 Open Networking Foundation
 * SPDX-FileCopyrightText: 2022 Orange
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef BACKENDS_EBPF_PSA_EXTERNS_EBPFPSACOUNTER_H_
#define BACKENDS_EBPF_PSA_EXTERNS_EBPFPSACOUNTER_H_

#include "backends/ebpf/ebpfTable.h"

namespace P4::EBPF {

class ControlBodyTranslatorPSA;

class EBPFCounterPSA : public EBPFCounterTable {
 protected:
    EBPFType *dataplaneWidthType;
    EBPFType *indexWidthType;
    bool isDirect;

 public:
    enum CounterType { PACKETS, BYTES, PACKETS_AND_BYTES };
    CounterType type;

    EBPFCounterPSA(const EBPFProgram *program, const IR::Declaration_Instance *di, cstring name,
                   CodeGenInspector *codeGen);

    static CounterType toCounterType(int type);

    void emitTypes(CodeBuilder *builder) override;
    virtual void emitKeyType(CodeBuilder *builder);
    virtual void emitValueType(CodeBuilder *builder);
    void emitInstance(CodeBuilder *builder) override;

    void emitMethodInvocation(CodeBuilder *builder, const P4::ExternMethod *method,
                              CodeGenInspector *codeGen);
    void emitDirectMethodInvocation(CodeBuilder *builder, const P4::ExternMethod *method,
                                    cstring valuePtr);
    virtual void emitCount(CodeBuilder *builder, const IR::MethodCallExpression *expression,
                           CodeGenInspector *codeGen);
    virtual void emitCounterUpdate(CodeBuilder *builder, cstring target, cstring keyName);
    virtual void emitCounterInitializer(CodeBuilder *builder);

    DECLARE_TYPEINFO(EBPFCounterPSA, EBPFCounterTable);
};

}  // namespace P4::EBPF

#endif /* BACKENDS_EBPF_PSA_EXTERNS_EBPFPSACOUNTER_H_ */
