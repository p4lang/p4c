#include "midend.h"
#include "lower.h"
#include "inlining.h"
#include "midend/actionsInlining.h"
#include "frontends/common/typeMap.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/toP4/toP4.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/unusedDeclarations.h"
#include "frontends/common/constantFolding.h"

namespace BMV2 {

P4::BlockMap* MidEnd::process(CompilerOptions& options, const IR::P4Program* program) {
    bool isv1 = options.langVersion == CompilerOptions::FrontendVersion::P4v1;

    std::ostream *inlineStream = options.dumpStream("-inline");
    auto evaluator0 = new P4::EvaluatorPass(isv1);

    (void)program->apply(*evaluator0);
    if (::errorCount() > 0)
        return nullptr;

    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;

    if (isv1) {
        // Inlining is simpler for P4 v1.0/1.1 programs,
        // so we have a specialized code path, which also
        // generates slighly nicer human-readable results.
        // Maybe this path will be removed when we have a
        // all fully general P4 v1.2 lowering passes
        // implemented.
        P4::InlineWorkList controlsToInline;
        P4::ActionsInlineList actionsToInline;

        auto find = new P4::DiscoverInlining(&controlsToInline, evaluator0->getBlockMap());
        find->allowParsers = false;
        // this last assignment is not necessary, since P4 v1.0 programs have no parser calls
        
        PassManager inlinerPasses = {
            find,
            new P4::InlineDriver(&controlsToInline, new SimpleControlsInliner(&refMap), isv1),
            new PassRepeated {
                // remove useless callees
                new P4::ResolveReferences(&refMap, isv1),
                new P4::RemoveUnusedDeclarations(&refMap),
            },
            new P4::ResolveReferences(&refMap, isv1),
            new P4::TypeChecker(&refMap, &typeMap, true, true),
            new P4::DiscoverActionsInlining(&actionsToInline, &refMap, &typeMap),
            new P4::InlineActionsDriver(&actionsToInline, new SimpleActionsInliner(&refMap), isv1),
            new PassRepeated {
                new P4::ResolveReferences(&refMap, isv1),
                new P4::RemoveUnusedDeclarations(&refMap),
            }
        };
        inlinerPasses.setStopOnError(true);
        program = program->apply(inlinerPasses);
        if (::errorCount() > 0)
            return nullptr;
    }

    // TODO: add separate inlining passes for v1.2 programs.
    // Today such programs that require inlining will fail to compile.

    auto evaluator1 = new P4::EvaluatorPass(isv1);
    PassManager midEnd = {
        new P4::SimplifyControlFlow(),
        new P4::ToP4(inlineStream, options.file),
        new P4::ResolveReferences(&refMap, isv1),
        new P4::TypeChecker(&refMap, &typeMap, true, true),
        new RemoveLeftSlices(&typeMap),
        new P4::ResolveReferences(&refMap, isv1),
        new P4::TypeChecker(&refMap, &typeMap, true, true),
        new LowerExpressions(&typeMap),
        new P4::ResolveReferences(&refMap, isv1),
        new P4::TypeChecker(&refMap, &typeMap, true, true),
        new P4::ConstantFolding(&refMap, &typeMap),
        evaluator1
    };

    midEnd.setStopOnError(true);
    (void)program->apply(midEnd);
    if (::errorCount() > 0)
        return nullptr;

    auto blockMap = evaluator1->getBlockMap();
    return blockMap;
}

}  // namespace BMV2
