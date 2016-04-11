#include "controlInlining.h"
#include "frontends/p4/callGraph.h"
#include "frontends/p4/evaluator/substituteParameters.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/parameterSubstitution.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace BMV2 {

CInlineWorkList* ControlsInlineList::next() {
    if (toInline.size() == 0)
        return nullptr;
    auto result = new CInlineWorkList();
    std::set<const IR::P4Control*> processing;
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

void CInlineWorkList::add(ControlsCallInfo* cci) {
    callerToWork[cci->caller].declToCallee[cci->instantiation] = cci->callee;
    for (auto mcs : cci->invocations) {
        callerToWork[cci->caller].callToinstance[mcs] = cci->instantiation;
    }
}

///////////////////////////////////////////////////////////////////////////////////////

class SimpleControlsInliner : public Transform {
    P4::ReferenceMap* refMap;
    ControlsInlineList* list;
    CInlineWorkList* toInline;
    CInlineWorkList::PerCaller* workToDo;
 public:
    explicit SimpleControlsInliner(P4::ReferenceMap* refMap, ControlsInlineList* list,
                                   CInlineWorkList* toInline) :
            refMap(refMap), list(list), toInline(toInline), workToDo(nullptr)
    { CHECK_NULL(refMap); CHECK_NULL(list); CHECK_NULL(toInline); }

    Visitor::profile_t init_apply(const IR::Node* node) {
        LOG1("SimpleControlsInliner " << toInline);
        return Transform::init_apply(node);
    }

    const IR::Node* preorder(IR::MethodCallStatement* statement) override;
    const IR::Node* preorder(IR::P4Control* caller) override;
};

const IR::Node* SimpleControlsInliner::preorder(IR::P4Control* caller) {
    prune();
    auto orig = getOriginal<IR::P4Control>();
    if (toInline->callerToWork.find(orig) == toInline->callerToWork.end())
        return caller;

    workToDo = &toInline->callerToWork[orig];
    LOG1("Simple inliner " << caller);
    auto stateful = new IR::NameMap<IR::Declaration, ordered_map>();
    for (auto s : *caller->statefulEnumerator()) {
        auto inst = s->to<IR::Declaration_Instance>();
        if (inst == nullptr ||
            workToDo->declToCallee.find(inst) == workToDo->declToCallee.end()) {
            // If some element with the same name is already there we don't reinsert it
            // since this program is generated from a P4 v1.0 program by duplicating
            // elements.
            if (stateful->getUnique(s->name) == nullptr)
                stateful->addUnique(s->name, s);
        } else {
            auto callee = workToDo->declToCallee[inst];
            CHECK_NULL(callee);
            IR::ParameterSubstitution subst;
            // TODO: this is correct only if the arguments have no side-effects.
            // There should be a prior pass to ensure this fact.  This is
            // true for programs that come out of the P4 v1.0 front-end.
            subst.populate(callee->constructorParams, inst->arguments);
            IR::TypeVariableSubstitution tvs;
            if (inst->type->is<IR::Type_Specialized>()) {
                auto spec = inst->type->to<IR::Type_Specialized>();
                tvs.setBindings(callee, callee->getTypeParameters(), spec->arguments);
            }
            P4::SubstituteParameters sp(refMap, &subst, &tvs);
            auto clone = callee->apply(sp);
            if (clone == nullptr)
                return caller;
            for (auto i : *clone->statefulEnumerator()) {
                if (stateful->getUnique(i->name) == nullptr)
                    stateful->addUnique(i->name, i);
            }
            workToDo->declToCallee[inst] = clone;
        }
    }

    visit(caller->body);
    auto result = new IR::P4Control(caller->srcInfo, caller->name, caller->type,
                                           caller->constructorParams, std::move(*stateful),
                                           caller->body);
    list->replace(orig, result);
    workToDo = nullptr;
    return result;
}

const IR::Node* SimpleControlsInliner::preorder(IR::MethodCallStatement* statement) {
    if (workToDo == nullptr)
        return statement;
    auto orig = getOriginal<IR::MethodCallStatement>();
    if (workToDo->callToinstance.find(orig) == workToDo->callToinstance.end())
        return statement;
    LOG1("Inlining invocation " << orig);
    prune();
    auto decl = workToDo->callToinstance[orig];
    CHECK_NULL(decl);
    return workToDo->declToCallee[decl]->body;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void DiscoverControlsInlining::postorder(const IR::MethodCallStatement* statement) {
    LOG2("Visiting " << statement);
    auto mi = P4::MethodInstance::resolve(statement, blockMap->refMap, blockMap->typeMap);
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

void DiscoverControlsInlining::visit_all(const IR::Block* block) {
    for (auto it : block->constantValue) {
        if (it.second->is<IR::Block>()) {
            visit(it.second->getNode());
        }
    }
}

bool DiscoverControlsInlining::preorder(const IR::ControlBlock* block) {
    LOG2("Visiting " << block);
    if (getContext()->node->is<IR::ParserBlock>()) {
        ::error("%1%: This target does not support invocation of a control from a parser",
                block->node);
    } else if (getContext()->node->is<IR::ControlBlock>()) {
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

bool DiscoverControlsInlining::preorder(const IR::ParserBlock* block) {
    LOG2("Visiting " << block);
    if (getContext()->node->is<IR::ControlBlock>()) {
        ::error("%1%: This target does not support invocation of a parser from a control",
                block->node);
    } else if (getContext()->node->is<IR::ParserBlock>()) {
        BUG("%1%: Parser inlining not yet implmented", block->node);
    }
    visit_all(block);
    return false;
}

const IR::Node* InlineControlsDriver::preorder(IR::P4Program* program) {
    LOG1("Inline controls driver");
    const IR::P4Program* prog = program;
    P4::ResolveReferences solver(refMap, true);

    toInline->analyze();
    while (auto todo = toInline->next()) {
        LOG1("Processing " << todo);
        SimpleControlsInliner si(refMap, toInline, todo);
        prog = prog->apply(si);
        if (::errorCount() > 0)
            return prog;
        // Must re-resolve references
        // TODO: this is too slow; should update references incrementally
        prog->apply(solver);
    }

    prune();
    return prog;
}

void ControlsInlineList::analyze() {
    P4::CallGraph<const IR::P4Control*> cg("Control call-graph");

    for (auto m : inlineMap) {
        auto inl = m.second;
        if (inl->invocations.size() == 0) continue;
        auto it = inl->invocations.begin();
        auto first = *it;
        if (inl->invocations.size() > 1) {
            ++it;
            auto second = *it;
            ::error("Multiple invocations of control block not supported on this target: %1%, %2%",
                    first, second);
            continue;
        }
        cg.add(inl->caller, inl->callee);
    }

    // must inline from leaves up
    std::vector<const IR::P4Control*> order;
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

}  // namespace BMV2
