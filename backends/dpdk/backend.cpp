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
#include "backend.h"
#include <unordered_map>
#include "backends/bmv2/psa_switch/psaSwitch.h"
#include "elimTypedef.h"
#include "ir/dbprint.h"
#include "ir/ir.h"
#include "lib/stringify.h"

namespace DPDK {

void PsaSwitchBackend::convert(const IR::ToplevelBlock *tlb) {
  CHECK_NULL(tlb);
  BMV2::PsaProgramStructure structure(refMap, typeMap);
  auto parsePsaArch = new BMV2::ParsePsaArchitecture(&structure);
  auto main = tlb->getMain();
  if (!main)
    return;

  if (main->type->name != "PSA_Switch")
    ::warning(ErrorType::WARN_INVALID,
              "%1%: the main package should be called PSA_Switch"
              "; are you using the wrong architecture?",
              main->type->name);

  main->apply(*parsePsaArch);

  auto evaluator = new P4::EvaluatorPass(refMap, typeMap);
  auto program = tlb->getProgram();
  PassManager simplify = {
      new P4::EliminateTypedef(refMap, typeMap),
      // because the user metadata type has changed
      new P4::ClearTypeMap(typeMap),
      new P4::SynthesizeActions(
          refMap, typeMap,
          new BMV2::SkipControls(&structure.non_pipeline_controls)),
      new P4::MoveActionsToTables(refMap, typeMap),
      new P4::TypeChecking(refMap, typeMap),
      new BMV2::LowerExpressions(typeMap),
      new P4::ConstantFolding(refMap, typeMap, false),
      new P4::TypeChecking(refMap, typeMap),
      new BMV2::RemoveComplexExpressions(
          refMap, typeMap,
          new BMV2::ProcessControls(&structure.pipeline_controls)),
      new P4::RemoveAllUnusedDeclarations(refMap),
      evaluator,
      new VisitFunctor([this, evaluator, structure]() {
        toplevel = evaluator->getToplevelBlock();
      }),
  };
  auto hook = options.getDebugHook();
  simplify.addDebugHook(hook, true);
  program = program->apply(simplify);

  // map IR node to compile-time allocated resource blocks.
  toplevel->apply(*new BMV2::BuildResourceMap(&structure.resourceMap));

  main = toplevel->getMain();
  if (!main)
    return;  // no main
  main->apply(*parsePsaArch);
  program = toplevel->getProgram();
}
}  // namespace DPDK
