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

class RemoveConsecutiveJmpAndLabel : public Transform {
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

class ThreadJumps : public Transform {
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
    passes.push_back(new RemoveConsecutiveJmpAndLabel);
    passes.push_back(new RemoveRedundantLabel);
    passes.push_back(r);
    passes.push_back(new ThreadJumps);
  }
};

} // namespace DPDK
#endif
