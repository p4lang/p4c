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

// TODO(hanw): remove
Util::JsonArray* ConvertHeaders::pushNewArray(Util::JsonArray* parent) {
    auto result = new Util::JsonArray();
    parent->append(result);
    return result;
}

/**
 * This function handles one layer of nested struct.
 */
void ConvertHeaders::addTypesAndInstances(const IR::Type_StructLike* type, bool meta) {
    LOG1("Adding " << type);
    for (auto f : type->fields) {
        auto ft = typeMap->getType(f, true);
        if (ft->is<IR::Type_StructLike>()) {
            // The headers struct can not contain nested structures.
            if (!meta && !ft->is<IR::Type_Header>()) {
                ::error("Type %1% should only contain headers or header stacks", type);
                return;
            }
            auto st = ft->to<IR::Type_StructLike>();
            addHeaderType(st);
        }
    }

    for (auto f : type->fields) {
        auto ft = typeMap->getType(f, true);
        if (ft->is<IR::Type_StructLike>()) {
            auto header_name = extVisibleName(f);
            auto header_type = extVisibleName(ft->to<IR::Type_StructLike>());
            if (meta == true) {
                json->add_metadata(header_type, header_name);
            } else {
                json->add_header(header_type, header_name);
            }
        } else if (ft->is<IR::Type_Stack>()) {
            // Done elsewhere
            LOG1("stack generation done elsewhere");
            continue;
        } else {
            // Treat this field like a scalar local variable
            cstring newName = refMap->newName(type->getName() + "." + f->name);
            if (ft->is<IR::Type_Bits>()) {
                auto tb = ft->to<IR::Type_Bits>();
                addHeaderField("scalars", newName, tb->size, tb->isSigned);
                scalars_width += tb->size;
                backend->scalarMetadataFields.emplace(f, newName);
            } else if (ft->is<IR::Type_Boolean>()) {
                addHeaderField("scalars", newName, boolWidth, 0);
                scalars_width += boolWidth;
                backend->scalarMetadataFields.emplace(f, newName);
            } else {
                BUG("%1%: Unhandled type for %2%", ft, f);
            }
        }
    }
}

void ConvertHeaders::addHeaderStacks(const IR::Type_Struct* headersStruct) {
    LOG1("Creating stack " << headersStruct);
    for (auto f : headersStruct->fields) {
        auto ft = typeMap->getType(f, true);
        auto stack = ft->to<IR::Type_Stack>();
        if (stack == nullptr)
            continue;
        auto stack_name = extVisibleName(f);
        auto stack_size = stack->getSize();
        auto type = typeMap->getTypeType(stack->elementType, true);
        BUG_CHECK(type->is<IR::Type_Header>(), "%1% not a header type", stack->elementType);
        auto ht = type->to<IR::Type_Header>();
        addHeaderType(ht);
        cstring stack_type = extVisibleName(stack->elementType->to<IR::Type_Header>());
        std::vector<unsigned> ids;
        for (unsigned i = 0; i < stack_size; i++) {
            cstring hdrName = extVisibleName(f) + "[" + Util::toString(i) + "]";
            auto id = json->add_header(stack_type, hdrName);
            ids.push_back(id);
        }
        json->add_header_stack(stack_type, stack_name, stack_size, ids);
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

void ConvertHeaders::addHeaderField(const cstring& header, const cstring& name,
                                     int size, bool is_signed) {
    auto field = new Util::JsonArray();
    field->append(name);
    field->append(size);
    field->append(is_signed);
    json->add_header_field(header, &field);
}

void ConvertHeaders::addHeaderType(const IR::Type_StructLike *st) {
    cstring name = extVisibleName(st);
    auto fields = new Util::JsonArray();
    for (auto f : st->fields) {
        auto ftype = typeMap->getType(f, true);
        if (ftype->to<IR::Type_StructLike>()) {
            BUG("%1%: nested structure", st);
        } else if (ftype->is<IR::Type_Boolean>()) {
            auto field = pushNewArray(fields);
            field->append(f->name.name);
            field->append(boolWidth);
            field->append(0);
        } else if (auto type = ftype->to<IR::Type_Bits>()) {
            auto field = pushNewArray(fields);
            field->append(f->name.name);
            field->append(type->size);
            field->append(type->isSigned);
        } else if (auto type = ftype->to<IR::Type_Varbits>()) {
            auto field = pushNewArray(fields);
            field->append(f->name.name);
            field->append(type->size);  // FIXME -- where does length go?
        } else if (ftype->to<IR::Type_Stack>()) {
            BUG("%1%: nested stack", st);
        } else {
            BUG("%1%: unexpected type for %2%.%3%", ftype, st, f->name);
        }
    }
    // must add padding
    unsigned width = st->width_bits();
    unsigned padding = width % 8;
    if (padding != 0) {
        cstring name = refMap->newName("_padding");
        auto field = pushNewArray(fields);
        field->append(name);
        field->append(8 - padding);
        field->append(false);
    }

    UNUSED auto id = json->add_header_type(name, &fields);

    LOG1("... creating aliases for metadata fields " << st);
    for (auto f : st->fields) {
        if (auto name_annotation = f->getAnnotation("alias")) {
            auto container = new Util::JsonArray();
            auto alias = new Util::JsonArray();
            auto target_name = name_annotation->expr.front()->to<IR::StringLiteral>()->value;
            LOG2("field alias " << target_name);
            container->append(target_name); // name on target
            // break down the alias into meta . field
            alias->append(name);      // metadata name
            alias->append(f->name);   // field name
            container->append(alias);
            json->field_aliases->append(container);
        }
    }
}

/**
 * We synthesize a "header_type" for each local which has a struct type
 * and we pack all the scalar-typed locals into a 'scalar' type
 */
Visitor::profile_t ConvertHeaders::init_apply(const IR::Node* node) {
    json->add_header_type("scalars");
    refMap->newName("scalars");  // TODO(hanw): avoid modifying refMap?

    // bit<n>, bool, error are packed into 'scalars' type,
    // struct and stack introduces new header types
    for (auto v : backend->getStructure().variables) {
        LOG1("variable " << v);
        auto type = typeMap->getType(v, true);
        if (auto st = type->to<IR::Type_StructLike>()) {
            auto metadata_type = extVisibleName(st);
            json->add_metadata(metadata_type, v->name);
            // only create header type only
            if (visitedHeaders.find(st->getName()) != visitedHeaders.end())
                continue;  // already seen
            else
                visitedHeaders.emplace(st->getName());
            addHeaderType(st);
        } else if (auto stack = type->to<IR::Type_Stack>()) {
            auto type = typeMap->getTypeType(stack->elementType, true);
            BUG_CHECK(type->is<IR::Type_Header>(), "%1% not a header type", stack->elementType);
            auto ht = type->to<IR::Type_Header>();
            addHeaderType(ht);
            cstring header_type = extVisibleName(stack->elementType->to<IR::Type_Header>());
            std::vector<unsigned> header_ids;
            for (unsigned i=0; i < stack->getSize(); i++) {
                cstring name = v->name + "[" + Util::toString(i) + "]";
                auto header_id = json->add_header(header_type, name);
                header_ids.push_back(header_id);
            }
            json->add_header_stack(header_type, v->name, stack->getSize(), header_ids);
        } else if (type->is<IR::Type_Bits>()) {
            auto tb = type->to<IR::Type_Bits>();
            addHeaderField("scalars", v->name.name, tb->size, tb->isSigned);
            scalars_width += tb->size;
        } else if (type->is<IR::Type_Boolean>()) {
            addHeaderField("scalars", v->name.name, boolWidth, false);
            scalars_width += boolWidth;
        } else if (type->is<IR::Type_Error>()) {
            addHeaderField("scalars", v->name.name, 32, 0);
            scalars_width += 32;
        } else {
            P4C_UNIMPLEMENTED("%1%: type not yet handled on this target", type);
        }
    }

    // always-have metadata instance
    json->add_metadata("scalars", "scalars");
    json->add_metadata("standard_metadata", "standard_metadata");
    return Inspector::init_apply(node);
}

void ConvertHeaders::end_apply(UNUSED const IR::Node* node) {
    // pad scalars to byte boundary
    unsigned padding = scalars_width % 8;
    if (padding != 0) {
        cstring name = refMap->newName("_padding");
        addHeaderField("scalars", name, 8-padding, false);
    }
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
    auto ft = typeMap->getType(param->getNode(), true);
    if (ft->is<IR::Type_Struct>()) {
        auto st = ft->to<IR::Type_Struct>();
        if (visitedHeaders.find(st->getName()) != visitedHeaders.end())
            return false;  // already seen
        else
            visitedHeaders.emplace(st->getName());

        if (st->getAnnotation("metadata")) {
            addHeaderType(st);
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

/// Blocks are not in IR tree, use a custom visitor to traverse
bool ConvertHeaders::preorder(const IR::PackageBlock *block) {
    for (auto it : block->constantValue) {
        if (it.second->is<IR::Block>()) {
            visit(it.second->getNode());
        }
    }
    return false;
}

}  // namespace BMV2

