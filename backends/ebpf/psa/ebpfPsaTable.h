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
#ifndef BACKENDS_EBPF_PSA_EBPFPSATABLE_H_
#define BACKENDS_EBPF_PSA_EBPFPSATABLE_H_

#include "frontends/p4/methodInstance.h"
#include "backends/ebpf/ebpfTable.h"
#include "backends/ebpf/psa/externs/ebpfPsaCounter.h"
#include "backends/ebpf/psa/externs/ebpfPsaMeter.h"

namespace EBPF {

class EBPFTableImplementationPSA;

class EBPFTablePSA : public EBPFTable {
 private:
    void emitTableDecl(CodeBuilder *builder,
                       cstring tblName,
                       TableKind kind,
                       cstring keyTypeName,
                       cstring valueTypeName,
                       size_t size) const;

 protected:
    ActionTranslationVisitor* createActionTranslationVisitor(
            cstring valueName, const EBPFProgram* program) const override;

    void initDirectCounters();
    void initImplementation();

    void emitTableValue(CodeBuilder* builder, const IR::MethodCallExpression* actionMce,
                        cstring valueName);
    void emitDefaultActionInitializer(CodeBuilder *builder);
    void emitConstEntriesInitializer(CodeBuilder *builder);
    void emitMapUpdateTraceMsg(CodeBuilder *builder, cstring mapName,
                               cstring returnCode) const;

    const IR::PathExpression* getActionNameExpression(const IR::Expression* expr) const;

 public:
    std::vector<std::pair<cstring, EBPFCounterPSA *>> counters;
    // TODO: DirectMeter is not implemented now, but
    //  this is needed in table implementation to validate table properties
    std::vector<cstring> meters;
    EBPFTableImplementationPSA* implementation;

    EBPFTablePSA(const EBPFProgram* program, const IR::TableBlock* table,
                 CodeGenInspector* codeGen);
    EBPFTablePSA(const EBPFProgram* program, CodeGenInspector* codeGen, cstring name);

    void emitInstance(CodeBuilder* builder) override;
    void emitTypes(CodeBuilder* builder) override;
    void emitValueStructStructure(CodeBuilder* builder) override;
    void emitAction(CodeBuilder* builder, cstring valueName, cstring actionRunVariable) override;
    void emitInitializer(CodeBuilder* builder) override;
    void emitDirectValueTypes(CodeBuilder* builder) override;
    void emitLookup(CodeBuilder* builder, cstring key, cstring value) override;
    void emitLookupDefault(CodeBuilder* builder, cstring key, cstring value,
                           cstring actionRunVariable) override;
    bool dropOnNoMatchingEntryFound() const override;

    EBPFCounterPSA* getDirectCounter(cstring name) const {
        auto result = std::find_if(counters.begin(), counters.end(),
            [name](std::pair<cstring, EBPFCounterPSA *> elem)->bool {
                return name == elem.first;
            });
        if (result != counters.end())
            return result->second;
        return nullptr;
    }

    bool isMatchTypeSupported(const IR::Declaration_ID* matchType) override {
        return EBPFTable::isMatchTypeSupported(matchType) ||
               matchType->name.name == "selector";
    }
};

}  // namespace EBPF

#endif /* BACKENDS_EBPF_PSA_EBPFPSATABLE_H_ */
