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
}

void
JsonObjects::add_header_type(const cstring& name) {
    auto header_type = new Util::JsonObject();
    unsigned id = BMV2::nextId("header_types");
    auto fields = insert_array_field(header_type, "fields");
    header_type->emplace("name", name);
    header_type->emplace("id", id);
    // add fields
}

/**
 * create a header instance in json
 */
unsigned
JsonObjects::add_header(const cstring& type, const cstring& name) {
    auto header = new Util::JsonObject();
    unsigned id = BMV2::nextId("headers");
    header->emplace("name", name);
    header->emplace("id", id);
    header->emplace("header_type", type);
    header->emplace("metadata", false);
    // header->emplace("pi_omit", true);
    headers->append(header);
    return id;
}

unsigned
JsonObjects::add_metadata(const cstring& type, const cstring& name) {
    auto header = new Util::JsonObject();
    unsigned id = BMV2::nextId("headers");
    header->emplace("name", name);
    header->emplace("id", id);
    header->emplace("header_type", type);
    header->emplace("metadata", true);
    // header->emplace("pi_omit", true);
    headers->append(header);
    return id;
}

void
JsonObjects::add_header_stack(const cstring& type, const cstring& name, const unsigned size) {
    auto stack = new Util::JsonObject();
    unsigned id = BMV2::nextId("stack");
    stack->emplace("name", name);
    stack->emplace("id", id);
    stack->emplace("header_type", type);
    stack->emplace("size", size);
    auto members = insert_array_field(stack, "header_ids");
    // id are generated automatically
    header_stacks->append(stack);
}

void
JsonObjects::add_field_list() {

}

/// Add an error to json.
void
JsonObjects::add_error(const cstring& type, const cstring& name) {
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
            } else {
                BUG("Json %1% name field is not string.", enum_name);
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
void
JsonObjects::add_parser() {

}

void
JsonObjects::add_deparser() {

}

void
JsonObjects::add_meter_array() {

}

void
JsonObjects::add_counter() {

}

void
JsonObjects::add_register() {

}

void
JsonObjects::add_calculation() {

}

void
JsonObjects::add_learn_list() {

}

void
JsonObjects::add_action() {

}

void
JsonObjects::add_pipeline() {

}

void
JsonObjects::add_checksum() {

}

void
JsonObjects::add_force_arith() {

}

void
JsonObjects::add_extern() {


}

void
JsonObjects::add_field_alias() {

}

}  // namespace bm
