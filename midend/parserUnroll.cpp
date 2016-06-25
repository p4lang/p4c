#include "parserUnroll.h"

namespace P4 {

void ParserAnalyzer::postorder(const IR::PathExpression* expression) {
    auto parser = findContext<IR::P4Parser>();
    if (parser == nullptr)
        return;
    auto state = findContext<IR::ParserState>();
    if (state == nullptr)
        return;
    auto decl = refMap->getDeclaration(expression->path);
    if (!decl->is<IR::ParserState>())
        return;
    CHECK_NULL(current);
    current->callGraph.add(state, decl->to<IR::ParserState>());
}

const IR::Node* ParserEvaluator::postorder(IR::P4Parser* parser) {
    LOG1("Unrolling " << parser);
    auto structure = ::get(parsers->parsers, getOriginal<IR::P4Parser>());
    CHECK_NULL(structure);
    structure->evaluate(refMap, typeMap);
    return parser;
}

ValueMap* ParserStructure::initializeVariables(TypeMap* typeMap) {
    ValueMap* result = new ValueMap();
    ValueFactory factory(typeMap);
    for (auto p : *parser->type->applyParams->parameters) {
        if (p->direction == IR::Direction::None)
            continue;
        auto type = typeMap->getType(p);
        bool initialized = p->direction == IR::Direction::In ||
                p->direction == IR::Direction::InOut;
        auto value = factory.create(type, !initialized);
        result->set(p, value);
    }
    for (auto d : *parser->stateful) {
        if (!d->is<IR::Declaration_Variable>())
            continue;
        auto type = typeMap->getType(d);
        auto value = factory.create(type, true);
        result->set(d, value);
    }
    return result;
}

void ParserStructure::evaluate(ReferenceMap* refMap, TypeMap* typeMap) {
    auto initial = initializeVariables(typeMap);
    auto work = new Work(initial, start, nullptr);
    toRun.push_back(work);
    run(refMap, typeMap);
}

void ParserStructure::run(ReferenceMap* refMap, TypeMap* typeMap) {
    LOG1("Process unrolling " << parser);
    while (!toRun.empty()) {
        auto work = toRun.back();
        toRun.pop_back();
        auto after = work->evaluate(refMap, typeMap);
        if (after == nullptr) continue;

        auto callees = callGraph.getCallees(work->state);
        if (callees == nullptr)
            continue;
        for (auto c : *callees) {
            auto newWork = new Work(after->clone(), c, work);
            toRun.push_back(newWork);
        }
    }
}

ValueMap* ParserStructure::Work::evaluate(ReferenceMap* refMap, TypeMap* typeMap) const {
    LOG1("Analyzing " << state);
    for (auto sd : *state->components)
        evaluate(sd, refMap, typeMap);
    return valueMap;
}

void ParserStructure::Work::evaluate(const IR::StatOrDecl* sord, ReferenceMap* refMap,
                                     TypeMap* typeMap) const {
    ExpressionEvaluator ev(refMap, typeMap, valueMap);

    if (sord->is<IR::AssignmentStatement>()) {
        auto ass = sord->to<IR::AssignmentStatement>();
        auto left = ev.evaluate(ass->left);
        auto right = ev.evaluate(ass->right);
        left->assign(right);
    } else if (sord->is<IR::MethodCallStatement>()) {
        // can have side-effects
        ev.evaluate(sord->to<IR::MethodCallStatement>()->methodCall);
    } else {
        BUG("%1%: unexpected declaration or statement", sord);
    }
    LOG2("After " << sord << " state is " << valueMap);
}

}  // namespace P4
