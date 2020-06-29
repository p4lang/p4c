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

#ifndef BACKENDS_DPDK_BACKEND_H_
#define BACKENDS_DPDK_BACKEND_H_

#include "ir/ir.h"
#include "lib/gmputil.h"
#include "lib/json.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/enumInstance.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/unusedDeclarations.h"
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

namespace DPDK {

class ConvertToDpdkProgram : public Transform {
    int next_reg_id = 0;
    int next_label_id = 0;
    std::map<int, cstring> reg_id_to_name;
    std::map<cstring, int> reg_name_to_id;
    std::map<cstring, cstring> symbol_table;

    BMV2::PsaProgramStructure& structure;
    const IR::DpdkAsmProgram* dpdk_program;

 public:
    ConvertToDpdkProgram(BMV2::PsaProgramStructure& structure) : structure(structure) {}

    const IR::DpdkAsmProgram* create();
    const IR::DpdkAsmStatement* createListStatement(cstring name,
            std::initializer_list<IR::IndexedVector<IR::DpdkAsmStatement>> statements);
    int next_free_reg() { return next_reg_id++; }
    const IR::Node* preorder(IR::P4Program* p) override;
    const IR::DpdkAsmProgram* getDpdkProgram() { return dpdk_program; }
};

class ConvertToDpdkParser : public Inspector {
    IR::IndexedVector<IR::DpdkAsmStatement> instructions;
 public:
    ConvertToDpdkParser() {}
    IR::IndexedVector<IR::DpdkAsmStatement> getInstructions() { return instructions; }
    bool preorder(const IR::P4Parser* a) override;
    bool preorder(const IR::ParserState* s) override;
    void add_instr(const IR::DpdkAsmStatement* s){ instructions.push_back(s); }
};

class ConvertToDpdkControl : public Inspector {
    int next_reg_id = 0;
    int next_label_id = 0;
    IR::IndexedVector<IR::DpdkAsmStatement> instructions;
    IR::IndexedVector<IR::DpdkTable> tables;
    IR::IndexedVector<IR::DpdkAction> actions;
 public:
    ConvertToDpdkControl() {}

    IR::IndexedVector<IR::DpdkTable>& getTables() { return tables; }
    IR::IndexedVector<IR::DpdkAction>& getActions() { return actions; }
    IR::IndexedVector<IR::DpdkAsmStatement>& getInstructions() { return instructions; }

    bool preorder(const IR::P4Action* a) override;
    bool preorder(const IR::P4Table* a) override;
    bool preorder(const IR::P4Control*) override;

    void add_inst(const IR::DpdkAsmStatement* s) { instructions.push_back(s); }
    void add_table(const IR::DpdkTable* t) { tables.push_back(t); }
    void add_action(const IR::DpdkAction* a) { actions.push_back(a); }
};

class PsaSwitchBackend : public BMV2::Backend {
    BMV2::BMV2Options &options;
    const IR::DpdkAsmProgram* dpdk_program = nullptr;

 public:
    void convert(const IR::ToplevelBlock* tlb) override;
    PsaSwitchBackend(BMV2::BMV2Options& options, P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
                          P4::ConvertEnums::EnumMapping* enumMap) :
        Backend(options, refMap, typeMap, enumMap), options(options) { }
    void codegen(std::ostream&) const;
};

}  // namespace DPDK

#endif  /* BACKENDS_DPDK_BACKEND_H_ */
