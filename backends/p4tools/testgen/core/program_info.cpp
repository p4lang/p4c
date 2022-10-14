#include "backends/p4tools/testgen/core/program_info.h"

#include "backends/p4tools/common/lib/ir.h"
#include "lib/exceptions.h"

#include "backends/p4tools/testgen/options.h"

namespace P4Tools {

namespace P4Testgen {

ProgramInfo::ProgramInfo(const IR::P4Program* program)
    : globalNameSpaceContext(NamespaceContext::Empty->push(program)),
      concolicMethodImpls({}),
      program(program) {
    concolicMethodImpls.add(*Concolic::getCoreConcolicMethodImpls());
    if (TestgenOptions::get().dcg) {
        // Create DCG.
        P4::ReferenceMap refMap;
        P4::TypeMap typeMap;
        auto* currentDCG = new NodesCallGraph("NodesCallGraph");
        P4ProgramDCGCreator dcgCreator(&refMap, &typeMap, currentDCG);
        program->apply(dcgCreator);
        dcg = currentDCG;
    }
    program->apply(Coverage::CollectStatements(allStatements));
}

/* =============================================================================================
 *  Namespaces and declarations
 * ============================================================================================= */

const IR::IDeclaration* ProgramInfo::findProgramDecl(const IR::Path* path) const {
    return globalNameSpaceContext->findDecl(path);
}

const IR::IDeclaration* ProgramInfo::findProgramDecl(const IR::PathExpression* pathExpr) const {
    return findProgramDecl(pathExpr->path);
}

const IR::Type_Declaration* ProgramInfo::resolveProgramType(const IR::Type_Name* type) const {
    const auto* path = type->path;
    const auto* decl = findProgramDecl(path)->to<IR::Type_Declaration>();
    BUG_CHECK(decl, "Not a type: %1%", path);
    return decl;
}

/* =============================================================================================
 *  Getters
 * ============================================================================================= */

const Coverage::CoverageSet& ProgramInfo::getAllStatements() const { return allStatements; }

const ConcolicMethodImpls* ProgramInfo::getConcolicMethodImpls() const {
    return &concolicMethodImpls;
}

const std::vector<Continuation::Command>* ProgramInfo::getPipelineSequence() const {
    return &pipelineSequence;
}

boost::optional<const Constraint*> ProgramInfo::getTargetConstraints() const {
    return targetConstraints;
}

void ProgramInfo::produceCopyInOutCall(const IR::Parameter* param, size_t paramIdx,
                                       const ArchSpec::ArchMember* archMember,
                                       std::vector<Continuation::Command>* copyIns,
                                       std::vector<Continuation::Command>* copyOuts) const {
    const auto* paramType = param->type;
    // We need to resolve type names.
    if (const auto* tn = paramType->to<IR::Type_Name>()) {
        paramType = resolveProgramType(tn);
    }
    // Retrieve the identifier of the global architecture map using the parameter
    // index.
    auto archRef = archMember->blockParams.at(paramIdx);
    // If the archRef is a nullptr or empty, we do not have a mapping for this
    // parameter.
    if (archRef.isNullOrEmpty()) {
        return;
    }
    const auto* archPath = new IR::PathExpression(paramType, new IR::Path(archRef));
    const auto* paramRef = new IR::PathExpression(paramType, new IR::Path(param->name));
    const auto* paramDir = new IR::StringLiteral(directionToString(param->direction));
    if (copyIns != nullptr) {
        // This mimicks the copy-in from the architecture environment.
        const auto* copyInCall = new IR::MethodCallStatement(IRUtils::generateInternalMethodCall(
            "copy_in", {archPath, paramRef, paramDir, new IR::BoolLiteral(false)}));
        copyIns->emplace_back(copyInCall);
    }
    if (copyOuts != nullptr) {
        // This mimicks the copy-out from the architecture environment.
        const auto* copyOutCall = new IR::MethodCallStatement(
            IRUtils::generateInternalMethodCall("copy_out", {archPath, paramRef, paramDir}));
        copyOuts->emplace_back(copyOutCall);
    }
}

}  // namespace P4Testgen

}  // namespace P4Tools
