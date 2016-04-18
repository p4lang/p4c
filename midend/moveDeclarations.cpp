#include "moveDeclarations.h"

namespace P4 {

const IR::Node* MoveDeclarations::postorder(IR::P4Action* action)  {
    auto body = new IR::Vector<IR::StatOrDecl>();
    body->insert(body->end(), toMove.begin(), toMove.end());
    body->append(*action->body);
    auto result = new IR::P4Action(action->srcInfo, action->name, action->annotations,
                                   action->parameters, body);
    return result;
}

const IR::Node* MoveDeclarations::postorder(IR::P4Control* control)  {
    auto bodyContents = new IR::Vector<IR::StatOrDecl>();
    bodyContents->insert(bodyContents->end(), toMove.begin(), toMove.end());
    bodyContents->append(*control->body->components);
    auto body = new IR::BlockStatement(control->body->srcInfo, bodyContents);
    auto result = new IR::P4Control(control->srcInfo, control->name, control->type,
                                    control->constructorParams, control->stateful, body);
    return result;
}

const IR::Node* MoveDeclarations::postorder(IR::P4Parser* parser)  {
    auto newStateful = new IR::Vector<IR::Declaration>();
    newStateful->append(toMove);
    newStateful->append(*parser->stateful);
    auto result = new IR::P4Parser(parser->srcInfo, parser->name, parser->type,
                                   parser->constructorParams, newStateful,
                                   parser->states);
    return result;
}

const IR::Node* MoveDeclarations::postorder(IR::Declaration_Variable* decl) {
    // We must keep the initializer here
    if (decl->initializer != nullptr) {
        auto move = new IR::Declaration_Variable(decl->srcInfo, decl->name,
                                                 decl->annotations, decl->type, nullptr);
        toMove.push_back(move);
        auto varRef = new IR::PathExpression(decl->name);
        auto keep = new IR::AssignmentStatement(decl->srcInfo, varRef, decl->initializer);
        return keep;
    } else {
        toMove.push_back(decl);
        return nullptr;
    }
}

const IR::Node* MoveDeclarations::postorder(IR::Declaration_Constant* decl) {
    if (findContext<IR::P4Control>() == nullptr &&
        findContext<IR::P4Action>() == nullptr &&
        findContext<IR::P4Parser>() == nullptr)
        // This is a global declaration
        return decl;
    toMove.push_back(decl);
    return nullptr;
}

}  // namespace P4
