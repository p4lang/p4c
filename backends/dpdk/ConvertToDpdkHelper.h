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

#define TOSTR_DECLA(NAME) std::ostream& toStr(std::ostream&, IR::NAME*)

namespace DPDK{
class ConvertStatementToDpdk : public Inspector {
    int next_label_id;
    int *next_tmp_id;
    IR::IndexedVector<IR::DpdkAsmStatement> instructions;
    P4::ReferenceMap *refmap;
    P4::TypeMap *typemap;
 public:
    ConvertStatementToDpdk(
        P4::ReferenceMap *refmap, 
        P4::TypeMap *typemap, 
        int next_label_id,
        int *next_tmp_id): 
        refmap(refmap), 
        typemap(typemap), 
        next_label_id(next_label_id),
        next_tmp_id(next_tmp_id){}
    IR::IndexedVector<IR::DpdkAsmStatement> getInstructions() { return instructions; }

    bool preorder(const IR::AssignmentStatement* a) override;
    bool preorder(const IR::BlockStatement* a) override;
    bool preorder(const IR::IfStatement* a) override;
    bool preorder(const IR::MethodCallStatement* a) override;

    void add_instr(const IR::DpdkAsmStatement* s){ instructions.push_back(s); }
    IR::IndexedVector<IR::DpdkAsmStatement> & get_instr(){return instructions;}
    int get_label_num(){return next_label_id;}
};

cstring toStr(const IR::Expression *const);
cstring toStr(const IR::Type *const);
cstring toStr(const IR::Argument* const);


class TreeUnroller: public Inspector {
    IR::IndexedVector<IR::DpdkAsmStatement> instructions;
    int* next_tmp_id;
    IR::PathExpression *tmp;
    P4::ReferenceMap *refmap;
    P4::TypeMap *typemap;
public:
    TreeUnroller(int* next_tmp_id, P4::ReferenceMap *refmap, P4::TypeMap *typemap): next_tmp_id(next_tmp_id), refmap(refmap), typemap(typemap){}
    bool preorder(const IR::Operation_Binary*) override;
    bool preorder(const IR::MethodCallExpression*) override;
    bool preorder(const IR::Member*) override;
    bool preorder(const IR::PathExpression*) override;
    bool preorder(const IR::Constant*) override;
    void add_instr(const IR::DpdkAsmStatement* s){ instructions.push_back(s); }
    IR::IndexedVector<IR::DpdkAsmStatement> & get_instr(){return instructions;}
    IR::Expression* get_tmp(){return tmp;}
};



} // namespace DPDK
#endif