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

#ifndef BACKENDS_BMV2_COMMON_JSONOBJECTS_H_
#define BACKENDS_BMV2_COMMON_JSONOBJECTS_H_

#include <map>
#include "lib/json.h"
#include "lib/ordered_map.h"

namespace BMV2 {

class JsonObjects {
 public:
    static Util::JsonObject* find_object_by_name(Util::JsonArray* array,
                                                 const cstring& name);

    void add_program_info(const cstring& name);
    void add_meta_info();
    unsigned add_header_type(const cstring& name, Util::JsonArray*& fields, unsigned max_length);
    unsigned add_union_type(const cstring& name, Util::JsonArray*& fields);
    unsigned add_union(const cstring& type, Util::JsonArray*& fields, const cstring& name);
    unsigned add_header_type(const cstring& name);
    void add_header_field(const cstring& name, Util::JsonArray*& field);
    unsigned add_header(const cstring& type, const cstring& name);
    unsigned add_metadata(const cstring& type, const cstring& name);
    void add_header_stack(const cstring& type, const cstring& name,
                          const unsigned size, const std::vector<unsigned>& header_ids);
    void add_header_union_stack(const cstring& type, const cstring& name,
                                const unsigned size, const std::vector<unsigned>& header_ids);
    void add_error(const cstring& name, const unsigned type);
    void add_enum(const cstring& enum_name, const cstring& entry_name,
                  const unsigned entry_value);
    unsigned add_parser(const cstring& name);
    unsigned add_parser_state(const unsigned id, const cstring& state_name);
    void add_parser_transition(const unsigned id, Util::IJson* transition);
    void add_parser_op(const unsigned id, Util::IJson* op);
    void add_parser_transition_key(const unsigned id, Util::IJson* key);
    void add_parse_vset(const cstring& name, const unsigned size);
    unsigned add_action(const cstring& name, Util::JsonArray*& params, Util::JsonArray*& body);
    void add_pipeline();
    void add_extern_attribute(const cstring& name, const cstring& type,
                              const cstring& value, Util::JsonArray* attributes);
    void add_extern(const cstring& name, const cstring& type, Util::JsonArray* attributes);
    JsonObjects();
    Util::JsonArray* insert_array_field(Util::JsonObject* parent, cstring name);
    Util::JsonArray* append_array(Util::JsonArray* parent);
    Util::JsonArray* create_parameters(Util::JsonObject* object);
    Util::JsonObject* create_primitive(Util::JsonArray* parent, cstring name);
    // Given a field list id returns the array of values called "elements"
    Util::JsonArray* get_field_list_contents(unsigned id) const;

    std::map<unsigned, Util::JsonObject*> map_parser;
    std::map<unsigned, Util::JsonObject*> map_parser_state;

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
    Util::JsonArray* header_union_types;
    Util::JsonArray* header_unions;
    Util::JsonArray* header_union_stacks;
    ordered_map<std::string, unsigned> header_type_id;
    ordered_map<std::string, unsigned> union_type_id;
    Util::JsonArray* learn_lists;
    Util::JsonArray* meter_arrays;
    Util::JsonArray* parsers;
    Util::JsonArray* parse_vsets;
    Util::JsonArray* pipelines;
    Util::JsonArray* register_arrays;
    Util::JsonArray* force_arith;
    Util::JsonArray* field_aliases;
};

}  // namespace BMV2

#endif /* BACKENDS_BMV2_COMMON_JSONOBJECTS_H_ */
