#include "actionsInlining.h"
#include "frontends/p4/callGraph.h"
#include "frontends/p4/evaluator/substituteParameters.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/parameterSubstitution.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace P4 {

void InlineWorkList::dbprint(std::ostream& out) const {
    for (auto t : sites) {
        out << t.first;
        for (auto c : t.second) {
            out << std::endl << "\t" << c.first << " => " << c.second;
        }
    }
}

void ActionsInlineList::analyze() {
    P4::CallGraph<const IR::ActionContainer*> cg("Actions call-graph");

    for (auto c : toInline)
        cg.add(c->caller, c->callee);

    // must inline from leaves up
    std::vector<const IR::ActionContainer*> order;
    cg.sort(order);
    for (auto c : order) {
        // This is quadratic, but hopefully the call graph is not too large
        for (auto ci : toInline) {
            if (ci->caller == c)
                inlineOrder.push_back(ci);
        }
    }

    std::reverse(inlineOrder.begin(), inlineOrder.end());
}

InlineWorkList* ActionsInlineList::next() {
    if (inlineOrder.size() == 0)
        return nullptr;

    std::set<const IR::ActionContainer*> callers;
    auto result = new InlineWorkList();

    // Find actions that can be inlined simultaneously.
    // This traversal is in topological order starting from leaf callees.
    // We stop at the first action which calls one of the actions
    // we have already selected.
    while (!inlineOrder.empty()) {
        auto last = inlineOrder.back();
        if (callers.find(last->callee) != callers.end())
            break;
        inlineOrder.pop_back();
        result->add(last);
        callers.emplace(last->caller);
    }
    BUG_CHECK(!result->empty(), "Empty list of methods to inline");
    return result;
}

void DiscoverActionsInlining::postorder(const IR::MethodCallStatement* mcs) {
    auto mi = P4::MethodInstance::resolve(mcs, refMap, typeMap);
    CHECK_NULL(mi);
    auto ac = mi->to<P4::ActionCall>();
    if (ac == nullptr)
        return;
    auto caller = findContext<IR::ActionContainer>();
    if (caller == nullptr) {
        if (findContext<IR::ParserContainer>() != nullptr)
            ::error("%1%: action invocation in parser not supported", mcs);
        else if (findContext<IR::ControlContainer>() != nullptr)
            BUG("%1%: direct action invocation not yet implemented", mcs);
        BUG("%1%: unexpected action invocation", mcs);
        return;
    }

    auto aci = new ActionCallInfo(caller, ac->action, mcs);
    toInline->add(aci);
}

class ActionsInliner : public Transform {
    P4::ReferenceMap* refMap;
    ActionsInlineList* list;
    InlineWorkList*    toInline;

    std::map<const IR::MethodCallStatement*, const IR::ActionContainer*>* replMap;

 public:
    ActionsInliner(P4::ReferenceMap* refMap, ActionsInlineList* list,
                         InlineWorkList* toInline) :
            refMap(refMap), list(list), toInline(toInline), replMap(nullptr)
    { CHECK_NULL(refMap); CHECK_NULL(list); CHECK_NULL(toInline); }

    Visitor::profile_t init_apply(const IR::Node* node) {
        LOG1("SimpleActionsInliner " << toInline);
        return Transform::init_apply(node);
    }

    const IR::Node* preorder(IR::ParserContainer* cont) override
    { prune(); return cont; }  // skip
    const IR::Node* preorder(IR::ActionContainer* action) override {
        if (toInline->sites.count(getOriginal<IR::ActionContainer>()) == 0)
            prune();
        replMap = &toInline->sites[getOriginal<IR::ActionContainer>()];
        LOG1("Visiting: " << getOriginal());
        return action;
    }
    const IR::Node* postorder(IR::ActionContainer* action) override {
        if (toInline->sites.count(getOriginal<IR::ActionContainer>()) > 0)
            list->replace(getOriginal<IR::ActionContainer>(), action);
        replMap = nullptr;
        return action;
    }
    const IR::Node* preorder(IR::MethodCallStatement* statement) override {
        LOG1("Visiting " << getOriginal());
        if (replMap == nullptr)
            return statement;

        auto callee = get(*replMap, getOriginal<IR::MethodCallStatement>());
        if (callee == nullptr)
            return statement;

        LOG1("Inlining: " << toInline);
        IR::ParameterSubstitution subst;
        subst.populate(callee->parameters, statement->methodCall->arguments);
        IR::TypeVariableSubstitution tvs;  // empty
        P4::SubstituteParameters sp(refMap, &subst, &tvs);
        auto clone = callee->apply(sp);
        if (::errorCount() > 0)
            return statement;
        CHECK_NULL(clone);
        auto result = clone->to<IR::ActionContainer>()->body;
        LOG1("Replacing " << statement << " with " << result);
        return result;
    }
};

const IR::Node* InlineActionsDriver::preorder(IR::P4Program* program) {
    LOG1("Inline actions driver");
    const IR::P4Program* prog = program;
    P4::ResolveReferences solver(refMap, isv1);

    toInline->analyze();
    while (auto todo = toInline->next()) {
        LOG1("Processing " << todo);
        ActionsInliner si(refMap, toInline, todo);
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

}  // namespace P4
