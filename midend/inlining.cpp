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
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/moveDeclarations.h"
#include "frontends/p4/resetHeaders.h"

namespace P4 {

namespace {
static cstring nameFromAnnotation(const IR::Annotations* annotations,
                                  const IR::IDeclaration* decl) {
    CHECK_NULL(annotations); CHECK_NULL(decl);
    auto anno = annotations->getSingle(IR::Annotation::nameAnnotation);
    if (anno != nullptr) {
        BUG_CHECK(anno->expr.size() == 1, "name annotation needs single expression");
        auto str = anno->expr[0]->to<IR::StringLiteral>();
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

// Add a @name annotation ONLY.
static const IR::Annotations*
setNameAnnotation(cstring name, const IR::Annotations* annos) {
    if (annos == nullptr)
        annos = IR::Annotations::empty;
    return annos->addOrReplace(IR::Annotation::nameAnnotation,
                               new IR::StringLiteral(Util::SourceInfo(), name));
}

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
        auto newpath = new IR::Path(newid, expression->path->absolute);
        auto result = new IR::PathExpression(newpath);
        refMap->setDeclaration(newpath, decl);
        LOG1("Replaced " << expression << " with " << result);
        return result;
    }
};
}  // namespace

template <class T>
const T* PerInstanceSubstitutions::rename(ReferenceMap* refMap, const IR::Node* node) {
    Substitutions rename(refMap, &paramSubst, &tvs, &renameMap);
    auto convert = node->apply(rename);
    CHECK_NULL(convert);
    auto result = convert->to<T>();
    CHECK_NULL(result);
    return result;
}

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
    toInline->analyze(true);

    while (auto todo = toInline->next()) {
        LOG1("Processing " << todo);
        inliner->prepare(toInline, todo);
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
    auto mi = MethodInstance::resolve(statement, refMap, typeMap);
    if (!mi->isApply())
        return;
    auto am = mi->to<P4::ApplyMethod>();
    CHECK_NULL(am);
    if (!am->applyObject->is<IR::Type_Control>() &&
        !am->applyObject->is<IR::Type_Parser>())
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
    visit(block->container->states);
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

Visitor::profile_t GeneralInliner::init_apply(const IR::Node* node) {
    ResolveReferences solver(refMap);
    TypeChecking typeChecker(refMap, typeMap);
    node->apply(solver);
    (void)node->apply(typeChecker);
    return AbstractInliner::init_apply(node);
}

const IR::Node* GeneralInliner::preorder(IR::P4Control* caller) {
    // prepares the code to inline
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
            auto substs = new PerInstanceSubstitutions();
            workToDo->substitutions[inst] = substs;

            // Substitute constructor parameters
            substs->paramSubst.populate(callee->getConstructorParameters(), inst->arguments);
            if (inst->type->is<IR::Type_Specialized>()) {
                auto spec = inst->type->to<IR::Type_Specialized>();
                substs->tvs.setBindings(callee->getNode(),
                                        callee->getTypeParameters(), spec->arguments);
            }

            // Must rename callee local objects prefixing them with their instance name.
            cstring prefix = nameFromAnnotation(inst->annotations, inst);
            ComputeNewNames cnn(prefix, refMap, &substs->renameMap);
            (void)callee->apply(cnn);  // populates substs.renameMap

            // Substitute applyParameters which are not directionless
            // with fresh variable names.
            for (auto param : *callee->type->applyParams->parameters) {
                cstring newName = refMap->newName(param->name);
                auto path = new IR::PathExpression(newName);
                substs->paramSubst.add(param, path);
                LOG1("Replacing " << param->name << " with " << newName);
                if (param->direction == IR::Direction::None)
                    continue;
                auto vardecl = new IR::Declaration_Variable(Util::SourceInfo(), newName,
                                                            param->annotations, param->type,
                                                            nullptr);
                locals->push_back(vardecl);
            }

            /* We will perform these substitutions twice: once here, to
               compute the names for the locals that we need to inline here,
               and once again at the call site (where we do additional
               substitutions, including the callee parameters). */
            auto clone = substs->rename<IR::P4Control>(refMap, callee);
            for (auto i : *clone->controlLocals)
                locals->push_back(i);
        }
    }

    visit(caller->body);
    caller->controlLocals = locals;
    list->replace(orig, caller);
    workToDo = nullptr;
    prune();
    return caller;
}

const IR::Node* GeneralInliner::preorder(IR::MethodCallStatement* statement) {
    if (workToDo == nullptr)
        return statement;
    auto orig = getOriginal<IR::MethodCallStatement>();
    if (workToDo->callToInstance.find(orig) == workToDo->callToInstance.end())
        return statement;
    LOG1("Inlining invocation " << orig);
    auto decl = workToDo->callToInstance[orig];
    CHECK_NULL(decl);

    auto called = workToDo->declToCallee[decl];
    if (!called->is<IR::P4Control>())
        // Parsers are inlined in the ParserState processor
        return statement;

    auto callee = called->to<IR::P4Control>();
    auto body = new IR::IndexedVector<IR::StatOrDecl>();
    // clone the substitution: it may be reused for multiple invocations
    auto substs = new PerInstanceSubstitutions(*workToDo->substitutions[decl]);

    // Substitute directionless parameter with their actual values.
    // Evaluate in and inout parameters in order.
    auto it = statement->methodCall->arguments->begin();
    for (auto param : *callee->type->applyParams->parameters) {
        auto initializer = *it;
        LOG1("Looking for " << param->name);
        if (param->direction == IR::Direction::In || param->direction == IR::Direction::InOut) {
            auto expr = substs->paramSubst.lookupByName(param->name);
            auto stat = new IR::AssignmentStatement(Util::SourceInfo(), expr, initializer);
            body->push_back(stat);
        } else if (param->direction == IR::Direction::Out) {
            auto expr = substs->paramSubst.lookupByName(param->name);
            auto paramType = typeMap->getType(param, true);
            // This is important, since this variable may be used many times.
            DoResetHeaders::generateResets(typeMap, paramType, expr, body);
        } else if (param->direction == IR::Direction::None) {
            substs->paramSubst.add(param, initializer);
        }
        ++it;
    }

    // inline actual body
    callee = substs->rename<IR::P4Control>(refMap, callee);
    body->append(*callee->body->components);

    // Copy values of out and inout parameters
    it = statement->methodCall->arguments->begin();
    for (auto param : *callee->type->applyParams->parameters) {
        auto left = *it;
        if (param->direction == IR::Direction::InOut || param->direction == IR::Direction::Out) {
            auto expr = substs->paramSubst.lookupByName(param->name);
            auto copyout = new IR::AssignmentStatement(Util::SourceInfo(), left, expr->clone());
            body->push_back(copyout);
        }
        ++it;
    }

    auto annotations = callee->type->annotations->where(
        [](const IR::Annotation* a) { return a->name != IR::Annotation::nameAnnotation; });
    auto result = new IR::BlockStatement(statement->srcInfo, annotations, body);
    LOG1("Replacing " << orig << " with " << result);
    prune();
    return result;
}

namespace {
class ComputeNewStateNames : public Inspector {
    ReferenceMap* refMap;
    cstring prefix;
    cstring acceptName;
    std::map<cstring, cstring> *stateRenameMap;
 public:
    ComputeNewStateNames(ReferenceMap* refMap, cstring prefix, cstring acceptName,
                         std::map<cstring, cstring> *stateRenameMap) :
            refMap(refMap), prefix(prefix), acceptName(acceptName), stateRenameMap(stateRenameMap)
    { CHECK_NULL(refMap); CHECK_NULL(stateRenameMap); }
    bool preorder(const IR::ParserState* state) override {
        cstring newName;
        if (state->name.name == IR::ParserState::accept) {
            newName = acceptName;
        } else {
            cstring base = prefix + "_" + state->name.name;
            newName = refMap->newName(base);
        }
        stateRenameMap->emplace(state->name.name, newName);
        return false;  // prune
    }
};

// Renames the states in a parser for inlining.  We cannot rely on the
// ReferenceMap for identifying states - it is currently inconsistent,
// so we rely on the fact that state names only appear in very
// specific places.
class RenameStates : public Transform {
    std::map<cstring, cstring> *stateRenameMap;

 public:
    explicit RenameStates(std::map<cstring, cstring> *stateRenameMap) :
            stateRenameMap(stateRenameMap)
    { CHECK_NULL(stateRenameMap); }
    const IR::Node* preorder(IR::Path* path) override {
        // This is certainly a state name, by the way we organized the visitors
        cstring newName = ::get(stateRenameMap, path->name);
        path->name = IR::ID(path->name.srcInfo, newName);
        return path;
    }
    const IR::Node* preorder(IR::SelectExpression* expression) override {
        visit(&expression->selectCases);
        prune();
        return expression;
    }
    const IR::Node* preorder(IR::SelectCase* selCase) override {
        visit(selCase->state);
        prune();
        return selCase;
    }
    const IR::Node* preorder(IR::ParserState* state) override {
        if (state->name.name == IR::ParserState::accept ||
            state->name.name == IR::ParserState::reject) {
            prune();
            return state;
        }
        cstring newName = ::get(stateRenameMap, state->name.name);
        state->name = IR::ID(state->name.srcInfo, newName);
        if (state->selectExpression != nullptr)
            visit(state->selectExpression);
        prune();
        return state;
    }
    const IR::Node* preorder(IR::P4Parser* parser) override {
        visit(parser->states);
        prune();
        return parser;
    }
};
}  // namespace

const IR::Node* GeneralInliner::preorder(IR::ParserState* state) {
    LOG1("Visiting state " << state);
    auto states = new IR::IndexedVector<IR::ParserState>();
    auto current = new IR::IndexedVector<IR::StatOrDecl>();

    // Scan the statements to find a parser call instruction
    auto srcInfo = state->srcInfo;
    auto annotations = state->annotations;
    IR::ID name = state->name;
    for (auto e : *state->components) {
        if (!e->is<IR::MethodCallStatement>()) {
            current->push_back(e);
            continue;
        }
        auto call = e->to<IR::MethodCallStatement>();
        if (workToDo->callToInstance.find(call) == workToDo->callToInstance.end()) {
            current->push_back(e);
            continue;
        }

        LOG1("Inlining invocation " << call);
        auto decl = workToDo->callToInstance[call];
        CHECK_NULL(decl);

        auto called = workToDo->declToCallee[decl];
        auto callee = called->to<IR::P4Parser>();
        // clone the substitution: it may be reused for multiple invocations
        auto substs = new PerInstanceSubstitutions(*workToDo->substitutions[decl]);

        // Evaluate in and inout parameters in order.
        auto it = call->methodCall->arguments->begin();
        for (auto param : *callee->type->applyParams->parameters) {
            auto initializer = *it;
            LOG1("Looking for " << param->name);
            if (param->direction == IR::Direction::In || param->direction == IR::Direction::InOut) {
                auto expr = substs->paramSubst.lookupByName(param->name);
                auto stat = new IR::AssignmentStatement(Util::SourceInfo(), expr, initializer);
                current->push_back(stat);
            } else if (param->direction == IR::Direction::Out) {
                auto expr = substs->paramSubst.lookupByName(param->name);
                auto paramType = typeMap->getType(param, true);
                // This is important, since this variable may be used many times.
                DoResetHeaders::generateResets(typeMap, paramType, expr, current);
            } else if (param->direction == IR::Direction::None) {
                substs->paramSubst.add(param, initializer);
            }
            ++it;
        }

        callee = substs->rename<IR::P4Parser>(refMap, callee);

        cstring nextState = refMap->newName(state->name);
        std::map<cstring, cstring> renameMap;
        ComputeNewStateNames cnn(refMap, callee->name.name, nextState, &renameMap);
        (void)callee->apply(cnn);
        RenameStates rs(&renameMap);
        auto renamed = callee->apply(rs);
        cstring newStartName = ::get(renameMap, IR::ParserState::start);
        auto transition = new IR::PathExpression(IR::ID(newStartName));
        auto newState = new IR::ParserState(srcInfo, name, annotations, current, transition);
        states->push_back(newState);
        for (auto s : *renamed->to<IR::P4Parser>()->states) {
            if (s->name == IR::ParserState::accept ||
                s->name == IR::ParserState::reject)
                continue;
            states->push_back(s);
        }

        // Prepare next state
        annotations = IR::Annotations::empty;
        srcInfo = Util::SourceInfo();
        name = IR::ID(nextState);
        current = new IR::IndexedVector<IR::StatOrDecl>();

        // Copy back out and inout parameters
        it = call->methodCall->arguments->begin();
        for (auto param : *callee->type->applyParams->parameters) {
            auto left = *it;
            if (param->direction == IR::Direction::InOut ||
                param->direction == IR::Direction::Out) {
                auto expr = substs->paramSubst.lookupByName(param->name);
                auto copyout = new IR::AssignmentStatement(Util::SourceInfo(), left, expr->clone());
                current->push_back(copyout);
            }
            ++it;
        }
    }

    if (!states->empty()) {
        // Create final state
        auto newState = new IR::ParserState(srcInfo, name, annotations,
                                            current, state->selectExpression);
        states->push_back(newState);
        LOG1("Replacing with " << states->size() << " states");
        prune();
        return states;
    }
    prune();
    return state;
}

const IR::Node* GeneralInliner::preorder(IR::P4Parser* caller) {
    // prepares the code to inline
    auto orig = getOriginal<IR::P4Parser>();
    if (toInline->callerToWork.find(orig) == toInline->callerToWork.end()) {
        prune();
        return caller;
    }

    workToDo = &toInline->callerToWork[orig];
    LOG1("Analyzing " << caller);
    auto locals = new IR::IndexedVector<IR::Declaration>();
    for (auto s : *caller->parserLocals) {
        auto inst = s->to<IR::Declaration_Instance>();
        if (inst == nullptr ||
            workToDo->declToCallee.find(inst) == workToDo->declToCallee.end()) {
            // not a call
            locals->push_back(s);
        } else {
            auto callee = workToDo->declToCallee[inst]->to<IR::P4Parser>();
            CHECK_NULL(callee);
            auto substs = new PerInstanceSubstitutions();
            workToDo->substitutions[inst] = substs;

            // Substitute constructor parameters
            substs->paramSubst.populate(callee->getConstructorParameters(), inst->arguments);
            if (inst->type->is<IR::Type_Specialized>()) {
                auto spec = inst->type->to<IR::Type_Specialized>();
                substs->tvs.setBindings(callee->getNode(),
                                        callee->getTypeParameters(), spec->arguments);
            }

            // Must rename callee local objects prefixing them with their instance name.
            cstring prefix = nameFromAnnotation(inst->annotations, inst);
            ComputeNewNames cnn(prefix, refMap, &substs->renameMap);
            (void)callee->apply(cnn);  // populates substs.renameMap

            // Substitute applyParameters which are not directionless
            // with fresh variable names.
            for (auto param : *callee->type->applyParams->parameters) {
                if (param->direction == IR::Direction::None)
                    continue;
                cstring newName = refMap->newName(param->name);
                auto path = new IR::PathExpression(newName);
                substs->paramSubst.add(param, path);
                LOG1("Replacing " << param->name << " with " << newName);
                auto vardecl = new IR::Declaration_Variable(Util::SourceInfo(), newName,
                                                            param->annotations, param->type,
                                                            nullptr);
                locals->push_back(vardecl);
            }

            /* We will perform these substitutions twice: once here, to
               compute the names for the locals that we need to inline here,
               and once again at the call site (where we do additional
               substitutions, including the callee parameters). */
            auto clone = substs->rename<IR::P4Parser>(refMap, callee);
            for (auto i : *clone->parserLocals)
                locals->push_back(i);
        }
    }

    visit(caller->states);
    caller->parserLocals = locals;
    list->replace(orig, caller);
    workToDo = nullptr;
    prune();
    return caller;
}

}  // namespace P4
