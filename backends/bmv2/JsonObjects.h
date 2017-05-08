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

#ifndef _BACKENDS_BMV2_JSONOBJECTS_H_
#define _BACKENDS_BMV2_JSONOBJECTS_H_

namespace bm {

class JsonObjects {
 public:
    void add_program_info(const cstring& name);
    void add_meta_info();
    unsigned add_header_type(const cstring& name, Util::JsonArray* fields);
    unsigned add_header_field(const cstring& name, Util::JsonArray* field);
    unsigned add_header(const cstring& type, const cstring& name);
    unsigned add_metadata(const cstring& type, const cstring& name);
    void add_header_stack(const cstring& type, const cstring& name,
                          const unsigned size, std::vector<unsigned>& header_ids);
    void add_field_list();
    void add_error(const cstring& type, const cstring& name);
    void add_enum(const cstring& enum_name, const cstring& entry_name,
                  const unsigned entry_value);
    unsigned add_parser(const cstring& name);
    unsigned add_parser_state(const unsigned id, const cstring& state_name);
    void add_parser_transition(const unsigned id, Util::IJson* transition);
    void add_parser_op(const unsigned id, Util::IJson* op);
    void add_parser_transition_key(const unsigned id, Util::IJson* key);
    void add_meter_array();
    void add_counter();
    void add_register();
    void add_calculation();
    void add_learn_list();
    void add_action(const cstring& name, const Util::JsonArray* params,
                    const Util::JsonArray* body);
    void add_pipeline();
    void add_checksum();
    void add_force_arith();
    void add_extern();
    void add_field_alias();
    JsonObjects();
    Util::JsonArray* insert_array_field(Util::JsonObject* parent, cstring name);
    Util::JsonArray* append_array(Util::JsonArray* parent);
    Util::JsonArray* create_parameters(Util::JsonObject* object);
    Util::JsonObject* create_primitive(Util::JsonArray* parent, cstring name);

    std::map<unsigned, Util::JsonObject*> map_parser;
    std::map<unsigned, Util::JsonObject*> map_parser_state;
    std::map<unsigned, Util::JsonObject*> map_parser_statement;
    std::map<unsigned, Util::JsonObject*> map_parser_transition;

 public:
    Util::JsonObject* toplevel;
    Util::JsonObject* meta;
    Util::JsonArray* actions;
    Util::JsonArray* calculations;
    Util::JsonArray* checksums;
    Util::JsonArray* counters;
    Util::JsonArray* deparsers;
    Util::JsonArray* enums;
    Util::JsonArray* errors;
    Util::JsonArray* externs;
    Util::JsonArray* field_lists;
    Util::JsonArray* headers;
    Util::JsonArray* header_stacks;
    Util::JsonArray* header_types;
    Util::JsonArray* learn_lists;
    Util::JsonArray* meter_arrays;
    Util::JsonArray* parsers;
    Util::JsonArray* pipelines;
    Util::JsonArray* register_arrays;
    Util::JsonArray* force_arith;
    Util::JsonArray* field_aliases;
};

}  // namespace bm

#endif /* _BACKENDS_BMV2_JSONOBJECTS_H_ */
