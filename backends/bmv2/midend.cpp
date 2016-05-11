#include "midend.h"
#include "lower.h"
#include "inlining.h"
#include "midend/actionsInlining.h"
#include "midend/uniqueNames.h"
#include "midend/moveDeclarations.h"
#include "midend/removeReturns.h"
#include "midend/moveConstructors.h"
#include "midend/actionSynthesis.h"
#include "midend/local_copyprop.h"
#include "frontends/p4/strengthReduction.h"
#include "frontends/common/typeMap.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/toP4/toP4.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/unusedDeclarations.h"
#include "frontends/common/constantFolding.h"

namespace BMV2 {

const IR::P4Program* MidEnd::processV1(CompilerOptions&, const IR::P4Program* program) {
    bool isv1 = true;
    auto evaluator = new P4::Evaluator(&refMap, &typeMap);

    // Inlining is simpler for P4 v1.0/1.1 programs, so we have a
    // specialized code path, which also generates slighly nicer
    // human-readable results.
    P4::InlineWorkList controlsToInline;
    P4::ActionsInlineList actionsToInline;

    PassManager midend = {
        new P4::TypeChecking(&refMap, &typeMap, isv1),
        evaluator,
        new P4::DiscoverInlining(&controlsToInline, &refMap, &typeMap, evaluator),
        new P4::InlineDriver(&controlsToInline, new SimpleControlsInliner(&refMap), isv1),
        new P4::RemoveAllUnusedDeclarations(&refMap, isv1),
        new P4::TypeChecking(&refMap, &typeMap, isv1),
        new P4::DiscoverActionsInlining(&actionsToInline, &refMap, &typeMap),
        new P4::InlineActionsDriver(&actionsToInline, new SimpleActionsInliner(&refMap), isv1),
        new P4::RemoveAllUnusedDeclarations(&refMap, isv1),
    };
    midend.setName("Mid end");
    midend.addDebugHooks(hooks);
    program = program->apply(midend);
    if (::errorCount() > 0)
        return nullptr;
    return program;
}

const IR::P4Program* MidEnd::processV1_2(CompilerOptions&, const IR::P4Program* program) {
    bool isv1 = false;
    auto evaluator = new P4::Evaluator(&refMap, &typeMap);

    PassManager simplify = {
        // Proper semantics for uninitialzed local variables in parser states:
        // headers must be invalidated
        new P4::TypeChecking(&refMap, &typeMap, isv1),
        new P4::ResetHeaders(&typeMap),
        // Give each local declaration a unique internal name
        new P4::UniqueNames(isv1),
        // Move all local declarations to the beginning
        new P4::MoveDeclarations(),
        new P4::ResolveReferences(&refMap, isv1),
        new P4::RemoveReturns(&refMap),
        // Move some constructor calls into temporaries
        new P4::MoveConstructors(isv1),
        new P4::RemoveAllUnusedDeclarations(&refMap, isv1),
        new P4::TypeChecking(&refMap, &typeMap, isv1),
        evaluator,
    };

    simplify.setName("Simplify");
    simplify.addDebugHooks(hooks);
    program = program->apply(simplify);
    if (::errorCount() > 0)
        return nullptr;
    auto toplevel = evaluator->getToplevelBlock();
    if (toplevel->getMain() == nullptr)
        // nothing further to do
        return nullptr;

    P4::InlineWorkList toInline;
    P4::ActionsInlineList actionsToInline;
    PassManager midEnd = {
        new P4::DiscoverInlining(&toInline, &refMap, &typeMap, evaluator),
        new P4::InlineDriver(&toInline, new P4::GeneralInliner(), isv1),
        new P4::RemoveAllUnusedDeclarations(&refMap, isv1),
        new P4::TypeChecking(&refMap, &typeMap, isv1),
        new P4::DiscoverActionsInlining(&actionsToInline, &refMap, &typeMap),
        new P4::InlineActionsDriver(&actionsToInline, new P4::ActionsInliner(), isv1),
        new P4::RemoveAllUnusedDeclarations(&refMap, isv1),
        new P4::TypeChecking(&refMap, &typeMap, isv1),
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        new P4::TypeChecking(&refMap, &typeMap, isv1),
        new P4::ConstantFolding(&refMap, &typeMap),
        new P4::StrengthReduction(),
        new P4::TypeChecking(&refMap, &typeMap, isv1, true),
        new P4::LocalCopyPropagation(),
        new P4::MoveDeclarations(),
        // Create actions for statements that can't be done in control blocks.
        new P4::TypeChecking(&refMap, &typeMap, isv1),
        new P4::SynthesizeActions(&refMap, &typeMap),
        // Move all stand-alone actions to custom tables
        new P4::TypeChecking(&refMap, &typeMap, isv1),
        new P4::MoveActionsToTables(&refMap, &typeMap),
    };

    midEnd.setName("MidEnd");
    midEnd.addDebugHooks(hooks);
    program = program->apply(midEnd);
    if (::errorCount() > 0)
        return nullptr;
    return program;
}


IR::ToplevelBlock* MidEnd::process(CompilerOptions& options, const IR::P4Program* program) {
    bool isv1 = options.langVersion == CompilerOptions::FrontendVersion::P4v1;
    if (isv1)
        program = processV1(options, program);
    else
        program = processV1_2(options, program);
    if (program == nullptr)
        return nullptr;

    // BMv2-specific passes
    auto evaluator = new P4::Evaluator(&refMap, &typeMap);
    PassManager backend = {
        new P4::TypeChecking(&refMap, &typeMap, isv1),
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        new P4::TypeChecking(&refMap, &typeMap, isv1),
        new RemoveLeftSlices(&typeMap),
        new P4::TypeChecking(&refMap, &typeMap, isv1),
        new LowerExpressions(&typeMap),
        new P4::TypeChecking(&refMap, &typeMap, isv1),
        new P4::ConstantFolding(&refMap, &typeMap),
        new P4::TypeChecking(&refMap, &typeMap, isv1),
        evaluator
    };

    backend.setName("Backend");
    backend.addDebugHooks(hooks);
    program = program->apply(backend);
    if (::errorCount() > 0)
        return nullptr;

    auto toplevel = evaluator->getToplevelBlock();
    return toplevel;
}

}  // namespace BMV2
