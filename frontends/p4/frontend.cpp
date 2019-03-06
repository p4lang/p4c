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
#include "actionsInlining.h"
#include "checkConstants.h"
#include "checkNamedArgs.h"
#include "createBuiltins.h"
#include "defaultArguments.h"
#include "deprecated.h"
#include "directCalls.h"
#include "dontcareArgs.h"
#include "evaluator/evaluator.h"
#include "frontends/common/constantFolding.h"
#include "functionsInlining.h"
#include "hierarchicalNames.h"
#include "inlining.h"
#include "localizeActions.h"
#include "moveConstructors.h"
#include "moveDeclarations.h"
#include "parseAnnotations.h"
#include "parserControlFlow.h"
#include "removeReturns.h"
#include "resetHeaders.h"
#include "setHeaders.h"
#include "sideEffects.h"
#include "simplify.h"
#include "simplifyDefUse.h"
#include "simplifyParsers.h"
#include "specialize.h"
#include "strengthReduction.h"
#include "structInitializers.h"
#include "tableKeyNames.h"
#include "toP4/toP4.h"
#include "typeChecking/typeChecker.h"
#include "uniqueNames.h"
#include "unusedDeclarations.h"
#include "uselessCasts.h"
#include "validateMatchAnnotations.h"
#include "validateParsedProgram.h"

namespace P4 {

namespace {
/**
This pass outputs the program as a P4 source file.
*/
class PrettyPrint : public Inspector {
    /// output file
    cstring ppfile;
    /// The file that is being compiled.  This used
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

/**
 * This pass is a no-op whose purpose is to mark the end of the
 * front-end, which is useful for debugging. It is implemented as an
 * empty @ref PassManager (instead of a @ref Visitor) for efficiency.
 */
class FrontEndLast : public PassManager {
 public:
    FrontEndLast() { setName("FrontEndLast"); }
};

/**
 * This pass is a no-op whose purpose is to mark a point in the
 * front-end, used for testing.
 */
class FrontEndDump : public PassManager {
 public:
    FrontEndDump() { setName("FrontEndDump"); }
};

// TODO: remove skipSideEffectOrdering flag
const IR::P4Program *FrontEnd::run(const CompilerOptions &options, const IR::P4Program* program,
                                   bool skipSideEffectOrdering) {
    if (program == nullptr)
        return nullptr;

    bool isv1 = options.isv1();
    ReferenceMap  refMap;
    TypeMap       typeMap;
    refMap.setIsV1(isv1);

    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);

    PassManager passes = {
        // Parse annotations
        new ParseAnnotationBodies(&parseAnnotations, &typeMap),
        new PrettyPrint(options),
        // Simple checks on parsed program
        new ValidateParsedProgram(),
        // Synthesize some built-in constructs
        new CreateBuiltins(),
        new ResolveReferences(&refMap, true),  // check shadowing
        // First pass of constant folding, before types are known --
        // may be needed to compute types.
        new ConstantFolding(&refMap, nullptr),
        // Desugars direct parser and control applications
        // into instantiations followed by application
        new InstantiateDirectCalls(&refMap),
        // Type checking and type inference.  Also inserts
        // explicit casts where implicit casts exist.
        new ResolveReferences(&refMap),  // check shadowing
        new Deprecated(&refMap),
        new CheckNamedArgs(),
        new TypeInference(&refMap, &typeMap, false),  // insert casts
        new ValidateMatchAnnotations(&typeMap),
        new DefaultArguments(&refMap, &typeMap),  // add default argument values to parameters
        new BindTypeVariables(&refMap, &typeMap),
        new StructInitializers(&refMap, &typeMap),
        new TableKeyNames(&refMap, &typeMap),
        // Another round of constant folding, using type information.
        new ConstantFolding(&refMap, &typeMap),
        new StrengthReduction(&refMap, &typeMap),
        new UselessCasts(&refMap, &typeMap),
        new SimplifyControlFlow(&refMap, &typeMap),
        new FrontEndDump(),  // used for testing the program at this point
        new RemoveAllUnusedDeclarations(&refMap, true),
        new SimplifyParsers(&refMap),
        new ResetHeaders(&refMap, &typeMap),
        new UniqueNames(&refMap),  // Give each local declaration a unique internal name
        new MoveDeclarations(),  // Move all local declarations to the beginning
        new MoveInitializers(),
        new SideEffectOrdering(&refMap, &typeMap, skipSideEffectOrdering),
        new SetHeaders(&refMap, &typeMap),
        new SimplifyControlFlow(&refMap, &typeMap),
        new MoveDeclarations(),  // Move all local declarations to the beginning
        new SimplifyDefUse(&refMap, &typeMap),
        new UniqueParameters(&refMap, &typeMap),
        new SimplifyControlFlow(&refMap, &typeMap),
        new SpecializeAll(&refMap, &typeMap),
        new RemoveParserControlFlow(&refMap, &typeMap),
        new RemoveReturns(&refMap),
        new RemoveDontcareArgs(&refMap, &typeMap),
        new MoveConstructors(&refMap),
        new RemoveAllUnusedDeclarations(&refMap),
        new ClearTypeMap(&typeMap),
        evaluator,
        new Inline(&refMap, &typeMap, evaluator),
        new InlineActions(&refMap, &typeMap),
        new InlineFunctions(&refMap, &typeMap),
        // Check for constants only after inlining
        new CheckConstants(&refMap, &typeMap),
        new SimplifyControlFlow(&refMap, &typeMap),
        new RemoveParserControlFlow(&refMap, &typeMap),
        new UniqueNames(&refMap),
        new LocalizeAllActions(&refMap),
        new UniqueNames(&refMap),  // needed again after inlining
        new UniqueParameters(&refMap, &typeMap),
        new SimplifyControlFlow(&refMap, &typeMap),
        new HierarchicalNames(),
        new FrontEndLast(),
    };

    passes.setName("FrontEnd");
    passes.setStopOnError(true);
    passes.addDebugHooks(hooks);
    const IR::P4Program* result = program->apply(passes);
    return result;
}

}  // namespace P4
