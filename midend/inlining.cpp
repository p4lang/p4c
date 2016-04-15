#include "inlining.h"
#include "frontends/p4/callGraph.h"
#include "frontends/p4/methodInstance.h"

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
        cg.add(inl->caller, inl->callee);
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
    LOG1("Inline controls driver");
    const IR::P4Program* prog = program;
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

}  // namespace P4
