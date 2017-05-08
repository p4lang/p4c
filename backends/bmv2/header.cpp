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

#include "ir/ir.h"
#include "backend.h"
#include "header.h"

namespace BMV2 {

Util::JsonArray* ConvertHeaders::pushNewArray(Util::JsonArray* parent) {
    auto result = new Util::JsonArray();
    parent->append(result);
    return result;
}

/**
    Blocks are not in IR tree, use a custom visitor to traverse
*/
bool ConvertHeaders::preorder(const IR::PackageBlock *block) {
    for (auto it : block->constantValue) {
        if (it.second->is<IR::Block>()) {
            visit(it.second->getNode());
        }
    }
    return false;
}

/**
 * This function handles one layer of nested struct.
 */
void ConvertHeaders::addTypesAndInstances(const IR::Type_StructLike* type, bool meta) {
    LOG1("Adding " << type);
    for (auto f : type->fields) {
        auto ft = backend->getTypeMap()->getType(f, true);
        if (ft->is<IR::Type_StructLike>()) {
            // The headers struct can not contain nested structures.
            if (!meta && !ft->is<IR::Type_Header>()) {
                ::error("Type %1% should only contain headers or header stacks", type);
                return;
            }
            auto st = ft->to<IR::Type_StructLike>();
            backend->createJsonType(st);
        }
    }

    for (auto f : type->fields) {
        auto ft = backend->getTypeMap()->getType(f, true);
        if (ft->is<IR::Type_StructLike>()) {
            backend->headerInstancesCreated.insert(ft);
            auto header_name = extVisibleName(f);
            auto header_type = extVisibleName(ft->to<IR::Type_StructLike>());
            if (meta == true) {
                backend->bm->add_metadata(header_type, header_name);
            } else {
                backend->bm->add_header(header_type, header_name);
            }
        } else if (ft->is<IR::Type_Stack>()) {
            // Done elsewhere
            LOG1("stack generation done elsewhere");
            continue;
        } else {
            // Treat this field like a scalar local variable
            auto scalarFields = backend->scalarsStruct->get("fields")->to<Util::JsonArray>();
            CHECK_NULL(scalarFields);
            // TODO(hanw): avoid modifying refMap
            cstring newName = backend->getRefMap()->newName(type->getName() + "." + f->name);
            if (ft->is<IR::Type_Bits>()) {
                auto tb = ft->to<IR::Type_Bits>();
                auto field = pushNewArray(scalarFields);
                field->append(newName);
                field->append(tb->size);
                field->append(tb->isSigned);
                backend->scalars_width += tb->size;
                LOG1("insert field " << f);
                backend->scalarMetadataFields.emplace(f, newName);
                // scalars->add_field();
            } else if (ft->is<IR::Type_Boolean>()) {
                auto field = pushNewArray(scalarFields);
                field->append(newName);
                field->append(backend->boolWidth);
                field->append(0);
                backend->scalars_width += backend->boolWidth;
                LOG1("insert field " << f);
                backend->scalarMetadataFields.emplace(f, newName);
                // scalars->add_field();
            } else {
                BUG("%1%: Unhandled type for %2%", ft, f);
            }
        }
    }
}

void ConvertHeaders::addHeaderStacks(const IR::Type_Struct* headersStruct) {
    LOG1("Creating stack " << headersStruct);
    for (auto f : headersStruct->fields) {
        auto ft = backend->getTypeMap()->getType(f, true);
        auto stack = ft->to<IR::Type_Stack>();
        if (stack == nullptr)
            continue;
        auto stack_name = extVisibleName(f); // name
        auto stack_size = stack->getSize();  // size
        auto type = backend->getTypeMap()->getTypeType(stack->elementType, true);
        BUG_CHECK(type->is<IR::Type_Header>(), "%1% not a header type", stack->elementType);
        auto ht = type->to<IR::Type_Header>();
        backend->createJsonType(ht);
        cstring stack_type = extVisibleName(stack->elementType->to<IR::Type_Header>());
        std::vector<unsigned> ids;
        for (unsigned i = 0; i < stack_size; i++) {
            cstring hdrName = extVisibleName(f) + "[" + Util::toString(i) + "]";
            auto id = backend->bm->add_header(stack_type, hdrName);
            ids.push_back(id);
        }
        backend->bm->add_header_stack(stack_type, stack_name, stack_size, ids);
    }
}

/// TODO(hanw): only check immediate struct fields.
bool ConvertHeaders::isHeaders(const IR::Type_StructLike* st) {
    bool result = false;
    for (auto f : st->fields) {
        if (f->type->is<IR::Type_Header>() || f->type->is<IR::Type_Stack>()) {
            result = true;
        }
    }
    return result;
}

bool ConvertHeaders::checkNestedStruct(const IR::Type_Struct* st) {
    bool result = false;
    for (auto f : st->fields) {
        if (f->type->is<IR::Type_Struct>()) {
            result = true;
        }
    }
    return result;
}

/**
 * Generate json for header from IR::Block's constructor parameters
 *
 * The only allowed fields in a struct are: Type_Bits, Type_Bool and Type_Header
 *
 * header H {
 *   bit<32> x;
 * }
 *
 * struct {
 *   bit<32> x;
 *   bool    y;
 *   H       h;
 * }
 *
 * Type_Struct within a Type_Struct, i.e. nested struct, should be flattened apriori.
 *
 * @pre assumes no nested struct in parameters.
 * @post none
 */
bool ConvertHeaders::preorder(const IR::Parameter* param) {
    //// keep track of which headers we've already generated the json for
    auto ft = backend->getTypeMap()->getType(param->getNode(), true);
    if (ft->is<IR::Type_Struct>()) {
        auto st = ft->to<IR::Type_Struct>();
        if (visitedHeaders.find(st->getName()) != visitedHeaders.end())
            return false;  // already seen
        else
            visitedHeaders.emplace(st->getName());

        // BUG_CHECK(!checkNestedStruct(st), "%1% nested struct not implemented", st->getName());

        if (st->getAnnotation("metadata")) {
            backend->createJsonType(st);
        } else {
            auto isHeader = isHeaders(st);
            if (isHeader) {
                addTypesAndInstances(st, false);
                addHeaderStacks(st);
            } else {
                addTypesAndInstances(st, true);
            }
        }
    }
    return false;
}

}  // namespace BMV2

