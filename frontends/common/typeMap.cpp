/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "typeMap.h"
#include "lib/map.h"

namespace P4 {

void TypeMap::dbprint(std::ostream& out) const {
    out << "TypeMap for " << dbp(program) << std::endl;
    for (auto it : typeMap)
        out << "\t" << dbp(it.first) << "->" << dbp(it.second) << std::endl;
    out << "Left values" << std::endl;
    for (auto it : leftValues)
        out << "\t" << dbp(it) << std::endl;
    out << "Constants" << std::endl;
    for (auto it : constants)
        out << "\t" << dbp(it) << std::endl;
    out << "--------------" << std::endl;
}

void TypeMap::setLeftValue(const IR::Expression* expression) {
    leftValues.insert(expression);
    LOG1("Left value " << dbp(expression));
}

void TypeMap::setCompileTimeConstant(const IR::Expression* expression) {
    constants.insert(expression);
    LOG1("Constant value " << dbp(expression));
}

bool TypeMap::isCompileTimeConstant(const IR::Expression* expression) const {
    bool result = constants.find(expression) != constants.end();
    LOG1(dbp(expression) << (result ? " constant" : " not constant"));
    return result;
}

void TypeMap::clear() {
    LOG1("Clearing typeMap");
    typeMap.clear(); leftValues.clear(); constants.clear(); program = nullptr;
}

void TypeMap::checkPrecondition(const IR::Node* element, const IR::Type* type) const {
    CHECK_NULL(element); CHECK_NULL(type);
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
        // std::cout << this;
        BUG("Could not find type for %1%", dbp(element));
    }
    if (result != nullptr && result->is<IR::Type_Name>())
        BUG("%1% in map", dbp(result));
    return result;
}

}  // namespace P4
