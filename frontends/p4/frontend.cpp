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
#include "frontends/p4/fromv1.0/v1model.h"
#include "ubpf/ubpfModel.h"
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
#include "parserControlFlow.h"
#include "removeReturns.h"
#include "resetHeaders.h"
#include "setHeaders.h"
#include "sideEffects.h"
#include "simplify.h"
#include "simplifyDefUse.h"
#include "simplifyParsers.h"
#include "specialize.h"
#include "specializeGenericFunctions.h"
#include "strengthReduction.h"
#include "structInitializers.h"
#include "switchAddDefault.h"
#include "tableKeyNames.h"
#include "typeChecking/typeChecker.h"
#include "uniqueNames.h"
#include "unusedDeclarations.h"
#include "uselessCasts.h"
#include "validateMatchAnnotations.h"
#include "validateParsedProgram.h"

namespace P4 {

// TODO: remove skipSideEffectOrdering flag
const IR::P4Program *FrontEnd::run(const CompilerOptions &options, const IR::P4Program *program,
                                   bool skipSideEffectOrdering, std::ostream *outStream) {
    if (program == nullptr && options.listFrontendPasses == 0)
        return nullptr;

    bool isv1 = options.isv1();
    ReferenceMap refMap;
    TypeMap typeMap;
    refMap.setIsV1(isv1);

    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);
    std::initializer_list<Visitor *> frontendPasses = {
            new P4V1::getV1ModelVersion,
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
            new ResolveReferences(&refMap),  // check shadowing
            new Deprecated(&refMap),
            new CheckNamedArgs(),
            // Type checking and type inference.  Also inserts
            // explicit casts where implicit casts exist.
            new TypeInference(&refMap, &typeMap, false),  // insert casts
            new ValidateMatchAnnotations(&typeMap),
            new BindTypeVariables(&refMap, &typeMap),
            new DefaultArguments(&refMap, &typeMap),  // add default argument values to parameters
            new ResolveReferences(&refMap),
            new TypeInference(&refMap, &typeMap, false),  // more casts may be needed
            new RemoveParserControlFlow(&refMap, &typeMap),
            new StructInitializers(&refMap, &typeMap),
            new SpecializeGenericFunctions(&refMap, &typeMap),
            new TableKeyNames(&refMap, &typeMap),
            new PassRepeated({
                                     new ConstantFolding(&refMap, &typeMap),
                                     new StrengthReduction(&refMap, &typeMap),
                                     new UselessCasts(&refMap, &typeMap)
                             }),
            new SimplifyControlFlow(&refMap, &typeMap),
            new SwitchAddDefault,
            new FrontEndDump(),  // used for testing the program at this point
            new RemoveAllUnusedDeclarations(&refMap, true),
            new SimplifyParsers(&refMap),
            new ResetHeaders(&refMap, &typeMap),
            new UniqueNames(&refMap),  // Give each local declaration a unique internal name
            new MoveDeclarations(),  // Move all local declarations to the beginning
            new MoveInitializers(&refMap),
            new SideEffectOrdering(&refMap, &typeMap, skipSideEffectOrdering),
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
            new SetHeaders(&refMap, &typeMap),
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
    if (options.listFrontendPasses) {
        for (auto it : frontendPasses) {
            if (it != nullptr) {
                *outStream << it->name() << '\n';
            }
        }
        return nullptr;
    }

    PassManager passes(frontendPasses);

    if (options.excludeFrontendPasses) {
        passes.removePasses(options.passesToExcludeFrontend);
    }

    passes.setName("FrontEnd");
    passes.setStopOnError(true);
    passes.addDebugHooks(hooks, true);
    const IR::P4Program *result = program->apply(passes);
    return result;
}

}  // namespace P4
