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
#ifndef BACKENDS_EBPF_PSA_EXTERNS_EBPFPSACOUNTER_H_
#define BACKENDS_EBPF_PSA_EXTERNS_EBPFPSACOUNTER_H_

#include "backends/ebpf/codeGen.h"
#include "backends/ebpf/ebpfProgram.h"
#include "backends/ebpf/ebpfTable.h"
#include "backends/ebpf/ebpfType.h"
#include "frontends/p4/methodInstance.h"
#include "ir/ir.h"
#include "lib/cstring.h"

namespace EBPF {

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
};

}  // namespace EBPF

#endif /* BACKENDS_EBPF_PSA_EXTERNS_EBPFPSACOUNTER_H_ */
