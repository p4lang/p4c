/*
 * SPDX-FileCopyrightText: 2022 Open Networking Foundation
 * SPDX-FileCopyrightText: 2022 Orange
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef BACKENDS_EBPF_PSA_EBPFPSATABLE_H_
#define BACKENDS_EBPF_PSA_EBPFPSATABLE_H_

#include "backends/ebpf/ebpfTable.h"
#include "backends/ebpf/psa/externs/ebpfPsaCounter.h"
#include "backends/ebpf/psa/externs/ebpfPsaMeter.h"
#include "frontends/p4/methodInstance.h"

namespace P4::EBPF {

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
    const cstring addPrefixFunctionName = "add_prefix_and_entries"_cs;
    const cstring tuplesMapName = instanceName + "_tuples_map"_cs;
    const cstring prefixesMapName = instanceName + "_prefixes"_cs;

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

 public:
    /// We use vectors to keep an order of Direct Meters or Counters from a P4 program.
    /// This order is important from CLI tool point of view.
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
    const IR::PathExpression *getActionNameExpression(const IR::Expression *expr) const;
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

    DECLARE_TYPEINFO(EBPFTablePSA, EBPFTable);
};

class EBPFTablePsaPropertyVisitor : public Inspector {
 protected:
    EBPFTablePSA *table;

 public:
    explicit EBPFTablePsaPropertyVisitor(EBPFTablePSA *table) : table(table) {}

    /// Use these two preorders to print error when property contains something other than name of
    /// extern instance. ListExpression is required because without it Expression will take
    /// precedence over it and throw error for whole list.
    bool preorder(const IR::ListExpression *) override { return true; }
    bool preorder(const IR::Expression *expr) override {
        ::P4::error(ErrorType::ERR_UNSUPPORTED,
                    "%1%: unsupported expression, expected a named instance", expr);
        return false;
    }

    void visitTableProperty(cstring propertyName) {
        auto property = table->table->container->properties->getProperty(propertyName);
        if (property != nullptr) property->apply(*this);
    }
};

}  // namespace P4::EBPF

#endif /* BACKENDS_EBPF_PSA_EBPFPSATABLE_H_ */
