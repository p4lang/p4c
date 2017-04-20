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

#include "convertErrorCode.h"

namespace BMV2 {

Util::JsonArray* ErrorCodesVisitor::pushNewArray(Util::JsonArray* parent) {
    auto result = new Util::JsonArray();
    parent->append(result);
    return result;
}

bool ErrorCodesVisitor::preorder(const IR::Type_Error* errors) {
    for (auto m : *errors->getDeclarations()) {
        BUG_CHECK(errorCodesMap->find(m) == errorCodesMap->end(), "Duplicate error");
        errorCodesMap->emplace(m, errorCodesMap->size());
    }
    return false;
}

void ErrorCodesVisitor::postorder(const IR::P4Program* program) {
    for (const auto &p : *errorCodesMap) {
        auto entry = new Util::JsonArray();
        auto name = p.first->getName().name.c_str();
        entry->append(name);
        entry->append(p.second);
        errors->append(entry);
    }
}

} // namespace BMV2
