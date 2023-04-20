#include "backends/p4tools/modules/testgen/core/program_info.h"

#include "backends/p4tools/common/compiler/reachability.h"
#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/common/lib/variables.h"
#include "ir/id.h"
#include "ir/irutils.h"
#include "lib/cstring.h"
#include "lib/enumerator.h"
#include "lib/exceptions.h"
#include "midend/coverage.h"

#include "backends/p4tools/modules/testgen/lib/concolic.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/options.h"

namespace P4Tools::P4Testgen {

ProgramInfo::ProgramInfo(const IR::P4Program *program) : concolicMethodImpls({}), program(program) {
    concolicMethodImpls.add(*Concolic::getCoreConcolicMethodImpls());
    if (TestgenOptions::get().dcg || !TestgenOptions::get().pattern.empty()) {
        // Create DCG.
        auto *currentDCG = new NodesCallGraph("NodesCallGraph");
        P4ProgramDCGCreator dcgCreator(currentDCG);
        program->apply(dcgCreator);
        dcg = currentDCG;
    }
    program->apply(P4::Coverage::CollectStatements(allStatements));
}

/* =============================================================================================
 *  Namespaces and declarations
 * ============================================================================================= */

const IR::IDeclaration *ProgramInfo::findProgramDecl(const IR::IGeneralNamespace *ns,
                                                     const IR::Path *path) {
    auto name = path->name.name;
    const auto *decls = ns->getDeclsByName(name)->toVector();
    if (!decls->empty()) {
        // TODO: Figure out what to do with multiple results. Maybe return all of them and
        // let the caller sort it out?
        BUG_CHECK(decls->size() == 1, "Handling of overloaded names not implemented");
        return decls->at(0);
    }
    BUG("Variable %1% not found in the available namespaces.", path);
}

const IR::IDeclaration *ProgramInfo::findProgramDecl(const IR::IGeneralNamespace *ns,
                                                     const IR::PathExpression *pathExpr) {
    return findProgramDecl(ns, pathExpr->path);
}

const IR::Type_Declaration *ProgramInfo::resolveProgramType(const IR::IGeneralNamespace *ns,
                                                            const IR::Type_Name *type) {
    const auto *path = type->path;
    const auto *decl = findProgramDecl(ns, path)->to<IR::Type_Declaration>();
    BUG_CHECK(decl, "Not a type: %1%", path);
    return decl;
}

const IR::Expression *ProgramInfo::createTargetUninitialized(const IR::Type *type,
                                                             bool forceTaint) const {
    if (forceTaint) {
        return ToolsVariables::getTaintExpression(type);
    }
    return IR::getDefaultValue(type);
}

/* =============================================================================================
 *  Getters
 * ============================================================================================= */

const P4::Coverage::CoverageSet &ProgramInfo::getAllStatements() const { return allStatements; }

const ConcolicMethodImpls *ProgramInfo::getConcolicMethodImpls() const {
    return &concolicMethodImpls;
}

const std::vector<Continuation::Command> *ProgramInfo::getPipelineSequence() const {
    return &pipelineSequence;
}

std::optional<const IR::Expression *> ProgramInfo::getTargetConstraints() const {
    return targetConstraints;
}

void ProgramInfo::produceCopyInOutCall(const IR::Parameter *param, size_t paramIdx,
                                       const ArchSpec::ArchMember *archMember,
                                       std::vector<Continuation::Command> *copyIns,
                                       std::vector<Continuation::Command> *copyOuts) const {
    const auto *paramType = param->type;
    // We need to resolve type names.
    if (const auto *tn = paramType->to<IR::Type_Name>()) {
        paramType = resolveProgramType(program, tn);
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
