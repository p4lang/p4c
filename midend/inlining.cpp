/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "lib/nullstream.h"
#include "frontends/p4/toP4/toP4.h"

#include "inlining.h"
#include "frontends/p4/callGraph.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/parameterSubstitution.h"
#include "frontends/p4/evaluator/substituteParameters.h"

namespace P4 {

void InlineWorkList::analyze(bool allowMultipleCalls) {
    P4::CallGraph<const IR::IContainer*> cg("Call-graph");

    for (auto m : inlineMap) {
        auto inl = m.second;
        if (inl->invocations.size() == 0) continue;
        auto it = inl->invocations.begin();
        auto first = *it;
        if (!allowMultipleCalls && inl->invocations.size() > 1) {
            ++it;
            auto second = *it;
            ::error("Multiple invocations of the same block not supported on this target: %1%, %2%",
                    first, second);
            continue;
        }
        cg.calls(inl->caller, inl->callee);
    }

    // must inline from leaves up
    std::vector<const IR::IContainer*> order;
    cg.sort(order);
    for (auto c : order) {
        // This is quadratic, but hopefully the call graph is not too large
        for (auto m : inlineMap) {
            auto inl = m.second;
            if (inl->caller == c)
                toInline.push_back(inl);
        }
    }

    std::reverse(toInline.begin(), toInline.end());
}

InlineSummary* InlineWorkList::next() {
    if (toInline.size() == 0)
        return nullptr;
    auto result = new InlineSummary();
    std::set<const IR::IContainer*> processing;
    while (!toInline.empty()) {
        auto toadd = toInline.back();
        if (processing.find(toadd->callee) != processing.end())
            break;
        toInline.pop_back();
        result->add(toadd);
        processing.emplace(toadd->caller);
    }
    return result;
}

const IR::Node* InlineDriver::preorder(IR::P4Program* program) {
    LOG1("InlineDriver");
    const IR::P4Program* prog = program;  // we need the 'const'
    toInline->analyze(false);

    while (auto todo = toInline->next()) {
        LOG1("Processing " << todo);
        inliner->prepare(toInline, todo, p4v1);
        prog = prog->apply(*inliner);
        if (::errorCount() > 0)
            return prog;
    }

    prune();
    return prog;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void DiscoverInlining::postorder(const IR::MethodCallStatement* statement) {
    LOG2("Visiting " << statement);
    auto mi = P4::MethodInstance::resolve(statement, refMap, typeMap);
    if (!mi->isApply())
        return;
    auto am = mi->to<P4::ApplyMethod>();
    CHECK_NULL(am);
    if (!am->type->is<IR::Type_Control>())
        return;
    auto instantiation = am->object->to<IR::Declaration_Instance>();
    BUG_CHECK(instantiation != nullptr, "%1% expected an instance declaration", am->object);
    inlineList->addInvocation(instantiation, statement);
}

void DiscoverInlining::visit_all(const IR::Block* block) {
    for (auto it : block->constantValue) {
        if (it.second->is<IR::Block>()) {
            visit(it.second->getNode());
        }
    }
}

bool DiscoverInlining::preorder(const IR::ControlBlock* block) {
    LOG2("Visiting " << block);
    if (getContext()->node->is<IR::ParserBlock>()) {
        if (!allowParsersFromControls)
            ::error("%1%: This target does not support invocation of a control from a parser",
                    block->node);
    } else if (getContext()->node->is<IR::ControlBlock>() && allowControls) {
        auto parent = getContext()->node->to<IR::ControlBlock>();
        LOG1("Will inline " << block << "@" << block->node << " into " << parent);
        auto instance = block->node->to<IR::Declaration_Instance>();
        auto callee = block->container;
        inlineList->addInstantiation(parent->container, callee, instance);
    }

    visit_all(block);
    visit(block->container->body);
    return false;
}

bool DiscoverInlining::preorder(const IR::ParserBlock* block) {
    LOG2("Visiting " << block);
    if (getContext()->node->is<IR::ControlBlock>()) {
        if (!allowControlsFromParsers)
            ::error("%1%: This target does not support invocation of a parser from a control",
                    block->node);
    } else if (getContext()->node->is<IR::ParserBlock>()) {
        auto parent = getContext()->node->to<IR::ParserBlock>();
        LOG1("Will inline " << block << "@" << block->node << " into " << parent);
        auto instance = block->node->to<IR::Declaration_Instance>();
        auto callee = block->container;
        inlineList->addInstantiation(parent->container, callee, instance);
    }
    visit_all(block);
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace {
class SymRenameMap {
    std::map<const IR::IDeclaration*, cstring> internalName;
    std::map<const IR::IDeclaration*, cstring> externalName;

 public:
    void setNewName(const IR::IDeclaration* decl, cstring name, cstring extName) {
        CHECK_NULL(decl);
        BUG_CHECK(!name.isNullOrEmpty() && !extName.isNullOrEmpty(), "Empty name");
        LOG1("Renaming " << decl << " to " << name);
        if (internalName.find(decl) != internalName.end())
            BUG("%1%: already renamed", decl);
        internalName.emplace(decl, name);
        externalName.emplace(decl, extName);
    }
    cstring getName(const IR::IDeclaration* decl) const {
        CHECK_NULL(decl);
        BUG_CHECK(internalName.find(decl) != internalName.end(), "%1%: no new name", decl);
        auto result = ::get(internalName, decl);
        return result;
    }
    cstring getExtName(const IR::IDeclaration* decl) const {
        CHECK_NULL(decl);
        BUG_CHECK(externalName.find(decl) != externalName.end(), "%1%: no external name", decl);
        auto result = ::get(externalName, decl);
        return result;
    }
    bool isRenamed(const IR::IDeclaration* decl) const {
        CHECK_NULL(decl);
        return internalName.find(decl) != internalName.end();
    }
};

static cstring nameFromAnnotation(const IR::Annotations* annotations,
                                  const IR::IDeclaration* decl) {
    CHECK_NULL(annotations); CHECK_NULL(decl);
    auto anno = annotations->getSingle(IR::Annotation::nameAnnotation);
    if (anno != nullptr) {
        CHECK_NULL(anno->expr);
        auto str = anno->expr->to<IR::StringLiteral>();
        CHECK_NULL(str);
        return str->value;
    }
    return decl->getName();
}

// This class computes new names for inlined objects.
// An object's name is prefixed with the instance name that includes it.
// For example:
// control c() {
//   table t() { ... }  apply { t.apply() }
// }
// control d() {
//   c() cinst;
//   apply { cinst.apply(); }
// }
// After inlining we will get:
// control d() {
//   @name("cinst.t") table t() { ... }
//   apply { t.apply(); }
// }
// So the externally visible name for the table is "cinst.t"
class ComputeNewNames : public Inspector {
    cstring           prefix;
    P4::ReferenceMap* refMap;
    SymRenameMap*     renameMap;
 public:
    ComputeNewNames(cstring prefix, P4::ReferenceMap* refMap, SymRenameMap* renameMap) :
            prefix(prefix), refMap(refMap), renameMap(renameMap) {
        BUG_CHECK(!prefix.isNullOrEmpty(), "Null prefix");
        CHECK_NULL(refMap); CHECK_NULL(renameMap);
    }

    void rename(const IR::Declaration* decl) {
        BUG_CHECK(decl->is<IR::IAnnotated>(), "%1%: no annotations", decl);
        cstring name = nameFromAnnotation(decl->to<IR::IAnnotated>()->getAnnotations(), decl);
        cstring extName = prefix + "." + name;
        cstring baseName = extName.replace('.', '_');
        cstring newName = refMap->newName(baseName);
        renameMap->setNewName(decl, newName, extName);
    }
    void postorder(const IR::P4Table* table) override { rename(table); }
    void postorder(const IR::P4Action* action) override { rename(action); }
    void postorder(const IR::Declaration_Instance* instance) override { rename(instance); }
};
}  // namespace

// Add a @name annotation ONLY.
static const IR::Annotations*
setNameAnnotation(cstring name, const IR::Annotations* annos) {
    if (annos == nullptr)
        annos = IR::Annotations::empty;
    return annos->addOrReplace(IR::Annotation::nameAnnotation,
                               new IR::StringLiteral(Util::SourceInfo(), name));
}

namespace {
// Prefix the names of all stateful declarations with a given string.
// Also, perform parameter substitution.  Unfortunately these two
// transformations have to be performed at the same time, because
// otherwise the refMap is invalidated.
class Substitutions : public SubstituteParameters {
    P4::ReferenceMap* refMap;  // updated
    const SymRenameMap*  renameMap;

 public:
    Substitutions(P4::ReferenceMap* refMap,
                  P4::ParameterSubstitution* subst,
                  P4::TypeVariableSubstitution* tvs,
                  const SymRenameMap* renameMap) :
            SubstituteParameters(refMap, subst, tvs),
            refMap(refMap), renameMap(renameMap)
    { CHECK_NULL(refMap); CHECK_NULL(renameMap); }
    const IR::Node* postorder(IR::P4Table* table) override {
        auto orig = getOriginal<IR::IDeclaration>();
        cstring newName = renameMap->getName(orig);
        cstring extName = renameMap->getExtName(orig);
        LOG1("Renaming " << orig << " to " << newName << " (" << extName << ")");
        auto annos = setNameAnnotation(extName, table->annotations);
        auto result = new IR::P4Table(table->srcInfo, newName, annos,
                                      table->parameters, table->properties);
        return result;
    }
    const IR::Node* postorder(IR::P4Action* action) override {
        auto orig = getOriginal<IR::IDeclaration>();
        cstring newName = renameMap->getName(orig);
        cstring extName = renameMap->getExtName(orig);
        LOG1("Renaming " << orig << " to " << newName << "(" << extName << ")");
        auto annos = setNameAnnotation(extName, action->annotations);
        auto result = new IR::P4Action(action->srcInfo, newName, annos,
                                       action->parameters, action->body);
        return result;
    }
    const IR::Node* postorder(IR::Declaration_Instance* instance) override {
        auto orig = getOriginal<IR::IDeclaration>();
        cstring newName = renameMap->getName(orig);
        cstring extName = renameMap->getExtName(orig);
        LOG1("Renaming " << orig << " to " << newName << "(" << extName << ")");
        auto annos = setNameAnnotation(extName, instance->annotations);
        instance->name = newName;
        instance->annotations = annos;
        return instance;
    }
    const IR::Node* postorder(IR::PathExpression* expression) override {
        LOG1("Visiting (AddNamePrefix) " << expression);
        auto decl = refMap->getDeclaration(expression->path, true);
        auto param = decl->to<IR::Parameter>();
        if (param != nullptr && subst->contains(param)) {
            // This path is the same as in SubstituteParameters
            auto value = subst->lookup(param);
            LOG1("Replaced " << expression << " with " << value);
            return value;
        }

        cstring newName;
        if (renameMap->isRenamed(decl))
            newName = renameMap->getName(decl);
        else
            newName = expression->path->name;
        IR::ID newid = IR::ID(expression->path->srcInfo, newName);
        auto newpath = new IR::Path(expression->path->prefix, newid);
        auto result = new IR::PathExpression(newpath);
        refMap->setDeclaration(newpath, decl);
        LOG1("Replaced " << expression << " with " << result);
        return result;
    }
};
}  // namespace

Visitor::profile_t GeneralInliner::init_apply(const IR::Node* node) {
    P4::ResolveReferences solver(refMap, true);
    node->apply(solver);
    return AbstractInliner::init_apply(node);
}

const IR::Node* GeneralInliner::preorder(IR::P4Control* caller) {
    auto orig = getOriginal<IR::P4Control>();
    if (toInline->callerToWork.find(orig) == toInline->callerToWork.end()) {
        prune();
        return caller;
    }

    workToDo = &toInline->callerToWork[orig];
    LOG1("Analyzing " << caller);
    auto locals = new IR::IndexedVector<IR::Declaration>();
    for (auto s : *caller->controlLocals) {
        auto inst = s->to<IR::Declaration_Instance>();
        if (inst == nullptr ||
            workToDo->declToCallee.find(inst) == workToDo->declToCallee.end()) {
            // not a call
            locals->push_back(s);
        } else {
            auto callee = workToDo->declToCallee[inst]->to<IR::P4Control>();
            CHECK_NULL(callee);
            ParameterSubstitution subst;
            TypeVariableSubstitution tvs;

            // Substitute constructor parameters
            subst.populate(callee->getConstructorParameters(), inst->arguments);
            if (inst->type->is<IR::Type_Specialized>()) {
                auto spec = inst->type->to<IR::Type_Specialized>();
                tvs.setBindings(callee->getNode(), callee->getTypeParameters(), spec->arguments);
            }

            auto paramRename = new std::map<cstring, cstring>();
            workToDo->paramRename[inst] = paramRename;
            // Substitute also applyParameters with fresh variable names.
            auto it = inst->arguments->begin();
            for (auto param : *callee->type->applyParams->parameters) {
                cstring newName = refMap->newName(param->name);
                paramRename->emplace(param->name, newName);
                LOG1("Replacing " << param->name << " with " << newName);
                if (param->direction == IR::Direction::None) {
                    auto initializer = *it;
                    subst.add(param, initializer);
                } else  {
                    auto vardecl = new IR::Declaration_Variable(Util::SourceInfo(), newName,
                                                                param->annotations, param->type,
                                                                nullptr);
                    locals->push_back(vardecl);
                    auto path = new IR::PathExpression(newName);
                    refMap->setDeclaration(path->path, vardecl);
                    subst.add(param, path);
                }
                ++it;
            }

            // Must rename stateful objects prefixing them with their instance name.
            SymRenameMap renameMap;
            cstring prefix = nameFromAnnotation(inst->annotations, inst);
            ComputeNewNames cnn(prefix, refMap, &renameMap);
            auto clone = callee->apply(cnn);
            CHECK_NULL(clone);

            Substitutions rename(refMap, &subst, &tvs, &renameMap);
            clone = clone->apply(rename);
            CHECK_NULL(clone);

            for (auto i : *clone->to<IR::P4Control>()->controlLocals)
                locals->push_back(i);
            workToDo->declToCallee[inst] = clone->to<IR::IContainer>();
        }
    }

    visit(caller->body);
    auto result = new IR::P4Control(caller->srcInfo, caller->name, caller->type,
                                    caller->constructorParams, locals,
                                    caller->body);
    list->replace(orig, result);
    workToDo = nullptr;
    prune();
    return result;
}

const IR::Node* GeneralInliner::preorder(IR::MethodCallStatement* statement) {
    if (workToDo == nullptr)
        return statement;
    auto orig = getOriginal<IR::MethodCallStatement>();
    if (workToDo->callToinstance.find(orig) == workToDo->callToinstance.end())
        return statement;
    LOG1("Inlining invocation " << orig);
    auto decl = workToDo->callToinstance[orig];
    CHECK_NULL(decl);

    auto callee = workToDo->declToCallee[decl]->to<IR::P4Control>();
    auto body = new IR::IndexedVector<IR::StatOrDecl>();

    // Evaluate in and inout parameters in order.
    std::map<cstring, cstring> *paramRename = workToDo->paramRename[decl];
    auto it = statement->methodCall->arguments->begin();
    for (auto param : *callee->type->applyParams->parameters) {
        auto initializer = *it;
        LOG1("Looking for " << param->name);
        cstring newName = ::get(*paramRename, param->name);
        if (param->direction == IR::Direction::In || param->direction == IR::Direction::InOut) {
            auto path = new IR::PathExpression(newName);
            auto stat = new IR::AssignmentStatement(Util::SourceInfo(), path, initializer);
            body->push_back(stat);
        }
        ++it;
    }

    // inline actual body
    body->append(*callee->body->components);

    // Copy back out and inout parameters
    it = statement->methodCall->arguments->begin();
    for (auto param : *callee->type->applyParams->parameters) {
        auto left = *it;
        if (param->direction == IR::Direction::InOut || param->direction == IR::Direction::Out) {
            cstring newName = ::get(paramRename, param->name);
            auto right = new IR::PathExpression(newName);
            auto copyout = new IR::AssignmentStatement(Util::SourceInfo(), left, right);
            body->push_back(copyout);
        }
        ++it;
    }

    auto result = new IR::BlockStatement(statement->srcInfo, body);
    LOG1("Replacing " << orig << " with " << result);
    prune();
    return result;
}

const IR::Node* GeneralInliner::preorder(IR::P4Parser* caller) {
    // TODO
    return caller;
}

}  // namespace P4
