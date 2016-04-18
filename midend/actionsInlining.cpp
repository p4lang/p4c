#include "actionsInlining.h"
#include "frontends/p4/callGraph.h"
#include "frontends/p4/evaluator/substituteParameters.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/parameterSubstitution.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/toP4/toP4.h"
#include "lib/nullstream.h"

namespace P4 {

void AInlineWorkList::dbprint(std::ostream& out) const {
    for (auto t : sites) {
        out << t.first;
        for (auto c : t.second) {
            out << std::endl << "\t" << c.first << " => " << c.second;
        }
    }
}

void ActionsInlineList::analyze() {
    P4::CallGraph<const IR::P4Action*> cg("Actions call-graph");

    for (auto c : toInline)
        cg.add(c->caller, c->callee);

    // must inline from leaves up
    std::vector<const IR::P4Action*> order;
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

AInlineWorkList* ActionsInlineList::next() {
    if (inlineOrder.size() == 0)
        return nullptr;

    std::set<const IR::P4Action*> callers;
    auto result = new AInlineWorkList();

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
    auto caller = findContext<IR::P4Action>();
    if (caller == nullptr) {
        if (findContext<IR::P4Parser>() != nullptr) {
            ::error("%1%: action invocation in parser not supported", mcs);
        } else if (findContext<IR::P4Control>() != nullptr) {
            if (!allowDirectActionCalls)
                BUG("%1%: direct action invocation not yet implemented", mcs);
        } else {
            BUG("%1%: unexpected action invocation", mcs);
        }
        return;
    }

    auto aci = new ActionCallInfo(caller, ac->action, mcs);
    toInline->add(aci);
}

const IR::Node* InlineActionsDriver::preorder(IR::P4Program* program) {
    LOG1("Inline actions driver");
    const IR::P4Program* prog = program;
    toInline->analyze();
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

}  // namespace P4
