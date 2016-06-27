/*
Copyright 2013-present Barefoot Networks, Inc.

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

#include <iostream>
#include <fstream>

#include "ir/ir.h"
#include "../common/options.h"
#include "lib/nullstream.h"
#include "lib/path.h"
#include "frontend.h"

#include "frontends/p4/typeMap.h"
#include "frontends/p4/typeChecking/bindVariables.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
// Passes
#include "toP4/toP4.h"
#include "validateParsedProgram.h"
#include "createBuiltins.h"
#include "frontends/common/constantFolding.h"
#include "unusedDeclarations.h"
#include "checkAliasing.h"
#include "typeChecking/typeChecker.h"
#include "evaluator/evaluator.h"
#include "strengthReduction.h"
#include "simplify.h"

const IR::P4Program*
FrontEnd::run(const CompilerOptions &options, const IR::P4Program* program) {
    if (program == nullptr)
        return nullptr;

    bool isv1 = options.isv1();

    Util::PathName path(options.prettyPrintFile);
    std::ostream *ppStream = openFile(path.toString(), true);

    P4::ReferenceMap  refMap;  // This is reused many times, since every analysis clear it
    P4::TypeMap       typeMap;

    PassManager passes = {
        new P4::ToP4(ppStream, false, options.file),  // TODO: don't always run this
        // Simple checks on parsed program
        new P4::ValidateParsedProgram(isv1),
        // Synthesize some built-in constructs
        new P4::CreateBuiltins(),
        // First pass of constant folding, before types are known
        // May be needed to compute types
        new P4::ResolveReferences(&refMap, isv1, true),
        new P4::ConstantFolding(&refMap, nullptr),
        new P4::ResolveReferences(&refMap, isv1),
        // Type checking and type inference.  Also inserts
        // explicit casts where implicit casts exist.
        new P4::TypeInference(&refMap, &typeMap, true, false),
        new P4::BindTypeVariables(&refMap, &typeMap),
        // Another round of constant folding, using type information.
        new P4::TypeChecking(&refMap, &typeMap, true, isv1),
        new P4::ConstantFolding(&refMap, &typeMap),
        new P4::StrengthReduction(),
        new P4::TypeChecking(&refMap, &typeMap, false, isv1),
        new P4::CheckAliasing(&refMap, &typeMap),
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        new P4::RemoveAllUnusedDeclarations(&refMap, isv1),
    };

    passes.setName("FrontEnd");
    passes.setStopOnError(true);
    passes.addDebugHooks(hooks);
    const IR::P4Program* result = program->apply(passes);
    return result;
}
