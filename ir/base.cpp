#include "ir.h"

namespace IR {

cstring IDeclaration::externalName() const {
    if (!is<IAnnotated>())
        return getName();
    
    auto anno = to<IAnnotated>()->getAnnotations()->getSingle(IR::Annotation::nameAnnotation);
    if (anno != nullptr) {
        if (anno->expr == nullptr) {
            ::error("%1% should contain a string", anno);
        } else {
            auto str = anno->expr->to<IR::StringLiteral>();
            if (str == nullptr)
                ::error("%1% should contain a string", anno);
            else
                return str->value;
        }
    }
    return getName();
}

}
