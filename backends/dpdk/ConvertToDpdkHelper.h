#ifndef BACKENDS_DPDK_HELPER_H_
#define BACKENDS_DPDK_HELPER_H_

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


#define TOSTR_DECLA(NAME) std::ostream& toStr(std::ostream&, IR::NAME*)

namespace DPDK{
class BranchingInstructionGeneration {
    int *next_label_id;
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
public:
    IR::IndexedVector<IR::DpdkAsmStatement> instructions;
    BranchingInstructionGeneration(
        int *next_label_id,
        P4::ReferenceMap *refMap,
        P4::TypeMap *typeMap): 
        next_label_id(next_label_id),
        refMap(refMap),
        typeMap(typeMap){}
    void generate(const IR::Expression *, cstring, cstring);
};

class ConvertStatementToDpdk : public Inspector {
    int next_label_id;
    DpdkVariableCollector *collector;
    IR::IndexedVector<IR::DpdkAsmStatement> instructions;
    P4::ReferenceMap *refmap;
    P4::TypeMap *typemap;
    std::map<const IR::Declaration_Instance *, cstring> *csum_map;
 public:
    ConvertStatementToDpdk(
        P4::ReferenceMap *refmap,
        P4::TypeMap *typemap,
        int next_label_id,
        DpdkVariableCollector *collector,
        std::map<const IR::Declaration_Instance *, cstring> *csum_map):
        refmap(refmap),
        typemap(typemap),
        next_label_id(next_label_id),
        collector(collector),
        csum_map(csum_map){}
    IR::IndexedVector<IR::DpdkAsmStatement> getInstructions() { return instructions; }
    void branchingInstructionGeneration(cstring true_label, cstring false_label, const IR::Expression * expr);
    bool preorder(const IR::AssignmentStatement* a) override;
    bool preorder(const IR::BlockStatement* a) override;
    bool preorder(const IR::IfStatement* a) override;
    bool preorder(const IR::MethodCallStatement* a) override;

    void add_instr(const IR::DpdkAsmStatement* s){ instructions.push_back(s); }
    IR::IndexedVector<IR::DpdkAsmStatement> & get_instr(){return instructions;}
    int get_label_num(){return next_label_id;}
};

} // namespace DPDK
#endif
