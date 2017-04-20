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
#include "convertHeaders.h"

namespace BMV2 {

Util::JsonArray* ConvertHeaders::pushNewArray(Util::JsonArray* parent) {
    auto result = new Util::JsonArray();
    parent->append(result);
    return result;
}

bool ConvertHeaders::hasStructLikeMember(const IR::Type_StructLike *st, bool meta) {
    for (auto f : st->fields) {
        if (f->type->is<IR::Type_StructLike>() || f->type->is<IR::Type_Stack>()) {
            return true;
        }
    }
    return false;
}


void ConvertHeaders::pushFields(const IR::Type_StructLike *st,
                               Util::JsonArray *fields) {
    for (auto f : st->fields) {
        auto ftype = typeMap->getType(f, true);
        if (ftype->to<IR::Type_StructLike>()) {
            BUG("%1%: nested structure", st);
        } else if (ftype->is<IR::Type_Boolean>()) {
            auto field = pushNewArray(fields);
            field->append(f->name.name);
            field->append(1); // boolWidth
            field->append(0);
        } else if (auto type = ftype->to<IR::Type_Bits>()) {
            auto field = pushNewArray(fields);
            field->append(f->name.name);
            field->append(type->size);
            field->append(type->isSigned);
        } else if (auto type = ftype->to<IR::Type_Varbits>()) {
            auto field = pushNewArray(fields);
            field->append(f->name.name);
            field->append(type->size); // FIXME -- where does length go?
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
}

void ConvertHeaders::createHeaderTypeAndInstance(const IR::Type_StructLike* st, bool meta) {
    // must not have nested struct
    BUG_CHECK(!hasStructLikeMember(st, meta),
              "%1%: Header has nested structure.", st);

    auto isTypeCreated = headerTypesCreated.find(st) != headerTypesCreated.end();
    if (!isTypeCreated) {
        cstring extName = st->name;
        LOG1("create header " << extName);
        auto result = new Util::JsonObject();
        cstring name = extVisibleName(st);
        result->emplace("name", name);
        result->emplace("id", nextId("header_types"));
        auto fields = mkArrayField(result, "fields");
        pushFields(st, fields);
        headerTypes->append(result);
        headerTypesCreated.insert(st);
    }

    auto isInstanceCreated = headerInstancesCreated.find(st) != headerInstancesCreated.end();
    if (!isInstanceCreated) {
        unsigned id = nextId("headers");
        LOG1("create header instance " << id);
        auto json = new Util::JsonObject();
        //FIXME: fix name
        json->emplace("name", st->name);
        json->emplace("id", id);
        json->emplace("header_type", st->name);
        json->emplace("metadata", meta);
        json->emplace("pi_omit", true);
        headerInstances->append(json);
        headerInstancesCreated.insert(st);
    }
}

void ConvertHeaders::createStack(const IR::Type_Stack *stack, bool meta) {
    LOG1("Visiting createStack");
    auto json = new Util::JsonObject();
    json->emplace("name", "name");
    json->emplace("id", nextId("stack"));
    json->emplace("size", stack->getSize());
    auto type = typeMap->getTypeType(stack->elementType, true);
    auto ht = type->to<IR::Type_Header>();

    auto ht_name = stack->elementType->to<IR::Type_Header>()->name;
    json->emplace("header_type", ht_name);
    auto stackMembers = mkArrayField(json, "header_ids");
    for (unsigned i = 0; i < stack->getSize(); i++) {
        createHeaderTypeAndInstance(ht, meta);
    }
    headerStacks->append(json);
}

void ConvertHeaders::createNestedStruct(const IR::Type_StructLike *st, bool meta) {
    LOG1("Visiting createNestedStruct");
    if (!hasStructLikeMember(st, meta)) {
        LOG1("createHeaderTypeAndInstance");
        createHeaderTypeAndInstance(st, meta);
    } else {
        LOG1("create struct fields");
        for (auto f : st->fields) {
            if (f->type->is<IR::Type_StructLike>()) {
                LOG1("create field");
                createNestedStruct(f->type->to<IR::Type_StructLike>(), meta);
            } else if (f->type->is<IR::Type_Stack>()) {
                LOG1("create stack");
                createStack(f->type->to<IR::Type_Stack>(), meta);
            }
        }
    }
}

void ConvertHeaders::addLocals() {
    // We synthesize a "header_type" for each local which has a struct type
    // and we pack all the scalar-typed locals into a scalarsStruct
    auto scalarFields = scalarsStruct->get("fields")->to<Util::JsonArray>();
    CHECK_NULL(scalarFields);

    LOG1("... structure " << structure->variables);
    for (auto v : structure->variables) {
        LOG1("Creating local " << v);
        auto type = typeMap->getType(v, true);
        if (auto st = type->to<IR::Type_StructLike>()) {
            createJsonType(st);
            auto name = st->name;
            // create header instance?
            auto json = new Util::JsonObject();
            json->emplace("name", v->name);
            json->emplace("id", nextId("headers"));
            json->emplace("header_type", name);
            json->emplace("metadata", true);
            json->emplace("pi_omit", true);  // Don't expose in PI.
            headerInstances->append(json);
        } else if (auto stack = type->to<IR::Type_Stack>()) {
            auto json = new Util::JsonObject();
            json->emplace("name", v->name);
            json->emplace("id", nextId("stack"));
            json->emplace("size", stack->getSize());
            auto type = typeMap->getTypeType(stack->elementType, true);
            BUG_CHECK(type->is<IR::Type_Header>(), "%1% not a header type", stack->elementType);
            auto ht = type->to<IR::Type_Header>();
            createJsonType(ht);

            cstring header_type = stack->elementType->to<IR::Type_Header>()->name;
            json->emplace("header_type", header_type);
            auto stackMembers = mkArrayField(json, "header_ids");
            for (unsigned i=0; i < stack->getSize(); i++) {
                unsigned id = nextId("headers");
                stackMembers->append(id);
                auto header = new Util::JsonObject();
                cstring name = v->name + "[" + Util::toString(i) + "]";
                header->emplace("name", name);
                header->emplace("id", id);
                header->emplace("header_type", header_type);
                header->emplace("metadata", false);
                header->emplace("pi_omit", true);  // Don't expose in PI.
                headerInstances->append(header);
            }
            headerStacks->append(json);
        } else if (type->is<IR::Type_Bits>()) {
            auto tb = type->to<IR::Type_Bits>();
            auto field = pushNewArray(scalarFields);
            field->append(v->name.name);
            field->append(tb->size);
            field->append(tb->isSigned);
            scalars_width += tb->size;
        } else if (type->is<IR::Type_Boolean>()) {
            auto field = pushNewArray(scalarFields);
            field->append(v->name.name);
            field->append(boolWidth);
            field->append(0);
            scalars_width += boolWidth;
        } else if (type->is<IR::Type_Error>()) {
            auto field = pushNewArray(scalarFields);
            field->append(v->name.name);
            field->append(32);  // using 32-bit fields for errors
            field->append(0);
            scalars_width += 32;
        } else {
            BUG("%1%: type not yet handled on this target", type);
        }
    }

    //FIXME: do we need to put scalarsStruct in set?
    //headerTypesCreated.insert(scalarsName);
    headerTypes->append(scalarsStruct);

    // insert the scalars instance
    auto json = new Util::JsonObject();
    json->emplace("name", scalarsName);
    json->emplace("id", nextId("headers"));
    json->emplace("header_type", scalarsName);
    json->emplace("metadata", true);
    json->emplace("pi_omit", true);  // Don't expose in PI.
    headerInstances->append(json);
}

void ConvertHeaders::createScalars() {
    scalarsName = refMap->newName("scalars");
    scalarsStruct = new Util::JsonObject();
    scalarsStruct->emplace("name", scalarsName);
    scalarsStruct->emplace("id", nextId("header_types"));
    scalarFields = mkArrayField(scalarsStruct, "fields");
}

void ConvertHeaders::padScalars() {
    unsigned padding = scalars_width % 8;
    auto scalarFields = (*scalarsStruct)["fields"]->to<Util::JsonArray>();
    if (padding != 0) {
        cstring name = refMap->newName("_padding");
        auto field = pushNewArray(scalarFields);
        field->append(name);
        field->append(8 - padding);
        field->append(false);
    }
}

void ConvertHeaders::createJsonType(const IR::Type_StructLike *st) {
    auto isCreated = headerTypesCreated.find(st) != headerTypesCreated.end();
    if (!isCreated) {
        auto typeJson = new Util::JsonObject();
        cstring name = extVisibleName(st);
        typeJson->emplace("name", name);
        typeJson->emplace("id", nextId("header_types"));
        headerTypes->append(typeJson);
        auto fields = mkArrayField(typeJson, "fields");
        pushFields(st, fields);
        headerTypesCreated.insert(st);
    }
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

bool ConvertHeaders::preorder(const IR::Type_Control* ctrl) {
    LOG1("Visiting " << dbp(ctrl));
    auto parent = getContext()->node;
    if (parent->is<IR::P4Control>()) {
        return true;
    }
    return false;
}

bool ConvertHeaders::preorder(const IR::Type_Parser* prsr) {
    LOG3("Visiting " << dbp(prsr));
    auto parent = getContext()->node;
    if (parent->is<IR::P4Parser>()) {
        return true;
    }
    return false;
}

bool ConvertHeaders::preorder(const IR::Type_Extern* ext) {
    LOG3("Visiting " << dbp(ext));
    return false;
}

bool ConvertHeaders::preorder(const IR::Parameter* param) {
    auto type = typeMap->getType(param->getNode(), true);
    LOG3("Visiting " << dbp(type));
    if (type->is<IR::Type_StructLike>()) {
        auto st = type->to<IR::Type_StructLike>();
        auto isCreated = headerTypesCreated.find(st) != headerTypesCreated.end();
        if (!isCreated) {
            createNestedStruct(st, false);
        }
    }
    return false;
}

} // namespace BMV2

