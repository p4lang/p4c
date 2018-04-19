#include "directCalls.h"

namespace P4 {

const IR::Node* DoInstantiateCalls::postorder(IR::P4Parser* parser) {
    insert.append(parser->parserLocals);
    parser->parserLocals = insert;
    insert.clear();
    return parser;
}

const IR::Node* DoInstantiateCalls::postorder(IR::P4Control* control) {
    insert.append(control->controlLocals);
    control->controlLocals = insert;
    insert.clear();
    return control;
}

const IR::Node* DoInstantiateCalls::postorder(IR::MethodCallExpression* expression) {
    // Identify type.apply(...) methods
    auto mem = expression->method->to<IR::Member>();
    if (mem == nullptr)
        return expression;
    auto tn = mem->expr->to<IR::TypeNameExpression>();
    if (tn == nullptr)
        return expression;

    auto ref = refMap->getDeclaration(tn->typeName->path, true);
    if (!ref->is<IR::P4Control>() && !ref->is<IR::P4Parser>()) {
        ::error("%1%: cannot invoke method of %2%", expression, ref);
        return expression;
    }

    auto name = refMap->newName(tn->typeName->path->name + "_inst");
    LOG3("Inserting instance " << name);
    auto annos = new IR::Annotations();
    annos->add(new IR::Annotation(IR::Annotation::nameAnnotation,
                                  { new IR::StringLiteral(tn->typeName->path->toString()) }));
    auto inst = new IR::Declaration_Instance(
        expression->srcInfo, IR::ID(name), annos,
        tn->typeName->clone(), new IR::Vector<IR::Argument>());
    insert.push_back(inst);

    auto path = new IR::PathExpression(expression->srcInfo,
                                       new IR::Path(IR::ID(expression->srcInfo, name)));
    expression->method = new IR::Member(path, mem->member);
    return expression;
}

}  // namespace P4
