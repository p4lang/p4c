/*
Copyright 2024 Marvell Technology, Inc.

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

#include "portableProgramStructure.h"

namespace BMV2 {

using namespace P4::literals;

bool PortableProgramStructure::hasVisited(const IR::Type_StructLike *st) {
    if (auto h = st->to<IR::Type_Header>())
        return header_types.count(h->getName());
    else if (auto s = st->to<IR::Type_Struct>())
        return metadata_types.count(s->getName());
    else if (auto u = st->to<IR::Type_HeaderUnion>())
        return header_union_types.count(u->getName());
    return false;
}

void PortableProgramStructure::createStructLike(ConversionContext *ctxt,
                                                const IR::Type_StructLike *st) {
    CHECK_NULL(st);
    cstring name = st->controlPlaneName();
    unsigned max_length = 0;  // for variable-sized headers
    bool varbitFound = false;
    auto fields = new Util::JsonArray();
    LOG5("In createStructLike with struct " << st->toString());
    for (auto f : st->fields) {
        auto field = new Util::JsonArray();
        auto ftype = typeMap->getType(f, true);
        LOG5("Iterating field with field " << f << " and type " << ftype->toString());
        if (ftype->to<IR::Type_StructLike>()) {
            BUG("%1%: nested structure", st);
        } else if (ftype->is<IR::Type_Boolean>()) {
            field->append(f->name.name);
            field->append(1);
            field->append(false);
            max_length += 1;
        } else if (auto type = ftype->to<IR::Type_Bits>()) {
            field->append(f->name.name);
            field->append(type->size);
            field->append(type->isSigned);
            max_length += type->size;
        } else if (auto type = ftype->to<IR::Type_Varbits>()) {
            field->append(f->name.name);
            max_length += type->size;
            field->append("*");
            if (varbitFound)
                ::error(ErrorType::ERR_UNSUPPORTED,
                        "%1%: headers with multiple varbit fields are not supported", st);
            varbitFound = true;
        } else if (ftype->is<IR::Type_Error>()) {
            field->append(f->name.name);
            field->append(error_width);
            field->append(false);
            max_length += error_width;
        } else if (ftype->to<IR::Type_Stack>()) {
            BUG("%1%: nested stack", st);
        } else {
            BUG("%1%: unexpected type for %2%.%3%", ftype, st, f->name);
        }
        fields->append(field);
    }
    // must add padding
    unsigned padding = max_length % 8;
    if (padding != 0) {
        cstring name = refMap->newName("_padding");
        auto field = new Util::JsonArray();
        field->append(name);
        field->append(8 - padding);
        field->append(false);
        fields->append(field);
    }

    unsigned max_length_bytes = (max_length + padding) / 8;
    if (!varbitFound) {
        // ignore
        max_length = 0;
        max_length_bytes = 0;
    }
    ctxt->json->add_header_type(name, fields, max_length_bytes);
}

void PortableProgramStructure::createTypes(ConversionContext *ctxt) {
    for (auto kv : header_types) createStructLike(ctxt, kv.second);
    for (auto kv : metadata_types) createStructLike(ctxt, kv.second);
    for (auto kv : header_union_types) {
        auto st = kv.second;
        auto fields = new Util::JsonArray();
        for (auto f : st->fields) {
            auto field = new Util::JsonArray();
            auto ftype = typeMap->getType(f, true);
            auto ht = ftype->to<IR::Type_Header>();
            CHECK_NULL(ht);
            field->append(f->name.name);
            field->append(ht->name.name);
        }
        ctxt->json->add_union_type(st->name, fields);
    }
    /* TODO */
    // add errors to json
    // add enums to json
}

void PortableProgramStructure::createScalars(ConversionContext *ctxt) {
    auto name = scalars.begin()->first;
    ctxt->json->add_header("scalars_t"_cs, name);
    ctxt->json->add_header_type("scalars_t"_cs);
    unsigned max_length = 0;
    for (auto kv : scalars) {
        LOG5("Adding a scalar field " << kv.second << " to generated json");
        auto field = new Util::JsonArray();
        auto ftype = typeMap->getType(kv.second, true);
        if (auto type = ftype->to<IR::Type_Bits>()) {
            field->append(kv.second->name);
            max_length += type->size;
            field->append(type->size);
            field->append(type->isSigned);
        } else if (ftype->is<IR::Type_Boolean>()) {
            field->append(kv.second->name);
            max_length += 1;
            field->append(1);
            field->append(false);
        } else {
            BUG_CHECK(kv.second, "%1 is not of Type_Bits or Type_Boolean");
        }
        ctxt->json->add_header_field("scalars_t"_cs, field);
    }
    // must add padding
    unsigned padding = max_length % 8;
    if (padding != 0) {
        cstring name = refMap->newName("_padding");
        auto field = new Util::JsonArray();
        field->append(name);
        field->append(8 - padding);
        field->append(false);
        ctxt->json->add_header_field("scalars_t"_cs, field);
    }
}

void PortableProgramStructure::createHeaders(ConversionContext *ctxt) {
    for (auto kv : headers) {
        auto type = kv.second->type->to<IR::Type_StructLike>();
        ctxt->json->add_header(type->controlPlaneName(), kv.second->name);
    }
    for (auto kv : metadata) {
        auto type = kv.second->type->to<IR::Type_StructLike>();
        ctxt->json->add_metadata(type->controlPlaneName(), kv.second->name);
    }
    /* TODO */
    // for (auto kv : header_stacks) {
    //     json->add_header_stack(stack_type, stack_name, stack_size, ids);
    // }
    for (auto kv : header_unions) {
        auto header_name = kv.first;
        auto header_type = kv.second->to<IR::Type_StructLike>()->controlPlaneName();
        // We have to add separately a header instance for all
        // headers in the union.  Each instance will be named with
        // a prefix including the union name, e.g., "u.h"
        Util::JsonArray *fields = new Util::JsonArray();
        for (auto uf : kv.second->to<IR::Type_HeaderUnion>()->fields) {
            auto uft = typeMap->getType(uf, true);
            auto h_name = header_name + "." + uf->controlPlaneName();
            auto h_type = uft->to<IR::Type_StructLike>()->controlPlaneName();
            unsigned id = ctxt->json->add_header(h_type, h_name);
            fields->append(id);
        }
        ctxt->json->add_union(header_type, fields, header_name);
    }
}

void PortableProgramStructure::createExterns() {
    /* TODO */
    // add parse_vsets to json
    // add meter_arrays to json
    // add counter_arrays to json
    // add register_arrays to json
    // add checksums to json
    // add learn_list to json
    // add calculations to json
    // add extern_instances to json
}

void PortableProgramStructure::createActions(ConversionContext *ctxt) {
    auto cvt = new ActionConverter(ctxt, true);
    for (auto it : actions) {
        auto action = it.first;
        action->apply(*cvt);
    }
}

void PortableProgramStructure::createGlobals() {
    /* TODO */
    // for (auto e : globals) {
    //     convertExternInstances(e->node->to<IR::Declaration>(), e->to<IR::ExternBlock>());
    // }
}

cstring PortableProgramStructure::convertHashAlgorithm(cstring algo) {
    if (algo == "CRC16") {
        return "crc16"_cs;
    }
    if (algo == "CRC16_CUSTOM") {
        return "crc16_custom"_cs;
    }
    if (algo == "CRC32") {
        return "crc32"_cs;
    }
    if (algo == "CRC32_CUSTOM") {
        return "crc32_custom"_cs;
    }
    if (algo == "IDENTITY") {
        return "identity"_cs;
    }

    return nullptr;
}

bool InspectPortableProgram::isHeaders(const IR::Type_StructLike *st) {
    bool result = false;
    for (auto f : st->fields) {
        if (f->type->is<IR::Type_Header>() || f->type->is<IR::Type_Stack>()) {
            result = true;
        }
    }
    return result;
}

bool ParsePortableArchitecture::preorder(const IR::ToplevelBlock *block) {
    // Blocks are not in IR tree, use a custom visitor to traverse.
    for (auto it : block->constantValue) {
        if (it.second->is<IR::Block>()) visit(it.second->getNode());
    }
    return false;
}

}  // namespace BMV2
