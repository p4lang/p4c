#include "convertEnums.h"
#include "frontends/p4/enumInstance.h"

namespace P4 {

const IR::Node* DoConvertEnums::preorder(IR::Type_Enum* type) {
    bool convert = policy->convert(type);
    if (!convert)
        return type;
    unsigned long long count = type->members.size();
    unsigned long long width = policy->enumSize(count);
    LOG2("Converting enum " << type->name << " to " << "bit<" << width << ">");
    BUG_CHECK(count <= (1ULL << width),
              "%1%: not enough bits to represent %2%", width, type);
    auto r = new EnumRepresentation(type->srcInfo, width);
    auto canontype = typeMap->getTypeType(getOriginal(), true);
    BUG_CHECK(canontype->is<IR::Type_Enum>(),
              "canon type of enum %s is non enum %s?", type, canontype);
    repr.emplace(canontype->to<IR::Type_Enum>(), r);
    for (auto d : type->members)
        r->add(d->name.name);
    return type;
}

const IR::Node* DoConvertEnums::postorder(IR::Type_Name* type) {
    auto canontype = typeMap->getTypeType(getOriginal(), true);
    if (!canontype->is<IR::Type_Enum>())
        return type;
    if (findContext<IR::TypeNameExpression>() != nullptr)
        // This will be resolved by the caller.
        return type;
    auto enumType = canontype->to<IR::Type_Enum>();
    auto r = ::get(repr, enumType);
    if (r == nullptr)
        return type;
    return r->type;
}

/// process enum expression, e.g., X.a
const IR::Node* DoConvertEnums::postorder(IR::Member* expression) {
    auto ei = EnumInstance::resolve(getOriginal<IR::Member>(), typeMap);
    if (!ei)
        return expression;
    if (ei->is<SimpleEnumInstance>()) {
        auto r = ::get(repr, ei->type->to<IR::Type_Enum>());
        if (r == nullptr)
            return expression;
        unsigned value = r->get(ei->name.name);
        auto cst = new IR::Constant(expression->srcInfo, r->type, value);
        return cst;
    }
    return expression;
}

}  // namespace P4
