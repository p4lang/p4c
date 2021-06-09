/*
Copyright 2020 Intel Corp.

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

#ifndef BACKENDS_DPDK_PROGRAM_H_
#define BACKENDS_DPDK_PROGRAM_H_

#include "backends/bmv2/common/action.h"
#include "backends/bmv2/common/control.h"
#include "backends/bmv2/common/deparser.h"
#include "backends/bmv2/common/extern.h"
#include "backends/bmv2/common/header.h"
#include "backends/bmv2/common/helpers.h"
#include "backends/bmv2/common/lower.h"
#include "backends/bmv2/common/parser.h"
#include "backends/bmv2/common/programStructure.h"
#include "backends/bmv2/psa_switch/psaSwitch.h"
#include "dpdkArch.h"
#include "dpdkVarCollector.h"
#include "frontends/common/constantFolding.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/enumInstance.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/unusedDeclarations.h"
#include "ir/ir.h"
#include "lib/gmputil.h"
#include "lib/json.h"
namespace DPDK {

class ConvertToDpdkProgram : public Transform {
    std::map<int, cstring> reg_id_to_name;
    std::map<cstring, int> reg_name_to_id;
    std::map<cstring, cstring> symbol_table;

    BMV2::PsaProgramStructure &structure;
    P4::TypeMap *typemap;
    P4::ReferenceMap *refmap;
    DpdkVariableCollector *collector;
    const IR::DpdkAsmProgram *dpdk_program;
    CollectMetadataHeaderInfo *info;
    std::map<const cstring, IR::IndexedVector<IR::Parameter> *>
        *args_struct_map;
    std::map<const IR::Declaration_Instance *, cstring> *csum_map;
    std::map<const IR::Declaration_Instance *, cstring> *reg_map;

  public:
    ConvertToDpdkProgram(BMV2::PsaProgramStructure &structure,
                         P4::ReferenceMap *refmap, P4::TypeMap *typemap,
                         DpdkVariableCollector *collector,
                         DPDK::RewriteToDpdkArch *dpdkarch)
        : structure(structure), typemap(typemap), refmap(refmap),
          collector(collector) {
        info = dpdkarch->info;
        args_struct_map = dpdkarch->args_struct_map;
        csum_map = dpdkarch->csum_map;
        reg_map = dpdkarch->reg_map;
    }

    const IR::DpdkAsmProgram *create(IR::P4Program *prog);
    const IR::DpdkAsmStatement *createListStatement(
        cstring name,
        std::initializer_list<IR::IndexedVector<IR::DpdkAsmStatement>>
            statements);
    const IR::Node *preorder(IR::P4Program *p) override;
    const IR::DpdkAsmProgram *getDpdkProgram() { return dpdk_program; }
};

class ConvertToDpdkParser : public Inspector {
    IR::IndexedVector<IR::DpdkAsmStatement> instructions;
    P4::ReferenceMap *refmap;
    P4::TypeMap *typemap;
    DpdkVariableCollector *collector;
    std::map<const IR::Declaration_Instance *, cstring> *csum_map;

    IR::Type_Struct *metadataStruct;

  public:
    ConvertToDpdkParser(
        P4::ReferenceMap *refmap, P4::TypeMap *typemap,
        DpdkVariableCollector *collector,

        std::map<const IR::Declaration_Instance *, cstring> *csum_map, IR::Type_Struct *metadataStruct)
        : refmap(refmap), typemap(typemap), collector(collector),
          csum_map(csum_map), metadataStruct(metadataStruct) {}
    IR::IndexedVector<IR::DpdkAsmStatement> getInstructions() {
        return instructions;
    }
    bool preorder(const IR::P4Parser *a) override;
    bool preorder(const IR::ParserState *s) override;
    void add_instr(const IR::DpdkAsmStatement *s) { instructions.push_back(s); }
    cstring append_parser_name(const IR::P4Parser* p, cstring);
    void add_metadata_field(IR::Declaration_Variable *d);
};

class ConvertToDpdkControl : public Inspector {
    P4::TypeMap *typemap;
    P4::ReferenceMap *refmap;
    DpdkVariableCollector *collector;
    IR::IndexedVector<IR::DpdkAsmStatement> instructions;
    IR::IndexedVector<IR::DpdkTable> tables;
    IR::IndexedVector<IR::DpdkAction> actions;
    std::map<const IR::Declaration_Instance *, cstring> *csum_map;
    std::set<cstring> unique_actions;
    bool deparser;

  public:
    ConvertToDpdkControl(
        P4::ReferenceMap *refmap, P4::TypeMap *typemap,
        DpdkVariableCollector *collector,
        std::map<const IR::Declaration_Instance *, cstring> *csum_map,
        bool deparser = false)
        : typemap(typemap), refmap(refmap), collector(collector),
          csum_map(csum_map), deparser(deparser) {}

    IR::IndexedVector<IR::DpdkTable> &getTables() { return tables; }
    IR::IndexedVector<IR::DpdkAction> &getActions() { return actions; }
    IR::IndexedVector<IR::DpdkAsmStatement> &getInstructions() {
        return instructions;
    }

    bool preorder(const IR::P4Action *a) override;
    bool preorder(const IR::P4Table *a) override;
    bool preorder(const IR::P4Control *) override;

    void add_inst(const IR::DpdkAsmStatement *s) { instructions.push_back(s); }
    void add_table(const IR::DpdkTable *t) { tables.push_back(t); }
    void add_action(const IR::DpdkAction *a) { actions.push_back(a); }
};

class CollectActionUses : public Inspector {
    ordered_set<cstring>& actions;

 public:
    CollectActionUses(ordered_set<cstring>& a) : actions(a) { }
    bool preorder(const IR::ActionListElement* ale) {
        if (auto mce = ale->expression->to<IR::MethodCallExpression>()) {
            if (auto path = mce->method->to<IR::PathExpression>()) {
                actions.insert(path->path->name.toString());
            }
        }
        return false;
    }
};

class ElimUnusedActions : public Transform {
    const ordered_set<cstring>& used_actions;
    std::set<cstring> kept_actions;

 public:
    ElimUnusedActions(const ordered_set<cstring>& a) : used_actions(a) {}
    const IR::Node* postorder(IR::DpdkAction* a) override {
        if (kept_actions.count(a->name.toString()) != 0)
            return nullptr;
        if (used_actions.find(a->name.toString()) != used_actions.end()) {
            kept_actions.insert(a->name.toString());
            return a; }
        return nullptr;
    }
};

// frontend generates multiple copies of the same action in the localizeActions pass,
// they are not useful for dpdk datapath. Removed the duplicated actions in
// this pass.
class EliminateUnusedAction : public PassManager {
    ordered_set<cstring> actions;

 public:
    EliminateUnusedAction() {
        addPasses( {
            new CollectActionUses(actions),
            new ElimUnusedActions(actions)
        });
    }
};

} // namespace DPDK
#endif
