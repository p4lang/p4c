#include "midend.h"
#include "midend/actionsInlining.h"
#include "midend/inlining.h"
#include "midend/moveDeclarations.h"
#include "midend/uniqueNames.h"
#include "midend/removeReturns.h"
#include "midend/moveConstructors.h"
#include "midend/actionSynthesis.h"
#include "frontends/common/typeMap.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/toP4/toP4.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/unusedDeclarations.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4/strengthReduction.h"

namespace V12Test {

P4::BlockMap* MidEnd::process(CompilerOptions& options, const IR::P4Program* program) {
    bool isv1 = options.langVersion == CompilerOptions::FrontendVersion::P4v1;
    auto evaluator0 = new P4::EvaluatorPass(isv1);
    P4::ReferenceMap refMap;

    // TODO: duplicate actions that are used by multiple tables
    // TODO: duplicate global actions into the controls that are using them
    // TODO: remove table parameters if possible
    // TODO: remove action parameters if possible
    // TODO: remove expressions in table key
    
    PassManager simplify = {
        // Give each local declaration a unique internal name
        new P4::UniqueNames(isv1),
        // Move all local declarations to the beginning
        // TODO: this is not sufficient for parser states.
        // Local headers have to be "uninitialized" on entry.
        new P4::MoveDeclarations(),
        new P4::ResolveReferences(&refMap, isv1),
        new P4::RemoveReturns(&refMap, true),
        // Move some constructor calls into temporaries
        new P4::MoveConstructors(isv1),
        new P4::ResolveReferences(&refMap, isv1),
        new P4::RemoveUnusedDeclarations(&refMap),
        evaluator0,
    };

    simplify.setName("Simplify");
    simplify.setStopOnError(true);
    for (auto h : hooks)
        simplify.addDebugHook(h);
    program = program->apply(simplify);
    if (::errorCount() > 0)
        return nullptr;
    auto blockMap = evaluator0->getBlockMap();
    if (blockMap->getMain() == nullptr)
        // nothing further to do
        return nullptr;

    P4::TypeMap typeMap;
    P4::InlineWorkList toInline;
    P4::ActionsInlineList actionsToInline;

    auto inliner = new P4::GeneralInliner();
    auto actInl = new P4::DiscoverActionsInlining(&actionsToInline, &refMap, &typeMap);
    actInl->allowDirectActionCalls = true;

    PassManager midEnd = {
        new P4::DiscoverInlining(&toInline, blockMap),
        new P4::InlineDriver(&toInline, inliner, isv1),
        new PassRepeated {
            // remove useless callees
            new P4::ResolveReferences(&refMap, isv1),
            new P4::RemoveUnusedDeclarations(&refMap),
        },
        new P4::ResolveReferences(&refMap, isv1),
        new P4::TypeChecker(&refMap, &typeMap, true, true),
        actInl,
        new P4::InlineActionsDriver(&actionsToInline, new P4::ActionsInliner(), isv1),
        new PassRepeated {
            new P4::ResolveReferences(&refMap, isv1),
            new P4::RemoveUnusedDeclarations(&refMap),
        },
        // TODO: inlining introduces lots of copies,
        // so perhaps a copy-propagation step would be useful
        new P4::SimplifyControlFlow(),
        new P4::ResolveReferences(&refMap, isv1),
        new P4::RemoveReturns(&refMap, false),  // remove exits
        new P4::ResolveReferences(&refMap, isv1),
        new P4::TypeChecker(&refMap, &typeMap),
        new P4::ConstantFolding(&refMap, &typeMap),
        new P4::StrengthReduction(),
        new P4::MoveDeclarations(),  // more may have been introduced
        new P4::SimplifyControlFlow(),
        // Create actions for statements that can't be done in control blocks.
        new P4::ResolveReferences(&refMap, isv1),
        new P4::TypeChecker(&refMap, &typeMap),
        new P4::SynthesizeActions(&refMap, &typeMap),
        // Move all stand-alone action invocations to custom tables
        new P4::ResolveReferences(&refMap, isv1),
        new P4::TypeChecker(&refMap, &typeMap),
        new P4::MoveActionsToTables(&refMap, &typeMap),
        evaluator0
    };
    midEnd.setName("MidEnd");
    midEnd.setStopOnError(true);
    for (auto h : hooks)
        midEnd.addDebugHook(h);
    program = program->apply(midEnd);
    if (::errorCount() > 0)
        return nullptr;

    std::ostream *midendStream = options.dumpStream("-midend");
    P4::ToP4 top4(midendStream, options.file);
    program->apply(top4);

    blockMap = evaluator0->getBlockMap();
    return blockMap;
}

}  // namespace V12Test
