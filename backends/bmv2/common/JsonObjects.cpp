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

#include "JsonObjects.h"

#include "helpers.h"
#include "lib/json.h"

namespace BMV2 {

const int JSON_MAJOR_VERSION = 2;
const int JSON_MINOR_VERSION = 23;

JsonObjects::JsonObjects() {
    toplevel = new Util::JsonObject();
    meta = new Util::JsonObject();
    header_types = insert_array_field(toplevel, "header_types"_cs);
    headers = insert_array_field(toplevel, "headers"_cs);
    header_stacks = insert_array_field(toplevel, "header_stacks"_cs);
    header_union_types = insert_array_field(toplevel, "header_union_types"_cs);
    header_unions = insert_array_field(toplevel, "header_unions"_cs);
    header_union_stacks = insert_array_field(toplevel, "header_union_stacks"_cs);
    field_lists = insert_array_field(toplevel, "field_lists"_cs);
    errors = insert_array_field(toplevel, "errors"_cs);
    enums = insert_array_field(toplevel, "enums"_cs);
    parsers = insert_array_field(toplevel, "parsers"_cs);
    parse_vsets = insert_array_field(toplevel, "parse_vsets"_cs);
    deparsers = insert_array_field(toplevel, "deparsers"_cs);
    meter_arrays = insert_array_field(toplevel, "meter_arrays"_cs);
    counters = insert_array_field(toplevel, "counter_arrays"_cs);
    register_arrays = insert_array_field(toplevel, "register_arrays"_cs);
    calculations = insert_array_field(toplevel, "calculations"_cs);
    learn_lists = insert_array_field(toplevel, "learn_lists"_cs);
    actions = insert_array_field(toplevel, "actions"_cs);
    pipelines = insert_array_field(toplevel, "pipelines"_cs);
    checksums = insert_array_field(toplevel, "checksums"_cs);
    force_arith = insert_array_field(toplevel, "force_arith"_cs);
    externs = insert_array_field(toplevel, "extern_instances"_cs);
    field_aliases = insert_array_field(toplevel, "field_aliases"_cs);
}

Util::JsonArray *JsonObjects::get_field_list_contents(unsigned id) const {
    for (auto e : *field_lists) {
        auto obj = e->to<Util::JsonObject>();
        auto val = obj->get("id")->to<Util::JsonValue>();
        if (val != nullptr && val->isNumber() && val->getInt() == static_cast<int>(id)) {
            return obj->get("elements")->to<Util::JsonArray>();
        }
    }
    return nullptr;
}

Util::JsonObject *JsonObjects::find_object_by_name(Util::JsonArray *array, const cstring &name) {
    for (auto e : *array) {
        auto obj = e->to<Util::JsonObject>();
        auto val = obj->get("name")->to<Util::JsonValue>();
        if (val != nullptr && val->isString() && val->getString() == name) {
            return obj;
        }
    }
    return nullptr;
}

Util::JsonArray *JsonObjects::insert_array_field(Util::JsonObject *parent, cstring name) {
    auto result = new Util::JsonArray();
    parent->emplace(name, result);
    return result;
}

Util::JsonArray *JsonObjects::append_array(Util::JsonArray *parent) {
    auto result = new Util::JsonArray();
    parent->append(result);
    return result;
}

Util::JsonArray *JsonObjects::create_parameters(Util::JsonObject *object) {
    return insert_array_field(object, "parameters"_cs);
}

void JsonObjects::add_program_info(const cstring &name) { toplevel->emplace("program"_cs, name); }

void JsonObjects::add_meta_info() {
    static constexpr int version_major = JSON_MAJOR_VERSION;
    static constexpr int version_minor = JSON_MINOR_VERSION;
    auto version = insert_array_field(meta, "version"_cs);
    version->append(version_major);
    version->append(version_minor);
    meta->emplace("compiler"_cs, "https://github.com/p4lang/p4c");
    toplevel->emplace("__meta__"_cs, meta);
}
/// Create a header type in json.
/// @param name header name
/// @param type header type
/// @param max_length  maximum length for a header with varbit fields;
/// if 0 header does not contain varbit fields
/// @param fields a JsonArray for the fields in the header
unsigned JsonObjects::add_header_type(const cstring &name, Util::JsonArray *&fields,
                                      unsigned max_length) {
    std::string sname(name, name.size());
    auto header_type_id_it = header_type_id.find(sname);
    if (header_type_id_it != header_type_id.end()) {
        return header_type_id_it->second;
    }
    auto header_type = new Util::JsonObject();
    unsigned id = BMV2::nextId("header_types"_cs);
    header_type_id[sname] = id;
    header_type->emplace("name"_cs, name);
    header_type->emplace("id"_cs, id);
    if (fields != nullptr) {
        header_type->emplace("fields"_cs, fields);
    } else {
        auto temp = new Util::JsonArray();
        header_type->emplace("fields"_cs, temp);
    }
    if (max_length > 0) header_type->emplace("max_length"_cs, max_length);
    header_types->append(header_type);
    return id;
}

unsigned JsonObjects::add_union_type(const cstring &name, Util::JsonArray *&fields) {
    std::string sname(name, name.size());
    auto it = union_type_id.find(sname);
    if (it != union_type_id.end()) return it->second;
    auto union_type = new Util::JsonObject();
    unsigned id = BMV2::nextId("header_union_types"_cs);
    union_type_id[sname] = id;
    union_type->emplace("name"_cs, name);
    union_type->emplace("id"_cs, id);
    if (fields != nullptr) {
        union_type->emplace("headers"_cs, fields);
    } else {
        auto temp = new Util::JsonArray();
        union_type->emplace("headers"_cs, temp);
    }
    header_union_types->append(union_type);
    return id;
}

unsigned JsonObjects::add_header_type(const cstring &name) {
    std::string sname(name, name.size());
    auto header_type_id_it = header_type_id.find(sname);
    if (header_type_id_it != header_type_id.end()) {
        return header_type_id_it->second;
    }
    auto header_type = new Util::JsonObject();
    unsigned id = BMV2::nextId("header_types"_cs);
    header_type_id[sname] = id;
    header_type->emplace("name"_cs, name);
    header_type->emplace("id"_cs, id);
    auto temp = new Util::JsonArray();
    header_type->emplace("fields"_cs, temp);
    header_types->append(header_type);
    return id;
}

void JsonObjects::add_header_field(const cstring &name, Util::JsonArray *&field) {
    CHECK_NULL(field);
    Util::JsonObject *headerType = find_object_by_name(header_types, name);
    Util::JsonArray *fields = headerType->get("fields")->to<Util::JsonArray>();
    BUG_CHECK(fields != nullptr, "header '%1%' not found", name);
    fields->append(field);
}

unsigned JsonObjects::add_header(const cstring &type, const cstring &name) {
    auto header = new Util::JsonObject();
    unsigned id = BMV2::nextId("headers"_cs);
    LOG1("add header id " << id);
    header->emplace("name"_cs, name);
    header->emplace("id"_cs, id);
    header->emplace("header_type"_cs, type);
    header->emplace("metadata"_cs, false);
    header->emplace("pi_omit"_cs, true);
    headers->append(header);
    return id;
}

unsigned JsonObjects::add_union(const cstring &type, Util::JsonArray *&headers,
                                const cstring &name) {
    auto u = new Util::JsonObject();
    unsigned id = BMV2::nextId("header_unions"_cs);
    LOG3("add header_union id " << id);
    u->emplace("name"_cs, name);
    u->emplace("id"_cs, id);
    u->emplace("union_type"_cs, type);
    u->emplace("header_ids"_cs, headers);
    u->emplace("pi_omit"_cs, true);
    header_unions->append(u);
    return id;
}

unsigned JsonObjects::add_metadata(const cstring &type, const cstring &name) {
    auto header = new Util::JsonObject();
    unsigned id = BMV2::nextId("headers"_cs);
    LOG3("add metadata header id " << id);
    header->emplace("name"_cs, name);
    header->emplace("id"_cs, id);
    header->emplace("header_type"_cs, type);
    header->emplace("metadata"_cs, true);
    header->emplace("pi_omit"_cs, true);  // Don't expose in PI.
    headers->append(header);
    return id;
}

void JsonObjects::add_header_stack(const cstring &type, const cstring &name, const unsigned size,
                                   const std::vector<unsigned> &ids) {
    auto stack = new Util::JsonObject();
    unsigned id = BMV2::nextId("stack"_cs);
    stack->emplace("name"_cs, name);
    stack->emplace("id"_cs, id);
    stack->emplace("header_type"_cs, type);
    stack->emplace("size"_cs, size);
    auto members = new Util::JsonArray();
    stack->emplace("header_ids"_cs, members);
    for (auto id : ids) {
        members->append(id);
    }
    header_stacks->append(stack);
}

void JsonObjects::add_header_union_stack(const cstring &type, const cstring &name,
                                         const unsigned size, const std::vector<unsigned> &ids) {
    auto stack = new Util::JsonObject();
    unsigned id = BMV2::nextId("union_stack"_cs);
    stack->emplace("name"_cs, name);
    stack->emplace("id"_cs, id);
    stack->emplace("union_type"_cs, type);
    stack->emplace("size"_cs, size);
    auto members = new Util::JsonArray();
    stack->emplace("header_union_ids"_cs, members);
    for (auto id : ids) {
        members->append(id);
    }
    header_union_stacks->append(stack);
}

void JsonObjects::add_error(const cstring &name, const unsigned type) {
    auto arr = append_array(errors);
    arr->append(name);
    arr->append(type);
}

void JsonObjects::add_enum(const cstring &enum_name, const cstring &entry_name,
                           const unsigned entry_value) {
    // look up enum in json by name
    Util::JsonObject *enum_json = find_object_by_name(enums, enum_name);
    if (enum_json == nullptr) {  // first entry in a new enum
        enum_json = new Util::JsonObject();
        enum_json->emplace("name"_cs, enum_name);
        auto entries = insert_array_field(enum_json, "entries"_cs);
        auto entry = new Util::JsonArray();
        entry->append(entry_name);
        entry->append(entry_value);
        entries->append(entry);
        enums->append(enum_json);
        LOG3("new enum object: " << enum_name << " " << entry_name << " " << entry_value);
    } else {  // add entry to existing enum
        auto entries = enum_json->get("entries")->to<Util::JsonArray>();
        auto entry = new Util::JsonArray();
        entry->append(entry_name);
        entry->append(entry_value);
        entries->append(entry);
        LOG3("new enum entry: " << enum_name << " " << entry_name << " " << entry_value);
    }
}

unsigned JsonObjects::add_parser(const cstring &name) {
    auto parser = new Util::JsonObject();
    unsigned id = BMV2::nextId("parser"_cs);
    parser->emplace("name"_cs, name);
    parser->emplace("id"_cs, id);
    parser->emplace("init_state"_cs, IR::ParserState::start);
    auto parse_states = new Util::JsonArray();
    parser->emplace("parse_states"_cs, parse_states);
    parsers->append(parser);

    map_parser.emplace(id, parser);
    return id;
}

unsigned JsonObjects::add_parser_state(const unsigned parser_id, const cstring &state_name) {
    if (map_parser.find(parser_id) == map_parser.end()) BUG("parser %1% not found.", parser_id);
    auto parser = map_parser[parser_id];
    auto states = parser->get("parse_states")->to<Util::JsonArray>();
    auto state = new Util::JsonObject();
    unsigned state_id = BMV2::nextId("parse_states"_cs);
    state->emplace("name"_cs, state_name);
    state->emplace("id"_cs, state_id);
    auto operations = new Util::JsonArray();
    state->emplace("parser_ops"_cs, operations);
    auto transitions = new Util::JsonArray();
    state->emplace("transitions"_cs, transitions);
    states->append(state);
    auto key = new Util::JsonArray();
    state->emplace("transition_key"_cs, key);

    map_parser_state.emplace(state_id, state);
    return state_id;
}

void JsonObjects::add_parser_transition(const unsigned state_id, Util::IJson *transition) {
    if (map_parser_state.find(state_id) == map_parser_state.end())
        BUG("parser state %1% not found.", state_id);
    auto state = map_parser_state[state_id];
    auto transitions = state->get("transitions")->to<Util::JsonArray>();
    CHECK_NULL(transitions);
    auto trans = transition->to<Util::JsonObject>();
    CHECK_NULL(trans);
    transitions->append(trans);
}

void JsonObjects::add_parser_op(const unsigned state_id, Util::IJson *op) {
    if (map_parser_state.find(state_id) == map_parser_state.end())
        BUG("parser state %1% not found.", state_id);
    auto state = map_parser_state[state_id];
    auto statements = state->get("parser_ops")->to<Util::JsonArray>();
    CHECK_NULL(statements);
    statements->append(op);
}

void JsonObjects::add_parser_transition_key(const unsigned state_id, Util::IJson *newKey) {
    if (map_parser_state.find(state_id) != map_parser_state.end()) {
        auto state = map_parser_state[state_id];
        auto keys = state->get("transition_key")->to<Util::JsonArray>();
        CHECK_NULL(keys);
        auto new_keys = newKey->to<Util::JsonArray>();
        for (auto k : *new_keys) {
            keys->append(k);
        }
    }
}

void JsonObjects::add_parse_vset(const cstring &name, const unsigned bitwidth,
                                 const big_int &size) {
    auto parse_vset = new Util::JsonObject();
    unsigned id = BMV2::nextId("parse_vsets"_cs);
    parse_vset->emplace("name"_cs, name);
    parse_vset->emplace("id"_cs, id);
    parse_vset->emplace("compressed_bitwidth"_cs, bitwidth);
    parse_vset->emplace("max_size"_cs, size);
    parse_vsets->append(parse_vset);
}

unsigned JsonObjects::add_action(const cstring &name, Util::JsonArray *&params,
                                 Util::JsonArray *&body) {
    CHECK_NULL(params);
    CHECK_NULL(body);
    auto action = new Util::JsonObject();
    action->emplace("name"_cs, name);
    unsigned id = BMV2::nextId("actions"_cs);
    action->emplace("id"_cs, id);
    action->emplace("runtime_data"_cs, params);
    action->emplace("primitives"_cs, body);
    actions->append(action);
    return id;
}

void JsonObjects::add_extern_attribute(const cstring &name, const cstring &type,
                                       const cstring &value, Util::JsonArray *attributes) {
    auto attr = new Util::JsonObject();
    attr->emplace("name"_cs, name);
    attr->emplace("type"_cs, type);
    attr->emplace("value"_cs, value);
    attributes->append(attr);
}

void JsonObjects::add_extern(const cstring &name, const cstring &type,
                             Util::JsonArray *attributes) {
    auto extn = new Util::JsonObject();
    unsigned id = BMV2::nextId("extern_instances"_cs);
    extn->emplace("name"_cs, name);
    extn->emplace("id"_cs, id);
    extn->emplace("type"_cs, type);
    extn->emplace("attribute_values"_cs, attributes);
    externs->append(extn);
}

}  // namespace BMV2
