#include "backends/p4tools/modules/testgen/core/program_info.h"

#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/common/lib/variables.h"
#include "ir/id.h"
#include "ir/irutils.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "midend/coverage.h"

#include "backends/p4tools/modules/testgen/core/compiler_target.h"
#include "backends/p4tools/modules/testgen/lib/concolic.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"

namespace P4Tools::P4Testgen {

ProgramInfo::ProgramInfo(const TestgenCompilerResult &compilerResult)
    : compilerResult(compilerResult), concolicMethodImpls({}) {
    concolicMethodImpls.add(*Concolic::getCoreConcolicMethodImpls());
}

const IR::Expression *ProgramInfo::createTargetUninitialized(const IR::Type *type,
                                                             bool forceTaint) const {
    if (forceTaint) {
        return ToolsVariables::getTaintExpression(type);
    }
    return IR::getDefaultValue(type, {}, true);
}

/* =============================================================================================
 *  Getters
 * ============================================================================================= */

const P4::Coverage::CoverageSet &ProgramInfo::getCoverableNodes() const {
    return getCompilerResult().getCoverableNodes();
}

const TestgenCompilerResult &ProgramInfo::getCompilerResult() const { return compilerResult.get(); }

const IR::P4Program &ProgramInfo::getP4Program() const { return getCompilerResult().getProgram(); }

const NodesCallGraph &ProgramInfo::getCallGraph() const {
    return getCompilerResult().getCallGraph();
}

const ConcolicMethodImpls *ProgramInfo::getConcolicMethodImpls() const {
    return &concolicMethodImpls;
}

const std::vector<Continuation::Command> *ProgramInfo::getPipelineSequence() const {
    return &pipelineSequence;
}

std::optional<const IR::Expression *> ProgramInfo::getTargetConstraints() const {
    return targetConstraints;
}

cstring ProgramInfo::getCanonicalBlockName(cstring programBlockName) const {
    auto it = blockMap.find(programBlockName);
    if (it != blockMap.end()) {
        return it->second;
    }
    BUG("Unable to find var %s in the canonical block map.", programBlockName);
}

void ProgramInfo::produceCopyInOutCall(const IR::Parameter *param, size_t paramIdx,
                                       const ArchSpec::ArchMember *archMember,
                                       std::vector<Continuation::Command> *copyIns,
                                       std::vector<Continuation::Command> *copyOuts) const {
    const auto *paramType = param->type;
    // We need to resolve type names.
    if (const auto *tn = paramType->to<IR::Type_Name>()) {
        paramType = resolveProgramType(&getP4Program(), tn);
    }
    // Retrieve the identifier of the global architecture map using the parameter
    // index.
    auto archRef = archMember->blockParams.at(paramIdx);
    // If the archRef is a nullptr or empty, we do not have a mapping for this
    // parameter.
    if (archRef.isNullOrEmpty()) {
        return;
    }
    const auto *archPath = new IR::PathExpression(paramType, new IR::Path(archRef));
    const auto *paramRef = new IR::PathExpression(paramType, new IR::Path(param->name));
    const auto *paramDir = new IR::StringLiteral(directionToString(param->direction));
    if (copyIns != nullptr) {
        // This mimicks the copy-in from the architecture environment.
        const auto *copyInCall = new IR::MethodCallStatement(Utils::generateInternalMethodCall(
            "copy_in", {archPath, paramRef, paramDir, new IR::BoolLiteral(false)}));
        copyIns->emplace_back(copyInCall);
    }
    if (copyOuts != nullptr) {
        // This mimicks the copy-out from the architecture environment.
        const auto *copyOutCall = new IR::MethodCallStatement(
            Utils::generateInternalMethodCall("copy_out", {archPath, paramRef, paramDir}));
        copyOuts->emplace_back(copyOutCall);
    }
}

}  // namespace P4Tools::P4Testgen
