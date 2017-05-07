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

            // auto fields = create_header_fields(st);
            // bm->add_header_type(type, name, fields);
        }
    }

    for (auto f : type->fields) {
        LOG1("fields " << f);
        auto ft = backend->getTypeMap()->getType(f, true);
        if (ft->is<IR::Type_StructLike>()) {
            LOG1("add to header " << ft);
            LOG1("create header");
            auto json = new Util::JsonObject();
            json->emplace("name", extVisibleName(f));
            json->emplace("id", nextId("headers"));
            json->emplace_non_null("source_info", f->sourceInfoJsonObj());
            json->emplace("header_type", extVisibleName(ft->to<IR::Type_StructLike>()));
            json->emplace("metadata", meta);
            backend->headerInstances->append(json);
            backend->headerInstancesCreated.insert(ft);
            // auto name = extVisibleName(f);
            // auto type = extVisibleName(ft->to<IR::Type_StructLike>());
            // bm->add_header(type, name);
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
        LOG1("Creating " << stack);
        if (stack == nullptr)
            continue;

        auto json = new Util::JsonObject();
        json->emplace("name", extVisibleName(f));
        json->emplace("id", nextId("stack"));
        json->emplace_non_null("source_info", f->sourceInfoJsonObj());
        json->emplace("size", stack->getSize());
        auto type = backend->getTypeMap()->getTypeType(stack->elementType, true);
        BUG_CHECK(type->is<IR::Type_Header>(), "%1% not a header type", stack->elementType);
        auto ht = type->to<IR::Type_Header>();
        backend->createJsonType(ht);

        cstring header_type = extVisibleName(stack->elementType->to<IR::Type_Header>());
        json->emplace("header_type", header_type);
        auto stackMembers = mkArrayField(json, "header_ids");
        for (unsigned i=0; i < stack->getSize(); i++) {
            unsigned id = nextId("headers");
            stackMembers->append(id);
            auto header = new Util::JsonObject();
            cstring name = extVisibleName(f) + "[" + Util::toString(i) + "]";
            // replace
            header->emplace("name", name);
            header->emplace("id", id);
            header->emplace("header_type", header_type);
            header->emplace("metadata", false);
            backend->headerInstances->append(header);
            //bm->json->add_header(header_type, name, false);
        }
        backend->headerStacks->append(json);

/*
        auto name = extVisibleName(f);
        auto size = stack->getSize();
        auto elem = stack->elementType;
        auto elemType = typeMap.getTypeType(elem, true);
        bm->add_header_type(elemType);
        cstring hdrType = extVisibleName(elemType->to<IR::TypeHeader>());
        std::vector<unsigned> ids;
        for (unsigned i = 0; i < size; i++) {
            cstring hdrName = extVisibleName(f) + "[" + Util::toString(i) + "]";
            auto id = bm->json->add_header(hdrName, hdrType, false);
            ids.append(id);
        }
        bm->json->add_header_stack(name, headerType, size, ids);
*/

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
                LOG1("add stack" << st);
                addHeaderStacks(st);
            } else {
                LOG1("add metadata" << st);
                addTypesAndInstances(st, true);
            }
        }
    }
    return false;
}

}  // namespace BMV2

