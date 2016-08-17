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

namespace P4 {

namespace {
class PrettyPrint : public Inspector {
    cstring ppfile;
    cstring inputfile;
 public:
    explicit PrettyPrint(const CompilerOptions& options) {
        setName("PrettyPrint");
        ppfile = options.prettyPrintFile;
        inputfile = options.file;
    }
    bool preorder(const IR::P4Program* program) override {
        if (!ppfile.isNullOrEmpty()) {
            Util::PathName path(ppfile);
            std::ostream *ppStream = openFile(path.toString(), true);
            P4::ToP4 top4(ppStream, false, inputfile);
            (void)program->apply(top4);
        }
        return false;  // prune
    }
};
}  // namespace

const IR::P4Program*
FrontEnd::run(const CompilerOptions &options, const IR::P4Program* program) {
    if (program == nullptr)
        return nullptr;

    bool isv1 = options.isv1();
    ReferenceMap  refMap;
    TypeMap       typeMap;

    PassManager passes = {
        new PrettyPrint(options),
        // Simple checks on parsed program
        new ValidateParsedProgram(isv1),
        // Synthesize some built-in constructs
        new CreateBuiltins(),
        new ResolveReferences(&refMap, isv1, true),  // check shadowing
        // First pass of constant folding, before types are known --
        // may be needed to compute types.
        new ConstantFolding(&refMap, nullptr, isv1),
        // Type checking and type inference.  Also inserts
        // explicit casts where implicit casts exist.
        new ResolveReferences(&refMap, isv1),
        new TypeInference(&refMap, &typeMap),
        new BindTypeVariables(&refMap, &typeMap),
        // Another round of constant folding, using type information.
        new ClearTypeMap(&typeMap),
        new ConstantFolding(&refMap, &typeMap, isv1),
        new StrengthReduction(),
        new CheckAliasing(&refMap, &typeMap, isv1),
        new SimplifyControlFlow(&refMap, &typeMap, isv1),
        new RemoveAllUnusedDeclarations(&refMap, isv1),
    };

    passes.setName("FrontEnd");
    passes.setStopOnError(true);
    passes.addDebugHooks(hooks);
    const IR::P4Program* result = program->apply(passes);
    return result;
}

}  // namespace P4
