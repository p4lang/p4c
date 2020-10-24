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
  int next_label_id = 0;
  std::map<int, cstring> reg_id_to_name;
  std::map<cstring, int> reg_name_to_id;
  std::map<cstring, cstring> symbol_table;

  BMV2::PsaProgramStructure &structure;
  P4::TypeMap *typemap;
  P4::ReferenceMap *refmap;
  DpdkVariableCollector *collector;
  const IR::DpdkAsmProgram *dpdk_program;
  CollectMetadataHeaderInfo *info;
  std::map<const cstring, IR::IndexedVector<IR::Parameter> *> *args_struct_map;
  std::map<const IR::Declaration_Instance *, cstring> *csum_map;

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

public:
  ConvertToDpdkParser(
      P4::ReferenceMap *refmap, P4::TypeMap *typemap,
      DpdkVariableCollector *collector,
      std::map<const IR::Declaration_Instance *, cstring> *csum_map)
      : refmap(refmap), typemap(typemap), collector(collector),
        csum_map(csum_map) {}
  IR::IndexedVector<IR::DpdkAsmStatement> getInstructions() {
    return instructions;
  }
  bool preorder(const IR::P4Parser *a) override;
  bool preorder(const IR::ParserState *s) override;
  void add_instr(const IR::DpdkAsmStatement *s) { instructions.push_back(s); }
};

class ConvertToDpdkControl : public Inspector {
  P4::TypeMap *typemap;
  P4::ReferenceMap *refmap;
  DpdkVariableCollector *collector;
  int next_label_id = 0;
  IR::IndexedVector<IR::DpdkAsmStatement> instructions;
  IR::IndexedVector<IR::DpdkTable> tables;
  IR::IndexedVector<IR::DpdkAction> actions;
  std::map<const IR::Declaration_Instance *, cstring> *csum_map;

public:
  ConvertToDpdkControl(
      P4::ReferenceMap *refmap, P4::TypeMap *typemap,
      DpdkVariableCollector *collector,
      std::map<const IR::Declaration_Instance *, cstring> *csum_map)
      : typemap(typemap), refmap(refmap), collector(collector),
        csum_map(csum_map) {}

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

} // namespace DPDK
#endif
