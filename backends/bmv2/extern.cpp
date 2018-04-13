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

Util::JsonArray*
Extern::addExternAttributes(const IR::Declaration_Instance*,  // TODO: Unused param, may be removed
                            const IR::ExternBlock* block) {
    auto attributes = new Util::JsonArray();
    for (auto p : *block->getConstructorParameters()) {
        auto name = p->name;
        auto pVal = block->getParameterValue(name);
        if (pVal->is<IR::Constant>()) {
            auto cVal = pVal->to<IR::Constant>();
            if (p->type->is<IR::Type_Bits>()) {
                json->add_extern_attribute(name, "hexstr",
                                           stringRepr(cVal->value), attributes);
            } else {
                BUG("%1%: unhandled constant constructor param", cVal->toString());
            }
        } else if (pVal->is<IR::Declaration_ID>()) {
            auto declId = pVal->to<IR::Declaration_ID>();
            json->add_extern_attribute(name, "string", declId->name, attributes);
        } else if (pVal->is<IR::Type_Enum>()) {
            json->add_extern_attribute(name, "string", pVal->toString(), attributes);
        } else {
            BUG("%1%: unknown constructor param type", p->type);
        }
    }
    return attributes;
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
            auto name = decl->name;
            auto type = "";
            if (decl->type->is<IR::Type_Specialized>())
                type = decl->type->to<IR::Type_Specialized>()->baseType->toString();
            else if (decl->type->is<IR::Type_Name>())
                type = decl->type->to<IR::Type_Name>()->path->name.toString();
            else
                P4C_UNIMPLEMENTED("extern support for %1%", decl);
            auto attributes = addExternAttributes(decl, externBlock);
            json->add_extern(name, type, attributes);
        } else {
            BUG("%1% Unsupported block type for extern generation.", block->toString());
        }
    }
    return false;
}

/// Custom visitor to enable traversal on other blocks
bool Extern::preorder(const IR::PackageBlock *block) {
    if (backend->target != Target::PORTABLE_SWITCH && !emitExterns)
        return false;

    for (auto it : block->constantValue) {
        if (it.second->is<IR::Block>()) {
            visit(it.second->getNode());
        }
    }
    return false;
}


}  // namespace BMV2
