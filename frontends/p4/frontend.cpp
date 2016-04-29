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

    bool p4v10 = options.isv1();
    
    Util::PathName path(options.prettyPrintFile);
    std::ostream *ppStream = openFile(path.toString(), true);
    std::ostream *midStream = options.dumpStream("-fe");
    std::ostream *endStream = options.dumpStream("-last");

    P4::ReferenceMap  refMap;  // This is reused many times, since every analysis clear it
    P4::TypeMap       typeMap1, typeMap2, typeMap3;

    PassManager passes = {
        new P4::ToP4(ppStream, options.file),
        // Simple checks on parsed program
        new P4::ValidateParsedProgram(p4v10),
        // Synthesize some built-in constructs
        new P4::CreateBuiltins(),
        // First pass of constant folding, before types are known
        // May be needed to compute types
        new P4::ResolveReferences(&refMap, p4v10, true),
        new P4::ConstantFolding(&refMap, nullptr),
        new P4::ResolveReferences(&refMap, p4v10),
        // Type checking and type inference.  Also inserts
        // explicit casts where implicit casts exist.
        new P4::TypeChecker(&refMap, &typeMap1, true, false),
        // Another round of constant folding, using type information.
        new P4::SimplifyControlFlow(),
        new P4::ResolveReferences(&refMap, p4v10),
        new P4::ConstantFolding(&refMap, &typeMap1),
        new P4::StrengthReduction(),

        // Print program in the middle
        new P4::ToP4(midStream, options.file),
        new PassRepeated{
            // Remove unused declarations.
            new P4::ResolveReferences(&refMap, p4v10),
            new P4::RemoveUnusedDeclarations(&refMap),
        },
        // Print the program before the end.
        new P4::ToP4(endStream, options.file),
    };

    passes.setName("FrontEnd");
    passes.setStopOnError(true);
    for (auto h : hooks)
        passes.addDebugHook(h);
    const IR::P4Program* result = v12_program->apply(passes);
    return result;
}
