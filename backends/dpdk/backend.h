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
class PsaSwitchBackend : public BMV2::Backend {
  BMV2::BMV2Options &options;
  const IR::DpdkAsmProgram *dpdk_program = nullptr;

public:
  void convert(const IR::ToplevelBlock *tlb) override;
  PsaSwitchBackend(BMV2::BMV2Options &options, P4::ReferenceMap *refMap,
                   P4::TypeMap *typeMap, P4::ConvertEnums::EnumMapping *enumMap)
      : Backend(options, refMap, typeMap, enumMap), options(options) {}
  void codegen(std::ostream &) const;
};

} // namespace DPDK

#endif /* BACKENDS_DPDK_BACKEND_H_ */
