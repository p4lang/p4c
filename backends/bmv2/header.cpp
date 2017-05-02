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

bool ConvertHeaders::hasStructLikeMember(const IR::Type_StructLike *st, bool meta) {
    for (auto f : st->fields) {
        if (f->type->is<IR::Type_StructLike>() || f->type->is<IR::Type_Stack>()) {
            return true;
        }
    }
    return false;
}

void ConvertHeaders::createHeaderTypeAndInstance(const IR::Type_StructLike* st, bool meta) {
    LOG4("Visit " << __FUNCTION__);
    BUG_CHECK(!hasStructLikeMember(st, meta),
              "%1%: Header has nested structure.", st);

    auto isTypeCreated =
        backend->headerTypesCreated.find(st) != backend->headerTypesCreated.end();
    if (!isTypeCreated) {
        cstring extName = st->name;
        LOG1("create header " << extName);
        auto result = new Util::JsonObject();
        cstring name = extVisibleName(st);
        result->emplace("name", name);
        result->emplace("id", nextId("header_types"));
        auto fields = mkArrayField(result, "fields");
        backend->pushFields(st, fields);
        backend->headerTypes->append(result);
        backend->headerTypesCreated.insert(st);
    }

    auto isInstanceCreated =
        backend->headerInstancesCreated.find(st) != backend->headerInstancesCreated.end();
    if (!isInstanceCreated) {
        unsigned id = nextId("headers");
        LOG1("create header instance " << id);
        auto json = new Util::JsonObject();
        json->emplace("name", st->name); //FIXME: fix name
        json->emplace("id", id);
        json->emplace("header_type", st->name);
        json->emplace("metadata", meta);
        json->emplace("pi_omit", true);
        backend->headerInstances->append(json);
        backend->headerInstancesCreated.insert(st);
    }
}

void ConvertHeaders::createStack(const IR::Type_Stack *stack, bool meta) {
    LOG4("Visit " << __FUNCTION__);
    auto json = new Util::JsonObject();
    json->emplace("name", "name");
    json->emplace("id", nextId("stack"));
    json->emplace("size", stack->getSize());
    auto type = backend->getTypeMap().getTypeType(stack->elementType, true);
    auto ht = type->to<IR::Type_Header>();

    auto ht_name = stack->elementType->to<IR::Type_Header>()->name;
    json->emplace("header_type", ht_name);
    auto stackMembers = mkArrayField(json, "header_ids");
    for (unsigned i = 0; i < stack->getSize(); i++) {
        createHeaderTypeAndInstance(ht, meta);
    }
    backend->headerStacks->append(json);
}

void ConvertHeaders::createNestedStruct(const IR::Type_StructLike *st, bool meta) {
    LOG4("Visit " << __FUNCTION__);
    if (!hasStructLikeMember(st, meta)) {
        createHeaderTypeAndInstance(st, meta);
    } else {
        for (auto f : st->fields) {
            if (f->type->is<IR::Type_StructLike>()) {
                createNestedStruct(f->type->to<IR::Type_StructLike>(), meta);
            } else if (f->type->is<IR::Type_Stack>()) {
                createStack(f->type->to<IR::Type_Stack>(), meta);
            }
        }
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
    LOG3("Visiting " << dbp(ctrl));
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

// TODO(hanw): handle more than one layer of nesting.
bool ConvertHeaders::preorder(const IR::Parameter* param) {
    /// only handle control block parameters
    auto parent = getContext()->node;
    if (parent->is<IR::ParserBlock>() || parent->is<IR::ControlBlock>()) {
        auto type = backend->getTypeMap().getType(param->getNode(), true);
        LOG3("Visiting param " << dbp(type));
        if (type->is<IR::Type_StructLike>()) {
            auto st = type->to<IR::Type_StructLike>();
            auto isCreated =
                backend->headerTypesCreated.find(st) != backend->headerTypesCreated.end();
            if (!isCreated) {
                createNestedStruct(st, false);
            }
        }
    }
    return false;
}

} // namespace BMV2

