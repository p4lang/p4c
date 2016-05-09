#include <iostream>
#include <fstream>

#include "ir/ir.h"
#include "../common/options.h"
#include "lib/nullstream.h"
#include "lib/path.h"
#include "frontend.h"

#include "../common/typeMap.h"
#include "../common/resolveReferences/resolveReferences.h"
// Passes
#include "toP4/toP4.h"
#include "validateParsedProgram.h"
#include "createBuiltins.h"
#include "../common/constantFolding.h"
#include "unusedDeclarations.h"
#include "typeChecking/typeChecker.h"
#include "evaluator/evaluator.h"
#include "strengthReduction.h"
#include "simplify.h"

const IR::P4Program*
FrontEnd::run(const CompilerOptions &options, const IR::P4Program* v12_program) {
    if (v12_program == nullptr)
        return nullptr;

    bool isv1 = options.isv1();

    Util::PathName path(options.prettyPrintFile);
    std::ostream *ppStream = openFile(path.toString(), true);

    P4::ReferenceMap  refMap;  // This is reused many times, since every analysis clear it
    P4::TypeMap       typeMap;

    PassManager passes = {
        new P4::ToP4(ppStream, false, options.file),
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
        // Another round of constant folding, using type information.
        new P4::ResolveReferences(&refMap, isv1),
        new P4::ConstantFolding(&refMap, &typeMap),
        new P4::StrengthReduction(),
        new P4::TypeChecking(&refMap, &typeMap, isv1),
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        new P4::RemoveAllUnusedDeclarations(&refMap, isv1),
    };

    passes.setName("FrontEnd");
    passes.setStopOnError(true);
    passes.addDebugHooks(hooks);
    const IR::P4Program* result = v12_program->apply(passes);
    return result;
}
