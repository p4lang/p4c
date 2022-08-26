#include "backends/p4tools/testgen/core/program_info.h"

#include "lib/exceptions.h"

namespace P4Tools {

namespace P4Testgen {

ProgramInfo::ProgramInfo(const IR::P4Program* program)
    : globalNameSpaceContext(NamespaceContext::Empty->push(program)),
      concolicMethodImpls({}),
      program(program) {
    concolicMethodImpls.add(*Concolic::getCoreConcolicMethodImpls());
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

const ConcolicMethodImpls* ProgramInfo::getConcolicMethodImpls() const {
    return &concolicMethodImpls;
}

const std::vector<Continuation::Command>* ProgramInfo::getPipelineSequence() const {
    return &pipelineSequence;
}

boost::optional<const Constraint*> ProgramInfo::getTargetConstraints() const {
    return targetConstraints;
}

}  // namespace P4Testgen

}  // namespace P4Tools
