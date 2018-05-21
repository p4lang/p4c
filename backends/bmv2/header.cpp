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
Util::JsonArray* HeaderConverter::pushNewArray(Util::JsonArray* parent) {
    auto result = new Util::JsonArray();
    parent->append(result);
    return result;
}

HeaderConverter::HeaderConverter(Backend* backend, cstring scalarsName)
        : backend(backend), scalarsName(scalarsName), refMap(backend->getRefMap()),
          typeMap(backend->getTypeMap()), json(backend->json) {
    setName("HeaderConverter");
    CHECK_NULL(backend->json);
}

/**
 * Create header type and header instance from a IR::StructLike type
 *
 * @param meta this boolean indicates if the struct is a metadata or header.
 */
void HeaderConverter::addTypesAndInstances(const IR::Type_StructLike* type, bool meta) {
    LOG1("Adding " << type);
    for (auto f : type->fields) {
        auto ft = typeMap->getType(f, true);
        if (ft->is<IR::Type_StructLike>()) {
            // The headers struct can not contain nested structures.
            if (!meta && ft->is<IR::Type_Struct>()) {
                ::error("Type %1% should only contain headers, header stacks, or header unions",
                        type);
                return;
            }
            auto st = ft->to<IR::Type_StructLike>();
            addHeaderType(st);
        }
    }

    for (auto f : type->fields) {
        auto ft = typeMap->getType(f, true);
        if (ft->is<IR::Type_StructLike>()) {
            auto header_name = f->controlPlaneName();
            auto header_type = ft->to<IR::Type_StructLike>()->controlPlaneName();
            if (meta == true) {
                json->add_metadata(header_type, header_name);
            } else {
                if (ft->is<IR::Type_Header>()) {
                    json->add_header(header_type, header_name);
                } else if (ft->is<IR::Type_HeaderUnion>()) {
                    // We have to add separately a header instance for all
                    // headers in the union.  Each instance will be named with
                    // a prefix including the union name, e.g., "u.h"
                    Util::JsonArray* fields = new Util::JsonArray();
                    for (auto uf : ft->to<IR::Type_HeaderUnion>()->fields) {
                        auto uft = typeMap->getType(uf, true);
                        auto h_name = header_name + "." + uf->controlPlaneName();
                        auto h_type = uft->to<IR::Type_StructLike>()
                                         ->controlPlaneName();
                        unsigned id = json->add_header(h_type, h_name);
                        fields->append(id);
                    }
                    json->add_union(header_type, fields, header_name);
                } else {
                    BUG("Unexpected type %1%", ft);
                }
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
                addHeaderField(scalarsTypeName, newName, tb->size, tb->isSigned);
                scalars_width += tb->size;
                backend->scalarMetadataFields.emplace(f, newName);
            } else if (ft->is<IR::Type_Boolean>()) {
                addHeaderField(scalarsTypeName, newName, boolWidth, 0);
                scalars_width += boolWidth;
                backend->scalarMetadataFields.emplace(f, newName);
            } else if (ft->is<IR::Type_Error>()) {
                addHeaderField(scalarsTypeName, newName, errorWidth, 0);
                scalars_width += errorWidth;
                backend->scalarMetadataFields.emplace(f, newName);
            } else {
                BUG("%1%: Unhandled type for %2%", ft, f);
            }
        }
    }
}

void HeaderConverter::addHeaderStacks(const IR::Type_Struct* headersStruct) {
    LOG1("Creating stack " << headersStruct);
    for (auto f : headersStruct->fields) {
        auto ft = typeMap->getType(f, true);
        auto stack = ft->to<IR::Type_Stack>();
        if (stack == nullptr)
            continue;
        auto stack_name = f->controlPlaneName();
        auto stack_size = stack->getSize();
        auto type = typeMap->getTypeType(stack->elementType, true);
        BUG_CHECK(type->is<IR::Type_Header>(), "%1% not a header type", stack->elementType);
        auto ht = type->to<IR::Type_Header>();
        addHeaderType(ht);
        cstring stack_type = stack->elementType->to<IR::Type_Header>()
                                               ->controlPlaneName();
        std::vector<unsigned> ids;
        for (unsigned i = 0; i < stack_size; i++) {
            cstring hdrName = f->controlPlaneName() + "[" + Util::toString(i) + "]";
            auto id = json->add_header(stack_type, hdrName);
            ids.push_back(id);
        }
        json->add_header_stack(stack_type, stack_name, stack_size, ids);
    }
}

bool HeaderConverter::isHeaders(const IR::Type_StructLike* st) {
    bool result = false;
    for (auto f : st->fields) {
        if (f->type->is<IR::Type_Header>() || f->type->is<IR::Type_Stack>()) {
            result = true;
        }
    }
    return result;
}

void HeaderConverter::addHeaderField(const cstring& header, const cstring& name,
                                    int size, bool is_signed) {
    Util::JsonArray* field = new Util::JsonArray();
    field->append(name);
    field->append(size);
    field->append(is_signed);
    json->add_header_field(header, field);
}

void HeaderConverter::addHeaderType(const IR::Type_StructLike *st) {
    cstring name = st->controlPlaneName();
    auto fields = new Util::JsonArray();
    unsigned max_length = 0;  // for variable-sized headers
    bool varbitFound = false;

    if (st->is<IR::Type_HeaderUnion>()) {
        for (auto f : st->fields) {
            auto ftype = typeMap->getType(f, true);
            auto ht = ftype->to<IR::Type_Header>();
            CHECK_NULL(ht);
            addHeaderType(ht);
            auto field = pushNewArray(fields);
            field->append(f->name.name);
            field->append(ht->name.name);
        }
        json->add_union_type(st->name, fields);
        return;
    }

    for (auto f : st->fields) {
        auto ftype = typeMap->getType(f, true);
        if (ftype->to<IR::Type_StructLike>()) {
            BUG("%1%: nested structure", st);
        } else if (ftype->is<IR::Type_Boolean>()) {
            auto field = pushNewArray(fields);
            field->append(f->name.name);
            field->append(boolWidth);
            field->append(0);
            max_length += boolWidth;
        } else if (auto type = ftype->to<IR::Type_Bits>()) {
            auto field = pushNewArray(fields);
            field->append(f->name.name);
            field->append(type->size);
            field->append(type->isSigned);
            max_length += type->size;
        } else if (auto type = ftype->to<IR::Type_Varbits>()) {
            auto field = pushNewArray(fields);
            field->append(f->name.name);
            max_length += type->size;
            field->append("*");
            if (varbitFound)
                ::error("%1%: headers with multiple varbit fields not supported", st);
            varbitFound = true;
        } else if (ftype->to<IR::Type_Stack>()) {
            BUG("%1%: nested stack", st);
        } else {
            BUG("%1%: unexpected type for %2%.%3%", ftype, st, f->name);
        }
    }

    // must add padding
    unsigned padding = max_length % 8;
    if (padding != 0) {
        cstring name = refMap->newName("_padding");
        auto field = pushNewArray(fields);
        field->append(name);
        field->append(8 - padding);
        field->append(false);
    }

    unsigned max_length_bytes = (max_length + padding) / 8;
    if (!varbitFound) {
        // ignore
        max_length = 0;
        max_length_bytes = 0;
    }
    json->add_header_type(name, fields, max_length_bytes);

    LOG1("... creating aliases for metadata fields " << st);
    for (auto f : st->fields) {
        if (auto aliasAnnotation = f->getAnnotation("alias")) {
            auto container = new Util::JsonArray();
            auto alias = new Util::JsonArray();
            auto target_name = aliasAnnotation->expr.front()->to<IR::StringLiteral>()->value;
            LOG2("field alias " << target_name);
            container->append(target_name);  // name on target
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
Visitor::profile_t HeaderConverter::init_apply(const IR::Node* node) {
    scalarsTypeName = refMap->newName("scalars");
    json->add_header_type(scalarsTypeName);
    // bit<n>, bool, error are packed into scalars type,
    // varbit, struct and stack introduce new header types
    for (auto v : backend->getStructure().variables) {
        LOG1("variable " << v);
        auto type = typeMap->getType(v, true);
        if (auto st = type->to<IR::Type_StructLike>()) {
            auto metadata_type = st->controlPlaneName();
            if (type->is<IR::Type_Header>())
                json->add_header(metadata_type, v->name);
            else
                json->add_metadata(metadata_type, v->name);
            addHeaderType(st);
        } else if (auto stack = type->to<IR::Type_Stack>()) {
            auto type = typeMap->getTypeType(stack->elementType, true);
            BUG_CHECK(type->is<IR::Type_Header>(), "%1% not a header type", stack->elementType);
            auto ht = type->to<IR::Type_Header>();
            addHeaderType(ht);
            cstring header_type = stack->elementType->to<IR::Type_Header>()
                                                    ->controlPlaneName();
            std::vector<unsigned> header_ids;
            for (unsigned i=0; i < stack->getSize(); i++) {
                cstring name = v->name + "[" + Util::toString(i) + "]";
                auto header_id = json->add_header(header_type, name);
                header_ids.push_back(header_id);
            }
            json->add_header_stack(header_type, v->name, stack->getSize(), header_ids);
        } else if (type->is<IR::Type_Varbits>()) {
            // For each varbit variable we synthesize a separate header instance,
            // since we cannot have multiple varbit fields in a single header.
            auto vbt = type->to<IR::Type_Varbits>();
            cstring headerName = "$varbit" + Util::toString(vbt->size);  // This name will be unique
            auto vec = new IR::IndexedVector<IR::StructField>();
            auto sf = new IR::StructField("field", type);
            vec->push_back(sf);
            typeMap->setType(sf, type);
            auto hdrType = new IR::Type_Header(headerName, *vec);
            typeMap->setType(hdrType, hdrType);
            json->add_metadata(headerName, v->name);
            if (visitedHeaders.find(headerName) != visitedHeaders.end())
                continue;  // already seen
            visitedHeaders.emplace(headerName);
            addHeaderType(hdrType);
        } else if (type->is<IR::Type_Bits>()) {
            auto tb = type->to<IR::Type_Bits>();
            addHeaderField(scalarsTypeName, v->name.name, tb->size, tb->isSigned);
            scalars_width += tb->size;
        } else if (type->is<IR::Type_Boolean>()) {
            addHeaderField(scalarsTypeName, v->name.name, boolWidth, false);
            scalars_width += boolWidth;
        } else if (type->is<IR::Type_Error>()) {
            addHeaderField(scalarsTypeName, v->name.name, errorWidth, 0);
            scalars_width += errorWidth;
        } else if (type->is<IR::Type_Set>()) {
            continue;  // ignore: this is probably a value_set
        } else {
            P4C_UNIMPLEMENTED("%1%: type not yet handled on this target", type);
        }
    }

    // always-have metadata instance
    json->add_metadata(scalarsTypeName, scalarsName);
    json->add_metadata("standard_metadata", "standard_metadata");
    return Inspector::init_apply(node);
}

void HeaderConverter::end_apply(const IR::Node*) {
    // pad scalars to byte boundary
    unsigned padding = scalars_width % 8;
    if (padding != 0) {
        cstring name = refMap->newName("_padding");
        addHeaderField(scalarsTypeName, name, 8-padding, false);
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
bool HeaderConverter::preorder(const IR::Parameter* param) {
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

}  // namespace BMV2
