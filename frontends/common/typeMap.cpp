#include "typeMap.h"
#include "lib/map.h"

namespace P4 {

std::map<int, const IR::Type_Bits*> TypeMap::signedTypes;
std::map<int, const IR::Type_Bits*> TypeMap::unsignedTypes;
const IR::Type_InfInt* TypeMap::canonInfInt = new IR::Type_InfInt();

void TypeMap::dbprint(std::ostream& out) const {
    for (auto it : typeMap)
        out << it.first << "->" << it.second << std::endl;
}

void TypeMap::setLeftValue(const IR::Expression* expression) {
    leftValues.insert(expression);
    LOG1("Left value " << expression);
}

void TypeMap::clear() {
    LOG1("Clearing typeMap");
    typeMap.clear(); leftValues.clear();
}

void TypeMap::checkPrecondition(const IR::Node* element, const IR::Type* type) const {
    if (element == nullptr)
        BUG("Null element in setType");
    if (type == nullptr)
        BUG("Null type in setType");
    if (type->is<IR::Type_Name>())
        BUG("Element %1% maps to a Type_Name %2%", element, type);
}

void TypeMap::setType(const IR::Node* element, const IR::Type* type) {
    checkPrecondition(element, type);
    auto it = typeMap.find(element);
    if (it != typeMap.end()) {
        const IR::Type* existingType = it->second;
        if (existingType != type)
            BUG("Changing type of %1% in type map from %2% to %3%",
                element, existingType, type);
        return;
    }
    LOG1("setType " << element << " => " << type);
    typeMap.emplace(element, type);
}

const IR::Type* TypeMap::getType(const IR::Node* element, bool notNull) const {
    CHECK_NULL(element);
    auto result = get(typeMap, element);
    LOG2("Looking up type for " << element << " => " << result);
    if (notNull && result == nullptr)
        BUG("Could not find type for %1%", element);
    if (result != nullptr && result->is<IR::Type_Name>())
        BUG("%1% in map", result);
    return result;
}

const IR::Type_Bits* TypeMap::canonicalType(unsigned width, bool isSigned) {
    std::map<int, const IR::Type_Bits*> *map;
    if (isSigned)
        map = &signedTypes;
    else
        map = &unsignedTypes;

    auto it = map->find(width);
    if (it != map->end())
        return it->second;
    auto result = new IR::Type_Bits(Util::SourceInfo(), width, isSigned);
    map->emplace(width, result);
    return result;
}

}  // namespace P4
