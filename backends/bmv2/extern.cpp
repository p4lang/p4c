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

#include "extern.h"

namespace BMV2 {

/**
    Custom visitor to enable traversal on other blocks
*/
bool Extern::preorder(const IR::PackageBlock *block) {
    for (auto it : block->constantValue) {
        if (it.second->is<IR::Block>()) {
            visit(it.second->getNode());
        }
    }
    return false;
}

void Extern::addExternAttributes(const IR::Declaration_Instance* di,
                                                  const IR::ExternBlock* block,
                                                  Util::JsonArray* attributes) {
    auto paramIt = block->getConstructorParameters()->parameters.begin();
    for (auto arg : *di->arguments) {
        auto j = new Util::JsonObject();
        j->emplace("name", arg->toString());
        if (arg->is<IR::Constant>()) {
            auto cVal = arg->to<IR::Constant>();
            if (arg->type->is<IR::Type_Bits>()) {
                j->emplace("type", "hexstr");
                j->emplace("value", stringRepr(cVal->value));
            } else {
                BUG("%1%: unhandled constant constructor param", cVal->toString());
            }
        } else if (arg->is<IR::Declaration_ID>()) {
            auto declID = arg->to<IR::Declaration_ID>();
            j->emplace("type", "string");
            j->emplace("value", declID->toString());
        } else if (arg->is<IR::Type_Enum>()) {
            j->emplace("type", "string");
            j->emplace("value", arg->toString());
        } else {
            BUG("%1%: unknown constructor param type", arg->type);
        }
        attributes->append(j);
        ++paramIt;
    }
}

/// generate extern_instances from instance declarations.
bool Extern::preorder(const IR::Declaration_Instance* decl) {
    LOG1("ExternConv Visiting ..." << dbp(decl));
    // Declaration_Instance -> P4Control -> ControlBlock
    auto grandparent = getContext()->parent->node;
    if (grandparent->is<IR::ControlBlock>()) {
        auto block = grandparent->to<IR::ControlBlock>()->getValue(decl);
        CHECK_NULL(block);
        if (block->is<IR::ExternBlock>()) {
            auto externBlock = block->to<IR::ExternBlock>();
            auto result = new Util::JsonObject();
            result->emplace("name", decl->name);
            result->emplace("id", nextId("extern_instances"));
            result->emplace("type", decl->type->to<IR::Type_Specialized>()->baseType->toString());
            auto attributes = mkArrayField(result, "attribute_values");
            addExternAttributes(decl, externBlock, attributes);
            externs->append(result);
        }
    }
    return false;
}

} // namespace BMV2
