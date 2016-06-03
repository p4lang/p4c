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
    auto structure = ::get(parsers->parsers, getOriginal<IR::P4Parser>());
    CHECK_NULL(structure);
    structure->evaluate();
    return parser;
}

void ParserStructure::evaluate() {
    auto initial = new ValueMap();
    auto work = new Work(initial, start, nullptr);
    toRun.push_back(work);
    run();
}

void ParserStructure::run() {
    while (!toRun.empty()) {
        auto work = toRun.back();
        toRun.pop_back();
        auto after = work->evaluate();
        if (after == nullptr) continue;

        auto callees = callGraph.getCallees(work->state);
        for (auto c : *callees) {
            auto newWork = new Work(after->clone(), c, work);
            toRun.push_back(newWork);
        }
    }
}

ValueMap* ParserStructure::Work::evaluate() const {
    for (auto sd : *state->components)
        evaluate(sd);
    return valueMap;
}

void ParserStructure::Work::evaluate(const IR::StatOrDecl* sord) const {
    if (sord->is<IR::AssignmentStatement>()) {
        // TODO
    } else if (sord->is<IR::MethodCallStatement>()) {
        // TODO
    }
    BUG("%1%: unexpected declaration or statement", sord);
}

}  // namespace P4
