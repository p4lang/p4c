#include "midend.h"
#include "midend/actionsInlining.h"
#include "midend/inlining.h"
#include "midend/uniqueNames.h"
#include "frontends/common/typeMap.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/toP4/toP4.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/unusedDeclarations.h"
#include "frontends/common/constantFolding.h"

namespace V12Test {

P4::BlockMap* MidEnd::process(CompilerOptions& options, const IR::P4Program* program) {
    bool isv1 = options.langVersion == CompilerOptions::FrontendVersion::P4v1;

    std::ostream *inlineStream = options.dumpStream("-inline");
    auto evaluator0 = new P4::EvaluatorPass(isv1);

    program = program->apply(*evaluator0);
    if (::errorCount() > 0)
        return nullptr;
    auto blockMap = evaluator0->getBlockMap();
    if (blockMap->getMain() == nullptr)
        return nullptr;
    
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;

    P4::InlineWorkList toInline;
    P4::ActionsInlineList actionsToInline;

    auto inliner = new P4::GeneralInliner();
    auto find = new P4::DiscoverInlining(&toInline, blockMap);
    auto evaluator1 = new P4::EvaluatorPass(isv1);
    auto actInl = new P4::DiscoverActionsInlining(&actionsToInline, &refMap, &typeMap);
    actInl->allowDirectActionCalls = true;
    
    PassManager midEnd = {
        new P4::UniqueNames(isv1),
        find,
        new P4::InlineDriver(&toInline, inliner, isv1),
        new PassRepeated {
            // remove useless callees
            new P4::ResolveReferences(&refMap, isv1),
            new P4::RemoveUnusedDeclarations(&refMap),
        },
#if 0
        new P4::ResolveReferences(&refMap, isv1),
        new P4::TypeChecker(&refMap, &typeMap, true, true),
        actInl,
        new P4::InlineActionsDriver(&actionsToInline, &refMap, isv1),
        new PassRepeated {
            new P4::ResolveReferences(&refMap, isv1),
            new P4::RemoveUnusedDeclarations(&refMap),
        },
#endif
        new P4::SimplifyControlFlow(),
        //new P4::ToP4(inlineStream, options.file),
        evaluator1
    };
    midEnd.setStopOnError(true);

    (void)program->apply(midEnd);
    if (::errorCount() > 0)
        return nullptr;

    blockMap = evaluator1->getBlockMap();
    return blockMap;
}

}  // namespace V12Test
