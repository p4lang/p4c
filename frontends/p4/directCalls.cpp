#include "directCalls.h"

#include <ostream>

#include "ir/declaration.h"
#include "ir/id.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/log.h"

namespace P4 {

const IR::Node *DoInstantiateCalls::postorder(IR::P4Parser *parser) {
    parser->parserLocals.append(insert);
    insert.clear();
    return parser;
}

const IR::Node *DoInstantiateCalls::postorder(IR::P4Control *control) {
    control->controlLocals.append(insert);
    insert.clear();
    return control;
}

const IR::Node *DoInstantiateCalls::postorder(IR::MethodCallExpression *expression) {
    // Identify type.apply(...) methods
    auto mem = expression->method->to<IR::Member>();
    if (mem == nullptr) return expression;
    auto tn = mem->expr->to<IR::TypeNameExpression>();
    if (tn == nullptr) return expression;

    const IR::Type_Name *tname;
    if (auto ts = tn->typeName->to<IR::Type_Specialized>()) {
        tname = ts->baseType;
    } else {
        tname = tn->typeName->to<IR::Type_Name>();
    }
    CHECK_NULL(tname);
    auto ref = refMap->getDeclaration(tname->path, true);
    if (!ref->is<IR::P4Control>() && !ref->is<IR::P4Parser>()) return expression;

    auto name = refMap->newName(tname->path->name + "_inst");
    LOG3("Inserting instance " << name);
    auto annos = new IR::Annotations();
    annos->add(new IR::Annotation(IR::Annotation::nameAnnotation, tname->path->toString()));
    auto inst = new IR::Declaration_Instance(expression->srcInfo, IR::ID(name), annos,
                                             tn->typeName->clone(), new IR::Vector<IR::Argument>());
    insert.push_back(inst);

    auto path = new IR::PathExpression(expression->srcInfo,
                                       new IR::Path(IR::ID(expression->srcInfo, name)));
    expression->method = new IR::Member(path, mem->member);
    return expression;
}

}  // namespace P4
