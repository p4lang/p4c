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

#include "frontend.h"

#include <fstream>
#include <iostream>

#include "../common/options.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/fromv1.0/v1model.h"
#include "frontends/p4/typeChecking/bindVariables.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "lib/nullstream.h"
#include "lib/path.h"
// Passes
#include "actionsInlining.h"
#include "checkConstants.h"
#include "checkCoreMethods.h"
#include "checkNamedArgs.h"
#include "createBuiltins.h"
#include "defaultArguments.h"
#include "defaultValues.h"
#include "deprecated.h"
#include "directCalls.h"
#include "dontcareArgs.h"
#include "entryPriorities.h"
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
#include "reassociation.h"
#include "redundantParsers.h"
#include "removeParameters.h"
#include "removeReturns.h"
#include "resetHeaders.h"
#include "setHeaders.h"
#include "sideEffects.h"
#include "simplify.h"
#include "simplifyDefUse.h"
#include "simplifyParsers.h"
#include "simplifySwitch.h"
#include "specialize.h"
#include "specializeGenericFunctions.h"
#include "specializeGenericTypes.h"
#include "staticAssert.h"
#include "strengthReduction.h"
#include "structInitializers.h"
#include "switchAddDefault.h"
#include "tableKeyNames.h"
#include "toP4/toP4.h"
#include "typeChecking/typeChecker.h"
#include "uniqueNames.h"
#include "unusedDeclarations.h"
#include "uselessCasts.h"
#include "validateMatchAnnotations.h"
#include "validateParsedProgram.h"
#include "validateValueSets.h"

namespace P4 {

namespace {

/* Base class for inspectors that do not really visit the program. */
class NoVisit : public Inspector {
    // prune visit
    bool preorder(const IR::P4Program *) override { return false; }
};

/**
This pass outputs the program as a P4 source file.
*/
class PrettyPrint : public Inspector {
    /// output file
    cstring ppfile;
    /// The file that is being compiled.  This used
    cstring inputfile;

 public:
    explicit PrettyPrint(const CompilerOptions &options) {
        setName("PrettyPrint");
        ppfile = options.prettyPrintFile;
        inputfile = options.file;
    }
    bool preorder(const IR::P4Program *program) override {
        if (!ppfile.isNullOrEmpty()) {
            Util::PathName path(ppfile);
            std::ostream *ppStream = openFile(path.toString(), true);
            P4::ToP4 top4(ppStream, false, inputfile);
            (void)program->apply(top4);
        }
        return false;  // prune
    }
};

/**
 * This pass is a no-op whose purpose is to mark the end of the
 * front-end, which is useful for debugging. It is implemented as an
 * empty @ref PassManager (instead of a @ref Visitor) for efficiency.
 */
class FrontEndLast : public NoVisit {
 public:
    FrontEndLast() { setName("FrontEndLast"); }
};

/**
 * This pass is a no-op whose purpose is to mark a point in the
 * front-end, used for testing.
 */
class FrontEndDump : public NoVisit {
 public:
    FrontEndDump() { setName("FrontEndDump"); }
};

/** Changes the value of strictStruct in the typeMap */
class SetStrictStruct : public NoVisit {
    TypeMap *typeMap;
    bool strictStruct;

 public:
    SetStrictStruct(TypeMap *typeMap, bool strict) : typeMap(typeMap), strictStruct(strict) {}
    Visitor::profile_t init_apply(const IR::Node *node) override {
        typeMap->setStrictStruct(strictStruct);
        return Inspector::init_apply(node);
    }
};

}  // namespace

// TODO: remove skipSideEffectOrdering flag
const IR::P4Program *FrontEnd::run(const CompilerOptions &options, const IR::P4Program *program,
                                   bool skipSideEffectOrdering, std::ostream *outStream) {
    if (program == nullptr && options.listFrontendPasses == 0) return nullptr;

    bool isv1 = options.isv1();
    ReferenceMap refMap;
    TypeMap typeMap;
    refMap.setIsV1(isv1);

    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);
    PassManager passes({
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
        new SetStrictStruct(&typeMap, true),  // Next pass uses strict struct checking
        new TypeInference(&refMap, &typeMap, false, false),  // insert casts, dont' check arrays
        new SetStrictStruct(&typeMap, false),
        new ValidateMatchAnnotations(&typeMap),
        new ValidateValueSets(),
        new DefaultValues(&refMap, &typeMap),
        new BindTypeVariables(&refMap, &typeMap),
        new EntryPriorities(&refMap),
        new PassRepeated(
            {new SpecializeGenericTypes(&refMap, &typeMap),
             new DefaultArguments(&refMap, &typeMap),  // add default argument values to parameters
             new ResolveReferences(&refMap),
             new SetStrictStruct(&typeMap, true),          // Next pass uses strict struct checking
             new TypeInference(&refMap, &typeMap, false),  // more casts may be needed
             new SetStrictStruct(&typeMap, false),
             new SpecializeGenericFunctions(&refMap, &typeMap)}),
        new CheckCoreMethods(&refMap, &typeMap),
        new StaticAssert(&refMap, &typeMap),
        new RemoveParserIfs(&refMap, &typeMap),
        new StructInitializers(&refMap, &typeMap),
        new TableKeyNames(&refMap, &typeMap),
        new PassRepeated({new ConstantFolding(&refMap, &typeMap),
                          new StrengthReduction(&refMap, &typeMap), new Reassociation(),
                          new UselessCasts(&refMap, &typeMap)}),
        new SimplifyControlFlow(&refMap, &typeMap),
        new SwitchAddDefault,
        new FrontEndDump(),  // used for testing the program at this point
        new RemoveAllUnusedDeclarations(&refMap, true),
        new SimplifyParsers(&refMap),
        new ResetHeaders(&refMap, &typeMap),
        new UniqueNames(&refMap),  // Give each local declaration a unique internal name
        new MoveDeclarations(),    // Move all local declarations to the beginning
        new MoveInitializers(&refMap),
        new SideEffectOrdering(&refMap, &typeMap, skipSideEffectOrdering),
        new SimplifyControlFlow(&refMap, &typeMap),
        new SimplifySwitch(&refMap, &typeMap),
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
        new RemoveRedundantParsers(&refMap, &typeMap),
        new ClearTypeMap(&typeMap),
        evaluator,
        new Inline(&refMap, &typeMap, evaluator, options.optimizeParserInlining),
        new InlineActions(&refMap, &typeMap),
        new LocalizeAllActions(&refMap),
        new UniqueNames(&refMap),
        new UniqueParameters(&refMap, &typeMap),
        // Must be done before inlining functions, to allow
        // function calls used as action arguments to be inlined
        // in the proper place.
        new RemoveActionParameters(&refMap, &typeMap),
        new InlineFunctions(&refMap, &typeMap),
        new SetHeaders(&refMap, &typeMap),
        // Check for constants only after inlining
        new CheckConstants(&refMap, &typeMap),
        new SimplifyControlFlow(&refMap, &typeMap),
        new RemoveParserControlFlow(&refMap, &typeMap),  // more ifs may have been added to parsers
        new UniqueNames(&refMap),                        // needed again after inlining
        new MoveDeclarations(),                          // needed again after inlining
        new SimplifyDefUse(&refMap, &typeMap),
        new RemoveAllUnusedDeclarations(&refMap),
        new SimplifyControlFlow(&refMap, &typeMap),
        new HierarchicalNames(),
        new FrontEndLast(),
    });
    if (options.listFrontendPasses) {
        passes.listPasses(*outStream, "\n");
        *outStream << std::endl;
        return nullptr;
    }

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
