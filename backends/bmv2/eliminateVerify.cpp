#include "eliminateVerify.h"
#include "frontends/p4/methodInstance.h"

namespace BMV2 {

// Convert verify(b, e)
// into
// transition select(b) {
//    false : reject;
//    true  : continuation;
// }
// The error is just ignored - BMv2 does not expose it
const IR::Node* EliminateVerify::postorder(IR::ParserState* state) {
    // This code is very similar to RemoveParserControlFlow
    LOG1("Visiting " << dbp(state));
    // Set of newly created states
    auto states = new IR::IndexedVector<IR::ParserState>();
    IR::ParserState* currentState = state;
    // components of the currentState
    auto currentComponents = new IR::IndexedVector<IR::StatOrDecl>();
    auto origComponents = state->components;
    auto origSelect = state->selectExpression;

    cstring joinName;  // non-empty if we split the state
    for (auto c : *origComponents) {
        if (c->is<IR::MethodCallStatement>()) {
            auto mcs = c->to<IR::MethodCallStatement>();
            auto mi = P4::MethodInstance::resolve(mcs, refMap, typeMap);
            auto ef = mi->to<P4::ExternFunction>();
            if (ef != nullptr &&
                ef->method->name.name == IR::ParserState::verify) {
                LOG1("Converting " << dbp(c) << " into states");

                states->push_back(currentState);
                BUG_CHECK(mcs->methodCall->arguments->size() == 2, "%1%: Expected 2 arguments", mcs);
                auto cond = mcs->methodCall->arguments->at(0);
                cstring joinName = refMap->newName(state->name.name + "_join");

                auto vec = new IR::Vector<IR::Expression>();
                vec->push_back(cond);
                auto trueCase = new IR::SelectCase(
                    Util::SourceInfo(), new IR::BoolLiteral(true),
                    new IR::PathExpression(IR::ID(joinName)));
                auto falseCase = new IR::SelectCase(
                    Util::SourceInfo(), new IR::BoolLiteral(false),
                    new IR::PathExpression(IR::ID(IR::ParserState::reject)));
                auto cases = new IR::Vector<IR::SelectCase>();
                cases->push_back(trueCase);
                cases->push_back(falseCase);
                currentState->selectExpression = new IR::SelectExpression(
                    Util::SourceInfo(), new IR::ListExpression(vec), std::move(*cases));

                currentState->components = currentComponents;
                currentComponents = new IR::IndexedVector<IR::StatOrDecl>();
                currentState = new IR::ParserState(
                    Util::SourceInfo(), joinName, IR::Annotations::empty,
                    currentComponents, origSelect);  // may be overriten
            }
        } else {
            currentComponents->push_back(c);
        }
    }

    if (states->empty())
        return state;
    states->push_back(currentState);
    return states;
}

}  // namespace BMV2
