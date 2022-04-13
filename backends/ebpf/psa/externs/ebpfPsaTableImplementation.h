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

#ifndef BACKENDS_EBPF_PSA_EXTERNS_EBPFPSATABLEIMPLEMENTATION_H_
#define BACKENDS_EBPF_PSA_EXTERNS_EBPFPSATABLEIMPLEMENTATION_H_

#include "backends/ebpf/ebpfTable.h"
#include "backends/ebpf/psa/ebpfPsaTable.h"

namespace EBPF {

// Base class for ActionProfile and ActionSelector
class EBPFTableImplementationPSA : public EBPFTablePSA {
 public:
    EBPFTableImplementationPSA(const EBPFProgram* program, CodeGenInspector* codeGen,
                               const IR::Declaration_Instance* decl);

    void emitTypes(CodeBuilder* builder) override;
    void emitInitializer(CodeBuilder *builder) override;
    virtual void emitReferenceEntry(CodeBuilder *builder);

    virtual void registerTable(const EBPFTablePSA * instance);

    virtual void applyImplementation(CodeBuilder* builder, cstring tableValueName,
                                     cstring actionRunVariable) = 0;

 protected:
    const IR::Declaration_Instance* declaration;
    cstring referenceName;

    void verifyTableActionList(const EBPFTablePSA * instance);
    void verifyTableNoDefaultAction(const EBPFTablePSA * instance);
    void verifyTableNoDirectObjects(const EBPFTablePSA * instance);
    void verifyTableNoEntries(const EBPFTablePSA * instance);

    unsigned getUintFromExpression(const IR::Expression * expr, unsigned defaultValue);
};

class EBPFActionProfilePSA : public EBPFTableImplementationPSA {
 public:
    EBPFActionProfilePSA(const EBPFProgram* program, CodeGenInspector* codeGen,
                         const IR::Declaration_Instance* decl);

    void emitInstance(CodeBuilder *builder) override;
    void applyImplementation(CodeBuilder* builder, cstring tableValueName,
                             cstring actionRunVariable) override;
};

}  // namespace EBPF

#endif  // BACKENDS_EBPF_PSA_EXTERNS_EBPFPSATABLEIMPLEMENTATION_H_
