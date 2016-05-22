#include "createBuiltins.h"
#include "ir/ir.h"

namespace P4 {
void CreateBuiltins::postorder(IR::P4Parser* parser) {
    auto newStates = new IR::IndexedVector<IR::ParserState>(*parser->states);
    IR::ParserState* ac = new IR::ParserState(Util::SourceInfo(),
                                              IR::ParserState::accept,
                                              IR::Annotations::empty,
                                              new IR::IndexedVector<IR::StatOrDecl>(),
                                              nullptr);
    IR::ParserState* rj = new IR::ParserState(Util::SourceInfo(),
                                              IR::ParserState::reject,
                                              IR::Annotations::empty,
                                              new IR::IndexedVector<IR::StatOrDecl>(),
                                              nullptr);
    newStates->push_back(ac);
    newStates->push_back(rj);
    parser->states = newStates;
}

void CreateBuiltins::postorder(IR::ActionListElement* element) {
    // convert path expressions to method calls
    // actions = { a; b; }
    // becomes
    // action = { a(); b(); }
    if (element->expression->is<IR::PathExpression>())
        element->expression = new IR::MethodCallExpression(
            element->expression->srcInfo, element->expression,
            new IR::Vector<IR::Type>(), new IR::Vector<IR::Expression>());
}

void CreateBuiltins::postorder(IR::ExpressionValue* expression) {
    // convert a default_action = a; into
    // default_action = a();
    auto prop = findContext<IR::TableProperty>();
    if (prop != nullptr &&
        prop->name == IR::TableProperties::defaultActionPropertyName &&
        expression->expression->is<IR::PathExpression>())
        expression->expression = new IR::MethodCallExpression(
            expression->expression->srcInfo, expression->expression,
            new IR::Vector<IR::Type>(), new IR::Vector<IR::Expression>());
}

}  // namespace P4
