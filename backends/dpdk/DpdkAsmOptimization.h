#ifndef BACKEND_DPDK_OPTIMIZATION_H_
#define BACKEND_DPDK_OPTIMIZATION_H_

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
namespace DPDK{
class RemoveRedundantLabel: public Transform {
public:
    const IR::Node *postorder(IR::DpdkListStatement *l) override;
};

class RemoveUselessJmpAndLabel: public Transform {
public:
    const IR::Node *postorder(IR::DpdkListStatement *l) override;
};

class DpdkAsmOptimization: public PassManager{
private:
public:
    DpdkAsmOptimization(){
        passes.push_back(new RemoveRedundantLabel);
        auto *r = new PassRepeated{new RemoveUselessJmpAndLabel};
        passes.push_back(r);
    }
};

} //namespace DPDK
#endif