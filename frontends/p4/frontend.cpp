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

#include <iostream>

#include "../common/options.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/bindVariables.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "lib/nullstream.h"
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
#include "duplicateActionControlPlaneNameCheck.h"
#include "entryPriorities.h"
#include "evaluator/evaluator.h"
#include "frontends/common/constantFolding.h"
#include "functionsInlining.h"
#include "getV1ModelVersion.h"
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
#include "validateStringAnnotations.h"
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
    std::filesystem::path ppfile;
    /// The file that is being compiled.
    std::filesystem::path inputfile;

 public:
    explicit PrettyPrint(const CompilerOptions &options) {
        setName("PrettyPrint");
        ppfile = options.prettyPrintFile;
        inputfile = options.file;
    }
    bool preorder(const IR::P4Program *program) override {
        if (!ppfile.empty()) {
            std::ostream *ppStream = openFile(ppfile, true);
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

const IR::P4Program *FrontEnd::run(const CompilerOptions &options, const IR::P4Program *program,
                                   std::ostream *outStream) {
    if (program == nullptr && options.listFrontendPasses == 0) return nullptr;

    TypeMap typeMap;

    ParseAnnotations *parseAnnotations = policy->getParseAnnotations();
    if (!parseAnnotations) parseAnnotations = new ParseAnnotations();

    ConstantFoldingPolicy *constantFoldingPolicy = policy->getConstantFoldingPolicy();

    PassManager passes({
        new P4V1::GetV1ModelVersion,
        // Parse annotations
        new ParseAnnotationBodies(parseAnnotations, &typeMap),
        new PrettyPrint(options),
        // Simple checks on parsed program
        new ValidateParsedProgram(),
        // Synthesize some built-in constructs
        new CreateBuiltins(),
        new CheckShadowing(),
        // First pass of constant folding, before types are known --
        // may be needed to compute types.
        new ConstantFolding(constantFoldingPolicy),
        // Validate @name/@deprecated/@noWarn. Should run after constant folding.
        new ValidateStringAnnotations(),
        // Desugars direct parser and control applications
        // into instantiations followed by application
        new InstantiateDirectCalls(),
        new Deprecated(),
        new CheckNamedArgs(),
        // Type checking and type inference.  Also inserts
        // explicit casts where implicit casts exist.
        new SetStrictStruct(&typeMap, true),        // Next pass uses strict struct checking
        new TypeInference(&typeMap, false, false),  // insert casts, don't check arrays
        new SetStrictStruct(&typeMap, false),
        new ValidateMatchAnnotations(&typeMap),
        new ValidateValueSets(),
        new DefaultValues(&typeMap),
        new BindTypeVariables(&typeMap),
        new EntryPriorities(),
        new PassRepeated({
            new SpecializeGenericTypes(&typeMap),
            new DefaultArguments(&typeMap),       // add default argument values to parameters
            new SetStrictStruct(&typeMap, true),  // Next pass uses strict struct checking
            new TypeInference(&typeMap, false),   // more casts may be needed
            new SetStrictStruct(&typeMap, false),
            new SpecializeGenericFunctions(&typeMap),
        }),
        new CheckCoreMethods(&typeMap),
        new StaticAssert(&typeMap),
        new RemoveParserIfs(&typeMap),
        new StructInitializers(&typeMap),
        new TableKeyNames(&typeMap),
        new PassRepeated({
            new ConstantFolding(&typeMap, constantFoldingPolicy),
            new StrengthReduction(&typeMap, policy->enableSubConstToAddTransform()),
            new Reassociation(),
            new UselessCasts(&typeMap),
        }),
        new SimplifyControlFlow(&typeMap),
        new SwitchAddDefault,
        new FrontEndDump(),  // used for testing the program at this point
        new RemoveAllUnusedDeclarations(*policy, true),
        new SimplifyParsers(),
        new ResetHeaders(&typeMap),
        new UniqueNames(),       // Give each local declaration a unique internal name
        new MoveDeclarations(),  // Move all local declarations to the beginning
        new MoveInitializers(),
        new SideEffectOrdering(&typeMap, policy->skipSideEffectOrdering()),
        new SimplifyControlFlow(&typeMap),
        new SimplifySwitch(&typeMap),
        new MoveDeclarations(),  // Move all local declarations to the beginning
        new SimplifyDefUse(&typeMap),
        new UniqueParameters(&typeMap),
        new SimplifyControlFlow(&typeMap),
        new SpecializeAll(&typeMap, policy),
        new RemoveParserControlFlow(&typeMap),
        new RemoveReturns(),
        new RemoveDontcareArgs(&typeMap),
        new MoveConstructors(),
        new RemoveAllUnusedDeclarations(*policy),
        new RemoveRedundantParsers(&typeMap, *policy),
        new ClearTypeMap(&typeMap),
        new EvaluatorPass(&typeMap),
    });
    if (policy->optimize(options)) {
        passes.addPasses({
            new Inline(&typeMap, *policy, options.optimizeParserInlining),
        });
    }
    if (options.controlPlaneAPIGenEnabled()) {
        passes.addPasses({
            new DuplicateActionControlPlaneNameCheck(),
        });
    }
    if (policy->optimize(options)) {
        passes.addPasses({
            new InlineActions(&typeMap, *policy),
            new LocalizeAllActions(*policy),
            new UniqueNames(),
            new UniqueParameters(&typeMap),
            // Must be done before inlining functions, to allow
            // function calls used as action arguments to be inlined
            // in the proper place.
            new RemoveActionParameters(&typeMap),
            new InlineFunctions(&typeMap, *policy),
            new SetHeaders(&typeMap),
            // Check for constants only after inlining
            new CheckConstants(&typeMap),
            new ConstantFolding(&typeMap, constantFoldingPolicy),
            new SimplifyControlFlow(&typeMap),
            // more ifs may have been added to parsers
            new RemoveParserControlFlow(&typeMap),
            new UniqueNames(),       // needed again after inlining
            new MoveDeclarations(),  // needed again after inlining
            new SimplifyDefUse(&typeMap),
            new RemoveAllUnusedDeclarations(*policy),
            new SimplifyControlFlow(&typeMap),
        });
    }
    passes.addPasses({
        // Check for shadowing after all inlining passes. We disable this
        // check during inlining since it significantly slows compilation.
        new CheckShadowing(),
        new HierarchicalNames(),
        new FrontEndLast(),
    });
    if (options.listFrontendPasses) {
        passes.listPasses(*outStream, cstring::newline);
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
