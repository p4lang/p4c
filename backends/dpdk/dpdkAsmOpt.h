#ifndef BACKEND_DPDK_OPTIMIZATION_H_
#define BACKEND_DPDK_OPTIMIZATION_H_

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
// This pass removes label that no jmps jump to
class RemoveRedundantLabel : public Transform {
public:
  const IR::Node *postorder(IR::DpdkListStatement *l) override;
};

// This pass removes jmps that jump to a label that is immediately after it.
// For example,
// (jmp label1)
// (label1)
//
// jmp label1 will be removed.

class RemoveUselessJmpAndLabel : public Transform {
public:
  const IR::Node *postorder(IR::DpdkListStatement *l) override;
};

// This pass removes labels whose next instruction is a jmp statement. This pass
// will update any kinds of jmp(conditional + unconditional) that jump to this
// label to the label that jmp statement jump to. For example:
// jeq label1
// ...
// ...
// label1
// jmp label2
//
// will become:
// jeq label2
// ...
// ...
// jmp label2

class RemoveJmpAfterLabel : public Transform {
public:
  const IR::Node *postorder(IR::DpdkListStatement *l) override;
};

// This pass removes labels whose next instruction is a label. In addition, it
// will update any jmp that jump to this label to the label next to it.

class RemoveLabelAfterLabel : public Transform {
public:
  const IR::Node *postorder(IR::DpdkListStatement *l) override;
};

class DpdkAsmOptimization : public PassRepeated {
private:
public:
  DpdkAsmOptimization() {
    passes.push_back(new RemoveRedundantLabel);
    auto r = new PassRepeated{new RemoveLabelAfterLabel};
    passes.push_back(r);
    passes.push_back(new RemoveUselessJmpAndLabel);
    passes.push_back(new RemoveRedundantLabel);
    passes.push_back(r);
    passes.push_back(new RemoveJmpAfterLabel);
  }
};

} // namespace DPDK
#endif