#include "typeMap.h"
#include "lib/map.h"

namespace P4 {

const IR::Type_InfInt* TypeMap::canonInfInt = new IR::Type_InfInt();

void TypeMap::dbprint(std::ostream& out) const {
    out << "TypeMap for " << dbp(program);
    for (auto it : typeMap)
        out << dbp(it.first) << "->" << dbp(it.second) << std::endl;
}

void TypeMap::setLeftValue(const IR::Expression* expression) {
    leftValues.insert(expression);
    LOG1("Left value " << dbp(expression));
}

void TypeMap::clear() {
    LOG1("Clearing typeMap");
    typeMap.clear(); leftValues.clear(); program = nullptr;
}

void TypeMap::checkPrecondition(const IR::Node* element, const IR::Type* type) const {
    if (element == nullptr)
        BUG("Null element in setType");
    if (type == nullptr)
        BUG("Null type in setType");
    if (type->is<IR::Type_Name>())
        BUG("Element %1% maps to a Type_Name %2%", dbp(element), dbp(type));
}

void TypeMap::setType(const IR::Node* element, const IR::Type* type) {
    checkPrecondition(element, type);
    auto it = typeMap.find(element);
    if (it != typeMap.end()) {
        const IR::Type* existingType = it->second;
        if (existingType != type)
            BUG("Changing type of %1% in type map from %2% to %3%",
                dbp(element), dbp(existingType), dbp(type));
        return;
    }
    LOG1("setType " << dbp(element) << " => " << dbp(type));
    typeMap.emplace(element, type);
}

const IR::Type* TypeMap::getType(const IR::Node* element, bool notNull) const {
    CHECK_NULL(element);
    auto result = get(typeMap, element);
    LOG2("Looking up type for " << dbp(element) << " => " << dbp(result));
    if (notNull && result == nullptr) {
        std::cout << this;
        BUG("Could not find type for %1%", dbp(element));
    }
    if (result != nullptr && result->is<IR::Type_Name>())
        BUG("%1% in map", dbp(result));
    return result;
}

}  // namespace P4
