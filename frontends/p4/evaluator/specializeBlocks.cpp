#include "specializeBlocks.h"
#include "substituteParameters.h"
#include "frontends/p4/toP4/toP4.h"

namespace P4 {

BlockSpecialization::BlockSpecialization(const IR::Node* constructor,
                                         const IR::IContainer* cont,
                                         const IR::Vector<IR::Expression>* arguments,
                                         const IR::Vector<IR::Type>* typeArguments,
                                         cstring newName)  :
        constructor(constructor), cont(cont), arguments(arguments), typeArguments(typeArguments) {
    CHECK_NULL(constructor); CHECK_NULL(cont); CHECK_NULL(arguments);
    // These identifiers have different source positions, which is important
    // when resolving references: the declaration must precede the use.
    cloneId = IR::ID(cont->getNode()->srcInfo, newName);
    useId = IR::ID(constructor->srcInfo, newName);
    // FIXME: This is probably not sufficient, since the
    // newName may be in a different scope.
    auto path = new IR::Path(useId);
    type = new IR::Type_Name(useId.srcInfo, path);
}

void BlockSpecialization::dbprint(std::ostream& out) const {
    out << "Specialization of " << cont << " under " << cloneId << IndentCtl::endl;
}

//////////////////////////////////////////////////

void BlockSpecList::specialize(const IR::Node* node, const IR::IContainer* cont,
                               const IR::Vector<IR::Expression>* args,
                               const IR::Vector<IR::Type>* typeArgs,
                               cstring name) {
    BlockSpecialization* bs = new BlockSpecialization(node, cont, args, typeArgs, name);
    toSpecialize.push_back(bs);
    auto there = get(constructorMap, node);
    BUG_CHECK(there == nullptr, "Duplicate entry in map %1%", node);
    constructorMap.emplace(node, bs);
    auto repls = get(containerMap, cont);
    if (repls == nullptr) {
        repls = new std::vector<const BlockSpecialization*>();
        containerMap.emplace(cont, repls);
    }
    repls->push_back(bs);
}

void BlockSpecList::dbprint(std::ostream& out) const {
    for (auto s : toSpecialize)
        s->dbprint(out);
    for (auto it : containerMap) {
        for (auto bs : *it.second)
            out << it.first << " => " << bs;
    }
}

//////////////////////////////////////////////////

bool FindBlocksToSpecialize::isConstant(const IR::Expression* expr) {
    if (expr->is<IR::Constant>()) return true;
    if (expr->is<IR::BoolLiteral>()) return true;
    // TODO: there are more cases to consider, e.g., enums or externs
    return false;
}

Visitor::profile_t FindBlocksToSpecialize::init_apply(const IR::Node* node) {
    blocks->clear();
#if 0
    std::clog << "-------------------------" << std::endl;
    std::clog << "Before specialization" << std::endl;
    dump(node);
    std::clog << std::endl;
#endif
    return Inspector::init_apply(node);
}

void
FindBlocksToSpecialize::processNode(const IR::Node* node,
                                    const IR::Vector<IR::Expression>* args,
                                    const IR::Type* type) {
    LOG1("Checking " << node);
    const IR::Vector<IR::Type>* typeArgs;
    if (type->is<IR::Type_Specialized>()) {
        auto spec = type->to<IR::Type_Specialized>();
        typeArgs = spec->arguments;
        type = spec->baseType;
    } else {
        typeArgs = new IR::Vector<IR::Type>();
    }
    auto cont = typeMap->getContainerFromType(typeMap->getType(type));
    if (cont == nullptr || cont->is<IR::Type_Package>()) {
        LOG1("Skipping " << type);
        return;
    }

    if (args->size() == 0 && typeArgs->size() == 0)
        return;

    auto ctx = getContext();
    while (ctx != nullptr) {
        // Check if declaration is within another parameterized container.
        // If so we won't specialize it.
        if (ctx->node->is<IR::P4Control>() || ctx->node->is<IR::P4Parser>()) {
            auto cont = ctx->node->to<IR::IContainer>();
            auto mt = cont->getConstructorMethodType();
            if (mt->parameters->size() != 0 || mt->typeParameters->size() != 0) {
                LOG1("Will not instantiate call " << node <<
                     " within parameterized constructor " << ctx->node);
                return;
            }
        }
        ctx = ctx->parent;
    }

    for (auto arg : *args) {
        if (!isConstant(arg))
            ::error("Cannot evaluate constructor parameter %1% to a constant", arg);
    }

    cstring name = cont->to<IR::Type_Declaration>()->name.name;
    auto newname = refMap->newName(name);
    LOG1("Need to specialize " << node);
    blocks->specialize(node, cont, args, typeArgs, newname);
}

bool FindBlocksToSpecialize::preorder(const IR::Declaration_Instance* inst) {
    processNode(inst, inst->arguments, inst->type);
    return true;
}

bool FindBlocksToSpecialize::preorder(const IR::ConstructorCallExpression* expr) {
    processNode(expr, expr->arguments, expr->type);
    return true;
}

//////////////////////////////////////////////////

const IR::Node* SpecializeBlocks::postorder(IR::P4Program* prog) {
    return prog;
}

const IR::Node* SpecializeBlocks::postorder(IR::P4Parser* cont) {
    LOG1("Checking " << getOriginal());
    auto replacements = blocks->findReplacements(getOriginal<IR::IContainer>());
    if (replacements == nullptr)
        return cont;

    LOG1("Specializing " << getOriginal());

    auto result = new IR::Vector<IR::Node>();
    result->push_back(cont);
    // This will be interpolated in the parent Vector.
    // Parsers are always children of a P4Program,
    // in the 'declarations' field.  It is unpleasant to rely on this fact,
    // but I don't know how to do better at this point.

    for (auto bs : *replacements) {
        IR::ParameterSubstitution ps;
        ps.populate(cont->constructorParams, bs->arguments);
        IR::TypeVariableSubstitution tvs;
        tvs.setBindings(cont, cont->getTypeParameters(), bs->typeArguments);

        SubstituteParameters sp(refMap, &ps, &tvs);
        auto clone = cont->apply(sp);
        if (clone == nullptr)
            return cont;

        auto specialized = clone->to<IR::P4Parser>();
        auto newtype = new IR::Type_Parser(specialized->type->srcInfo, bs->cloneId,
                                           specialized->type->annotations,
                                           specialized->type->typeParams,
                                           specialized->type->applyParams);
        auto newcont = new IR::P4Parser(specialized->srcInfo, bs->cloneId, newtype,
                                        new IR::ParameterList(), specialized->stateful,
                                        specialized->states);
        LOG1("Synthesized " << newcont);
        result->push_back(newcont);
    }
    return result;
}

const IR::Node* SpecializeBlocks::postorder(IR::P4Control* cont) {
    auto replacements = blocks->findReplacements(getOriginal<IR::IContainer>());
    if (replacements == nullptr)
        return cont;

    auto result = new IR::Vector<IR::Node>();
    result->push_back(cont);
    for (auto bs : *replacements) {
        IR::ParameterSubstitution ps;
        ps.populate(cont->constructorParams, bs->arguments);
        IR::TypeVariableSubstitution tvs;
        tvs.setBindings(cont, cont->getTypeParameters(), bs->typeArguments);

        SubstituteParameters sp(refMap, &ps, &tvs);
        auto clone = cont->apply(sp);
        if (clone == nullptr)
            return cont;

        auto specialized = clone->to<IR::P4Control>();
        auto newtype = new IR::Type_Control(specialized->type->srcInfo, bs->cloneId,
                                            specialized->type->annotations,
                                            specialized->type->typeParams,
                                            specialized->type->applyParams);
        auto newcont = new IR::P4Control(specialized->srcInfo, bs->cloneId, newtype,
                                               new IR::ParameterList(), specialized->stateful,
                                               specialized->body);
        LOG1("Synthesized " << newcont);
        result->push_back(newcont);
    }
    return result;
}

const IR::Node* SpecializeBlocks::postorder(IR::Declaration_Instance* inst) {
    auto bs = blocks->findInvocation(getOriginal());
    if (bs == nullptr)
        return inst;
    if (!bs->type->is<IR::Type_Name>() && !bs->type->is<IR::Type_Specialized>())
        BUG("Incorrect type in Declaration_Instance %1%", bs->type);
    auto result = new IR::Declaration_Instance(inst->srcInfo, inst->name, bs->type,
                                               new IR::Vector<IR::Expression>(), inst->annotations);
    LOG1("Replacing " << inst << " with " << result);
    return result;
}

const IR::Node* SpecializeBlocks::postorder(IR::ConstructorCallExpression* expr) {
    auto bs = blocks->findInvocation(getOriginal());
    if (bs == nullptr)
        return expr;
    auto result = new IR::ConstructorCallExpression(expr->srcInfo, bs->type,
                                                    new IR::Vector<IR::Expression>());
    LOG1("Replacing " << expr << " with " << result);
    return result;
}

Visitor::profile_t SpecializeBlocks::init_apply(const IR::Node* node) {
    LOG1("Starting specialization " << std::endl << blocks);
    return Transform::init_apply(node);
}

}  // namespace P4
