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

#include <algorithm>
#include <utility>
#include <vector>

#include "backends/ebpf/codeGen.h"
#include "backends/ebpf/ebpfProgram.h"
#include "backends/ebpf/ebpfTable.h"
#include "backends/ebpf/psa/externs/ebpfPsaCounter.h"
#include "backends/ebpf/psa/externs/ebpfPsaMeter.h"
#include "ir/id.h"
#include "ir/ir.h"
#include "lib/cstring.h"

namespace EBPF {

class EBPFTableImplementationPSA;

class EBPFTablePSA : public EBPFTable {
 private:
    struct ConstTernaryEntryDesc {
        const IR::Entry *entry;
        unsigned priority;
    };
    typedef std::vector<ConstTernaryEntryDesc> EntriesGroup_t;
    typedef std::vector<EntriesGroup_t> EntriesGroupedByMask_t;
    EntriesGroupedByMask_t getConstEntriesGroupedByMask();
    bool hasConstEntries();
    const cstring addPrefixFunctionName = "add_prefix_and_entries";
    const cstring tuplesMapName = instanceName + "_tuples_map";
    const cstring prefixesMapName = instanceName + "_prefixes";

 protected:
    ActionTranslationVisitor *createActionTranslationVisitor(
        cstring valueName, const EBPFProgram *program) const override;

    void initDirectCounters();
    void initDirectMeters();
    void initImplementation();

    bool tableCacheEnabled = false;
    cstring cacheValueTypeName;
    cstring cacheTableName;
    cstring cacheKeyTypeName;
    void tryEnableTableCache();
    void createCacheTypeNames(bool isCacheKeyType, bool isCacheValueType);

    void emitTableValue(CodeBuilder *builder, const IR::Expression *expr, cstring valueName);
    void emitDefaultActionInitializer(CodeBuilder *builder);
    void emitConstEntriesInitializer(CodeBuilder *builder);
    void emitTernaryConstEntriesInitializer(CodeBuilder *builder);
    void emitMapUpdateTraceMsg(CodeBuilder *builder, cstring mapName, cstring returnCode) const;
    void emitValueMask(CodeBuilder *builder, cstring valueMask, cstring nextMask,
                       int tupleId) const;
    void emitKeyMasks(CodeBuilder *builder, EntriesGroupedByMask_t &entriesGroupedByMask,
                      std::vector<cstring> &keyMasksNames);
    void emitKeysAndValues(CodeBuilder *builder, EntriesGroup_t &sameMaskEntries,
                           std::vector<cstring> &keyNames, std::vector<cstring> &valueNames);

    const IR::PathExpression *getActionNameExpression(const IR::Expression *expr) const;

 public:
    // We use vectors to keep an order of Direct Meters or Counters from a P4 program.
    // This order is important from CLI tool point of view.
    std::vector<std::pair<cstring, EBPFCounterPSA *>> counters;
    std::vector<std::pair<cstring, EBPFMeterPSA *>> meters;
    EBPFTableImplementationPSA *implementation;

    EBPFTablePSA(const EBPFProgram *program, const IR::TableBlock *table,
                 CodeGenInspector *codeGen);
    EBPFTablePSA(const EBPFProgram *program, CodeGenInspector *codeGen, cstring name);

    void emitInstance(CodeBuilder *builder) override;
    void emitTypes(CodeBuilder *builder) override;
    void emitValueStructStructure(CodeBuilder *builder) override;
    void emitAction(CodeBuilder *builder, cstring valueName, cstring actionRunVariable) override;
    void emitInitializer(CodeBuilder *builder) override;
    void emitDirectValueTypes(CodeBuilder *builder) override;
    void emitLookupDefault(CodeBuilder *builder, cstring key, cstring value,
                           cstring actionRunVariable) override;
    bool dropOnNoMatchingEntryFound() const override;
    static cstring addPrefixFunc(bool trace);

    virtual void emitCacheTypes(CodeBuilder *builder);
    void emitCacheInstance(CodeBuilder *builder);
    void emitCacheLookup(CodeBuilder *builder, cstring key, cstring value) override;
    void emitCacheUpdate(CodeBuilder *builder, cstring key, cstring value) override;
    bool cacheEnabled() override { return tableCacheEnabled; }

    EBPFCounterPSA *getDirectCounter(cstring name) const {
        auto result = std::find_if(counters.begin(), counters.end(),
                                   [name](std::pair<cstring, EBPFCounterPSA *> elem) -> bool {
                                       return name == elem.first;
                                   });
        if (result != counters.end()) return result->second;
        return nullptr;
    }

    EBPFMeterPSA *getMeter(cstring name) const {
        auto result = std::find_if(
            meters.begin(), meters.end(),
            [name](std::pair<cstring, EBPFMeterPSA *> elem) -> bool { return name == elem.first; });
        if (result != meters.end()) return result->second;
        return nullptr;
    }

    bool isMatchTypeSupported(const IR::Declaration_ID *matchType) override {
        return EBPFTable::isMatchTypeSupported(matchType) || matchType->name.name == "selector";
    }
};

}  // namespace EBPF

#endif /* BACKENDS_EBPF_PSA_EBPFPSATABLE_H_ */
