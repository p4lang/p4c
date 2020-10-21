#ifndef BACKEND_DPDK_VARIABLE_COLLECTOR_H_
#define BACKEND_DPDK_VARIABLE_COLLECTOR_H_

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

class DpdkVariableCollector {
private:
  int next_tmp_id;
  IR::IndexedVector<IR::DpdkDeclaration> variables;

public:
  DpdkVariableCollector() { next_tmp_id = 0; }
  // cstring get_next_tmp(IR::Expression* type);
  cstring get_next_tmp();
  void push_variable(const IR::DpdkDeclaration *);
  IR::IndexedVector<IR::DpdkDeclaration> &get_globals() { return variables; }
};

} // namespace DPDK
#endif