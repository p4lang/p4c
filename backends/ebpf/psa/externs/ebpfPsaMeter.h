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
#ifndef BACKENDS_EBPF_PSA_EXTERNS_EBPFPSAMETER_H_
#define BACKENDS_EBPF_PSA_EXTERNS_EBPFPSAMETER_H_

#include <stddef.h>

#include "backends/ebpf/codeGen.h"
#include "backends/ebpf/ebpfProgram.h"
#include "backends/ebpf/ebpfTable.h"
#include "backends/ebpf/ebpfType.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/methodInstance.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "lib/cstring.h"

namespace EBPF {

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
    const cstring indirectValueField = "value";
    const cstring spinlockField = "lock";

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

}  // namespace EBPF

#endif  // BACKENDS_EBPF_PSA_EXTERNS_EBPFPSAMETER_H_
