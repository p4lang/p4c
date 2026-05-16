/*
 * SPDX-FileCopyrightText: 2022 Open Networking Foundation
 * SPDX-FileCopyrightText: 2022 Orange
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef BACKENDS_EBPF_PSA_EXTERNS_EBPFPSAMETER_H_
#define BACKENDS_EBPF_PSA_EXTERNS_EBPFPSAMETER_H_

#include "backends/ebpf/ebpfTable.h"

namespace P4::EBPF {

class ControlBodyTranslatorPSA;

class EBPFMeterPSA : public EBPFTableBase {
 private:
    static IR::IndexedVector<IR::StructField> getValueFields();
    static IR::Type_Struct *createSpinlockStruct();
    static EBPFType *getBaseValueType(P4::ReferenceMap *refMap);
    EBPFType *getIndirectValueType() const;
    static cstring getBaseStructName(P4::ReferenceMap *refMap);
    cstring getIndirectStructName() const;

    void emitIndex(CodeBuilder *builder, const P4::ExternMethod *method,
                   ControlBodyTranslatorPSA *translator) const;

 protected:
    const cstring indirectValueField = "value"_cs;
    const cstring spinlockField = "lock"_cs;

    size_t size{};
    EBPFType *keyType{};
    bool isDirect;

 public:
    enum MeterType { PACKETS, BYTES };
    MeterType type;

    EBPFMeterPSA(const EBPFProgram *program, cstring instanceName,
                 const IR::Declaration_Instance *di, CodeGenInspector *codeGen);

    static MeterType toType(const int typeCode);

    void emitKeyType(CodeBuilder *builder) const;
    static void emitValueStruct(CodeBuilder *builder, P4::ReferenceMap *refMap);
    void emitValueType(CodeBuilder *builder) const;
    void emitSpinLockField(CodeBuilder *builder) const;
    void emitInstance(CodeBuilder *builder) const;
    void emitExecute(CodeBuilder *builder, const P4::ExternMethod *method,
                     ControlBodyTranslatorPSA *translator) const;
    void emitDirectExecute(CodeBuilder *builder, const P4::ExternMethod *method,
                           cstring valuePtr) const;

    static cstring meterExecuteFunc(bool trace, P4::ReferenceMap *refMap);
};

}  // namespace P4::EBPF

#endif  // BACKENDS_EBPF_PSA_EXTERNS_EBPFPSAMETER_H_
