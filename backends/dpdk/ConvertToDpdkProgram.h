#ifndef BACKENDS_DPDK_PROGRAM_H_
#define BACKENDS_DPDK_PROGRAM_H_


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
#include "DpdkVariableCollector.h"

namespace DPDK
{
    
class ConvertToDpdkProgram : public Transform {
    DpdkVariableCollector collector;
    int next_label_id = 0;
    std::map<int, cstring> reg_id_to_name;
    std::map<cstring, int> reg_name_to_id;
    std::map<cstring, cstring> symbol_table;

    BMV2::PsaProgramStructure& structure;
    const IR::DpdkAsmProgram* dpdk_program;
    P4::ReferenceMap *refmap;
    P4::TypeMap *typemap;

 public:
    ConvertToDpdkProgram(
        BMV2::PsaProgramStructure& structure, 
        P4::ReferenceMap *refmap, 
        P4::TypeMap * typemap) : structure(structure), refmap(refmap), typemap(typemap) {}

    const IR::DpdkAsmProgram* create();
    const IR::DpdkAsmStatement* createListStatement(cstring name,
            std::initializer_list<IR::IndexedVector<IR::DpdkAsmStatement>> statements);
    const IR::Node* preorder(IR::P4Program* p) override;
    const IR::DpdkAsmProgram* getDpdkProgram() { return dpdk_program; }
};

class ConvertToDpdkParser : public Inspector {
    IR::IndexedVector<IR::DpdkAsmStatement> instructions;
    P4::ReferenceMap *refmap;
    P4::TypeMap *typemap;
    DpdkVariableCollector *collector;
 public:
    ConvertToDpdkParser(P4::ReferenceMap *refmap, P4::TypeMap *typemap, DpdkVariableCollector *collector): refmap(refmap), typemap(typemap), collector(collector) {}
    IR::IndexedVector<IR::DpdkAsmStatement> getInstructions() { return instructions; }
    bool preorder(const IR::P4Parser* a) override;
    bool preorder(const IR::ParserState* s) override;
    void add_instr(const IR::DpdkAsmStatement* s){ instructions.push_back(s); }
};

class ConvertToDpdkControl : public Inspector {
    DpdkVariableCollector *collector;
    int next_label_id = 0;
    IR::IndexedVector<IR::DpdkAsmStatement> instructions;
    IR::IndexedVector<IR::DpdkTable> tables;
    IR::IndexedVector<IR::DpdkAction> actions;
    P4::ReferenceMap *refmap;
    P4::TypeMap *typemap;
 public:
    ConvertToDpdkControl(P4::ReferenceMap *refmap, P4::TypeMap *typemap, DpdkVariableCollector *collector): refmap(refmap), typemap(typemap), collector(collector) {}

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

} // namespace DPDK
#endif