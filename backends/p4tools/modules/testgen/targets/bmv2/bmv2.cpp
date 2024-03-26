#include "backends/p4tools/modules/testgen/targets/bmv2/bmv2.h"

#include <optional>
#include <string>
#include <utility>

#include "backends/bmv2/common/annotations.h"
#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/compiler/midend.h"
#include "frontends/common/options.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "midend/coverage.h"

#include "backends/p4tools/modules/testgen/core/compiler_target.h"
#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/map_direct_externs.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/p4_asserts_parser.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/p4_refers_to_parser.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/p4runtime_translation.h"

namespace P4Tools::P4Testgen::Bmv2 {

BMv2V1ModelCompilerResult::BMv2V1ModelCompilerResult(TestgenCompilerResult compilerResult,
                                                     P4::P4RuntimeAPI p4runtimeApi,
                                                     DirectExternMap directExternMap,
                                                     ConstraintsVector p4ConstraintsRestrictions)
    : TestgenCompilerResult(std::move(compilerResult)),
      p4runtimeApi(p4runtimeApi),
      directExternMap(std::move(directExternMap)),
      p4ConstraintsRestrictions(std::move(p4ConstraintsRestrictions)) {}

const P4::P4RuntimeAPI &BMv2V1ModelCompilerResult::getP4RuntimeApi() const { return p4runtimeApi; }

ConstraintsVector BMv2V1ModelCompilerResult::getP4ConstraintsRestrictions() const {
    return p4ConstraintsRestrictions;
}

const DirectExternMap &BMv2V1ModelCompilerResult::getDirectExternMap() const {
    return directExternMap;
}

Bmv2V1ModelCompilerTarget::Bmv2V1ModelCompilerTarget() : TestgenCompilerTarget("bmv2", "v1model") {}

void Bmv2V1ModelCompilerTarget::make() {
    static Bmv2V1ModelCompilerTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new Bmv2V1ModelCompilerTarget();
    }
}

CompilerResultOrError Bmv2V1ModelCompilerTarget::runCompilerImpl(
    const IR::P4Program *program) const {
    program = runFrontend(program);
    if (program == nullptr) {
        return std::nullopt;
    }

    /// After the front end, get the P4Runtime API for the V1model architecture.
    auto p4runtimeApi = P4::P4RuntimeSerializer::get()->generateP4Runtime(program, "v1model");

    if (::errorCount() > 0) {
        return std::nullopt;
    }

    program = runMidEnd(program);
    if (program == nullptr) {
        return std::nullopt;
    }

    // Create DCG.
    NodesCallGraph *dcg = nullptr;
    if (TestgenOptions::get().dcg || !TestgenOptions::get().pattern.empty()) {
        dcg = new NodesCallGraph("NodesCallGraph");
        P4ProgramDCGCreator dcgCreator(dcg);
        program->apply(dcgCreator);
    }
    if (::errorCount() > 0) {
        return std::nullopt;
    }
    /// Collect coverage information about the program.
    auto coverage = P4::Coverage::CollectNodes(TestgenOptions::get().coverageOptions);
    program->apply(coverage);
    if (::errorCount() > 0) {
        return std::nullopt;
    }

    // Parses any @refers_to annotations and converts them into a vector of restrictions.
    auto refersToParser = RefersToParser();
    program->apply(refersToParser);
    if (::errorCount() > 0) {
        return std::nullopt;
    }
    ConstraintsVector p4ConstraintsRestrictions = refersToParser.getRestrictionsVector();

    // Defines all "entry_restriction" and then converts restrictions from string to IR
    // expressions, and stores them in p4ConstraintsRestrictions to move targetConstraints
    // further.
    program->apply(AssertsParser(p4ConstraintsRestrictions));
    if (::errorCount() > 0) {
        return std::nullopt;
    }
    // Try to map all instances of direct externs to the table they are attached to.
    // Save the map in @var directExternMap.
    auto directExternMapper = MapDirectExterns();
    program->apply(directExternMapper);
    if (::errorCount() > 0) {
        return std::nullopt;
    }

    return {*new BMv2V1ModelCompilerResult{
        TestgenCompilerResult(CompilerResult(*program), coverage.getCoverableNodes(), dcg),
        p4runtimeApi, directExternMapper.getDirectExternMap(), p4ConstraintsRestrictions}};
}

MidEnd Bmv2V1ModelCompilerTarget::mkMidEnd(const CompilerOptions &options) const {
    MidEnd midEnd(options);
    auto *refMap = midEnd.getRefMap();
    auto *typeMap = midEnd.getTypeMap();
    midEnd.addPasses({
        // Parse BMv2-specific annotations.
        new BMV2::ParseAnnotations(),
        new P4::TypeChecking(refMap, typeMap, true),
        new PropagateP4RuntimeTranslation(*typeMap),
    });
    midEnd.addDefaultPasses();

    return midEnd;
}

}  // namespace P4Tools::P4Testgen::Bmv2
