#include "ir.h"

cstring IR::IDeclaration::externName() const {
    if (auto annot = to<IR::IAnnotated>()) {
        if (auto a = annot->getAnnotations()->getSingle(IR::Annotation::nameAnnotation)) {
            CHECK_NULL(a->expr);
            auto str = a->expr->to<IR::StringLiteral>();
            CHECK_NULL(str);
            return str->value; } }
    return getName().name;
}
