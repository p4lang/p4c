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

#include "lib/json.h"
#include "JsonObjects.h"

namespace bm {

JsonObjects::JsonObjects() {
    toplevel = new Util::JsonObject();
    header_types = insert_array_field(toplevel, "header_types");
    headers = insert_array_field(toplevel, "headers");
    header_stacks = insert_array_field(toplevel, "header_stacks");
    field_lists = insert_array_field(toplevel, "field_lists");
    errors = insert_array_field(toplevel, "errors");
    enums = insert_array_field(toplevel, "enums");
    parsers = insert_array_field(toplevel, "parsers");
    deparsers = insert_array_field(toplevel, "deparsers");
    meter_arrays = insert_array_field(toplevel, "meter_arrays");
    counters = insert_array_field(toplevel, "counter_arrays");
    register_arrays = insert_array_field(toplevel, "register_arrays");
    calculations = insert_array_field(toplevel, "calculations");
    learn_lists = insert_array_field(toplevel, "learn_lists");
    actions = insert_array_field(toplevel, "actions");
    pipelines = insert_array_field(toplevel, "pipelines");
    checksums = insert_array_field(toplevel, "checksums");
    force_arith = insert_array_field(toplevel, "force_arith");
    externs = insert_array_field(toplevel, "extern_instances");
}

/// insert a json array to a parent object under key 'name'
Util::JsonArray*
JsonObjects::insert_array_field(Util::JsonObject* parent, cstring name) {
    auto result = new Util::JsonArray();
    parent->emplace(name, result);
    return result;
}

/// append a json array to a parent json array
Util::JsonArray*
JsonObjects::append_array(Util::JsonArray* parent) {
    auto result = new Util::JsonArray();
    parent->append(result);
    return result;
}

/// insert a json array named 'parameters' in a parent json object
Util::JsonArray*
JsonObjects::create_parameters(Util::JsonObject* object) {
    return insert_array_field(object, "parameters");
}

/// append a json object r to a parent json array
/// insert a field 'op' with 'name' to r
Util::JsonObject*
JsonObjects::create_primitive(Util::JsonArray* parent, cstring name) {
    auto result = new Util::JsonObject();
    result->emplace("op", name);
    parent->append(result);
    return result;
}

void
JsonObjects::add_program_info(const cstring& name) {
    toplevel->emplace("program", name);
}

void
JsonObjects::add_meta_info() {
    auto info = new Util::JsonObject();
    static constexpr int version_major = 2;
    static constexpr int version_minor = 7;
    auto version = insert_array_field(info, "version");
    version->append(version_major);
    version->append(version_minor);
    info->emplace("compiler", "https://github.com/p4lang/p4c");
    toplevel->emplace("__meta__", info);
    meta = info; // TODO(hanw): remove
}

/**
 * create a header type in json
 * @param name header name
 * @param type header type
 * @param fields a JsonArray for the fields in the header
 */
unsigned
JsonObjects::add_header_type(const cstring& name, Util::JsonArray** fields) {
    auto header_type = new Util::JsonObject();
    unsigned id = BMV2::nextId("header_types");
    header_type->emplace("name", name);
    header_type->emplace("id", id);
    if (fields != nullptr) {
        header_type->emplace("fields", *fields);
    } else {
        auto temp = new Util::JsonArray();
        header_type->emplace("fields", temp);
    }
    header_types->append(header_type);
    return id;
}

/// create a new field to an existing header type, returns a pointer to the field
void
JsonObjects::add_header_field(const cstring& name, Util::JsonArray** field) {
    CHECK_NULL(field);
    Util::JsonArray* fields = nullptr;
    for (auto h : *header_types) {
        auto hdr = h->to<Util::JsonObject>();
        auto hdrName = hdr->get("name")->to<Util::JsonValue>();
        if (hdrName != nullptr && hdrName->isString() && hdrName->getString() == name) {
            fields = hdr->get("fields")->to<Util::JsonArray>();
            CHECK_NULL(fields);
            break;
        }
    }
    BUG_CHECK(fields != nullptr, "header '%1%' not found", name);
    fields->append(*field);
}

/// create a header instance in json
unsigned
JsonObjects::add_header(const cstring& type, const cstring& name) {
    auto header = new Util::JsonObject();
    unsigned id = BMV2::nextId("headers");
    LOG1("add header id " << id);
    header->emplace("name", name);
    header->emplace("id", id);
    header->emplace("header_type", type);
    header->emplace("metadata", false);
    header->emplace("pi_omit", true);
    headers->append(header);
    return id;
}

unsigned
JsonObjects::add_metadata(const cstring& type, const cstring& name) {
    auto header = new Util::JsonObject();
    unsigned id = BMV2::nextId("headers");
    LOG1("add metadata header id " << id); header->emplace("name", name);
    header->emplace("id", id);
    header->emplace("header_type", type);
    header->emplace("metadata", true);
    header->emplace("pi_omit", true);  // Don't expose in PI.
    headers->append(header);
    return id;
}

void
JsonObjects::add_header_stack(const cstring& type, const cstring& name,
                              const unsigned size, std::vector<unsigned>& ids) {
    auto stack = new Util::JsonObject();
    unsigned id = BMV2::nextId("stack");
    stack->emplace("name", name);
    stack->emplace("id", id);
    stack->emplace("header_type", type);
    stack->emplace("size", size);
    auto members = new Util::JsonArray();
    stack->emplace("header_ids", members);
    for (auto id : ids) {
        members->append(id);
    }
    header_stacks->append(stack);
}

void
JsonObjects::add_field_list() {
}

/// Add an error to json.
void
JsonObjects::add_error(const cstring& name, const unsigned type) {
    auto arr = append_array(errors);
    arr->append(name);
    arr->append(type);
}

/// Add a single enum entry to json.
/// A enum entry is identified with { enum_name, entry_name, entry_value }
void
JsonObjects::add_enum(const cstring& enum_name, const cstring& entry_name,
                      const unsigned entry_value) {
    // look up enum in json by name
    Util::JsonObject* enum_json = nullptr;
    for (auto e : *enums) {
        auto obj = e->to<Util::JsonObject>();
        if (obj != nullptr) {
            auto jname = obj->get("name")->to<Util::JsonValue>();
            if (jname != nullptr && jname->isString() && jname->getString() == enum_name) {
                enum_json = obj;
                break;
            }
        }
    }
    if (enum_json == nullptr) {  // first entry in a new enum
        enum_json = new Util::JsonObject();
        enum_json->emplace("name", enum_name);
        auto entries = insert_array_field(enum_json, "entries");
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

///
unsigned
JsonObjects::add_parser(const cstring& name) {
    auto parser = new Util::JsonObject();
    unsigned id = BMV2::nextId("parser");
    parser->emplace("name", name);
    parser->emplace("id", id);
    parser->emplace("init_state", IR::ParserState::start);
    auto parse_states = new Util::JsonArray();
    parser->emplace("parse_states", parse_states);
    parsers->append(parser);

    map_parser.emplace(id, parser);
    return id;
}

unsigned
JsonObjects::add_parser_state(const unsigned id, const cstring& state_name) {
    if (map_parser.find(id) != map_parser.end()) {
        auto parser = map_parser[id];
        auto states = parser->get("parse_states")->to<Util::JsonArray>();

        auto state = new Util::JsonObject();
        unsigned id = BMV2::nextId("parse_states");
        state->emplace("name", state_name);
        state->emplace("id", id);
        auto operations = new Util::JsonArray();
        state->emplace("parser_ops", operations);
        auto transitions = new Util::JsonArray();
        state->emplace("transitions", transitions);
        states->append(state);
        auto key = new Util::JsonArray();
        state->emplace("transition_key", key);

        map_parser_state.emplace(id, state);
        return id;
    } else {
        BUG("%1% parser state not valid.", state_name);
    }
}

void
JsonObjects::add_parser_transition(const unsigned id, Util::IJson* transition) {
    if (map_parser_state.find(id) != map_parser_state.end()) {
       auto state = map_parser_state[id];
       auto transitions = state->get("transitions")->to<Util::JsonArray>();
       CHECK_NULL(transitions);
       auto trans = transition->to<Util::JsonObject>();
       CHECK_NULL(trans);
       transitions->append(trans); }
}

void
JsonObjects::add_parser_op(const unsigned id, Util::IJson* op) {
    if (map_parser_state.find(id) != map_parser_state.end()) {
        auto state = map_parser_state[id];
        auto statements = state->get("parser_ops")->to<Util::JsonArray>();
        CHECK_NULL(statements);
        statements->append(op);
    }
}

void
JsonObjects::add_parser_transition_key(const unsigned id, Util::IJson* newKey) {
    if (map_parser_state.find(id) != map_parser_state.end()) {
       auto state = map_parser_state[id];
       auto keys = state->get("transition_key")->to<Util::JsonArray>();
       CHECK_NULL(keys);
       auto new_keys = newKey->to<Util::JsonArray>();
       for (auto k : *new_keys) {
           keys->append(k);
       }
    }
}

unsigned
JsonObjects::add_action(const cstring& name, Util::JsonArray** params, Util::JsonArray** body) {
    CHECK_NULL(params);
    CHECK_NULL(body);
    auto action = new Util::JsonObject();
    action->emplace("name", name);
    unsigned id = BMV2::nextId("actions");
    action->emplace("id", id);
    action->emplace("runtime_data", *params);
    action->emplace("primitives", *body);
    actions->append(action);
    return id;
}

void
JsonObjects::add_extern_attribute(const cstring& name, const cstring& type,
                                  const cstring& value, Util::JsonArray* attributes) {
    auto attr = new Util::JsonObject();
    attr->emplace("name", name);
    attr->emplace("type", type);
    attr->emplace("value", value);
    attributes->append(attr);
}

void
JsonObjects::add_extern(const cstring& name, const cstring& type,
                        Util::JsonArray** attributes) {
    auto extn = new Util::JsonObject();
    unsigned id = BMV2::nextId("extern_instances");
    extn->emplace("name", name);
    extn->emplace("id", id);
    extn->emplace("type", type);
    extn->emplace("attribute_values", *attributes);
    externs->append(extn);
}

}  // namespace bm
