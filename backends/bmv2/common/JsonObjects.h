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
    /// @brief Finds an object in a JSON array by its name.
    /// @param array The JSON array to search in.
    /// @param name The name of the object to find.
    /// @return A pointer to the JsonObject with the specified name, or nullptr if not found.
    static Util::JsonObject *find_object_by_name(Util::JsonArray *array, const cstring &name);

    /// @brief Adds program information to the top-level JsonObject.
    /// @param name The name of the program.
    void add_program_info(const cstring &name);

    /// @brief Adds meta information to the JsonObject.
    void add_meta_info();

    /// @brief Create a header type in json.
    /// @param name header name
    /// @param type header type
    /// @param fields a JsonArray for the fields in the header
    /// @param max_length  maximum length for a header with varbit fields; if 0 header does not
    /// @return The ID of the newly created header type.
    unsigned add_header_type(const cstring &name, Util::JsonArray *&fields, unsigned max_length);

    /// @brief Creates a union type in JSON.
    /// @param name The name of the union type.
    /// @param fields A JsonArray containing the headers in the union type.
    /// @return The ID of the newly created union type.
    unsigned add_union_type(const cstring &name, Util::JsonArray *&fields);

    /// @brief Creates a header union instance in JSON.
    /// @param type The type of the header union.
    /// @param fields A JsonArray containing the header IDs in the union.
    /// @param name The name of the header union instance.
    /// @return The ID of the newly created header union instance.
    unsigned add_union(const cstring &type, Util::JsonArray *&fields, const cstring &name);

    /// @brief Create a header type with empty field list.
    /// @param name header name
    /// @param type header type
    /// @param fields a JsonArray for the fields in the header
    /// @param max_length  maximum length for a header with varbit fields; if 0 header does not
    /// @return The ID of the newly created header type.
    unsigned add_header_type(const cstring &name);

    /// @brief Adds a set of fields to an existing header type.
    /// @param name The name of the header type.
    /// @param field The JsonArray containing the fields to add.
    void add_header_field(const cstring &name, Util::JsonArray *&field);

    /// @brief Creates a header instance in JSON.
    /// @param type The type of the header.
    /// @param name The name of the header instance.
    /// @return The ID of the newly created header instance.
    unsigned add_header(const cstring &type, const cstring &name);

    /// @brief Creates a metadata header instance in JSON.
    /// @param type The type of the metadata.
    /// @param name The name of the metadata header instance.
    /// @return The ID of the newly created metadata header instance.
    unsigned add_metadata(const cstring &type, const cstring &name);

    /// @brief Adds a header stack to the JSON representation.
    /// @param type The type of the headers in the stack.
    /// @param name The name of the header stack.
    /// @param size The size of the header stack.
    /// @param ids The vector of header IDs in the stack.
    void add_header_stack(const cstring &type, const cstring &name, const unsigned size,
                          const std::vector<unsigned> &header_ids);

    /// @brief Adds a header union stack to the JSON representation.
    /// @param type The type of the header unions in the stack.
    /// @param name The name of the header union stack.
    /// @param size The size of the header union stack.
    /// @param ids The vector of header union IDs in the stack.
    void add_header_union_stack(const cstring &type, const cstring &name, const unsigned size,
                                const std::vector<unsigned> &header_ids);

    /// @brief Adds an error to the JSON representation.
    /// @param name The name of the error.
    /// @param type The type of the error.
    void add_error(const cstring &name, const unsigned type);

    /// @brief Adds a single enum entry to the JSON representation.
    /// @param enum_name The name of the enum.
    /// @param entry_name The name of the enum entry.
    /// @param entry_value The value of the enum entry.
    void add_enum(const cstring &enum_name, const cstring &entry_name, const unsigned entry_value);

    /// @brief Adds a parser to the JSON representation.
    /// @param name The name of the parser.
    /// @return The ID of the newly created parser.
    unsigned add_parser(const cstring &name);

    /// @brief Adds a parser state to an existing parser in the JSON representation.
    /// @param parser_id The ID of the parser.
    /// @param state_name The name of the parser state.
    /// @return The ID of the newly created parser state.
    unsigned add_parser_state(const unsigned id, const cstring &state_name);

    /// @brief Adds a parser transition to an existing parser state in the JSON representation.
    /// @param state_id The ID of the parser state.
    /// @param transition The transition to add.
    void add_parser_transition(const unsigned id, Util::IJson *transition);

    /// @brief Adds a parser operation to an existing parser state in the JSON representation.
    /// @param state_id The ID of the parser state.
    /// @param op The operation to add.
    void add_parser_op(const unsigned id, Util::IJson *op);

    /// @brief Adds a parser transition key to an existing parser state in the JSON representation.
    /// @param state_id The ID of the parser state.
    /// @param newKey The new transition key to add.
    void add_parser_transition_key(const unsigned id, Util::IJson *key);

    /// @brief Adds a parse vset to the JSON representation.
    /// @param name The name of the parse vset.
    /// @param bitwidth The compressed bit width of the parse vset.
    /// @param size The maximum size of the parse vset.
    void add_parse_vset(const cstring &name, const unsigned bitwidth, const big_int &size);

    /// @brief Adds an action to the JSON representation.
    /// @param name The name of the action.
    /// @param params The runtime data parameters of the action.
    /// @param body The primitives body of the action.
    /// @return The ID of the newly created action.
    unsigned add_action(const cstring &name, Util::JsonArray *&params, Util::JsonArray *&body);

    /// @brief Adds an extern attribute to the JSON representation.
    /// @param name The name of the attribute.
    /// @param type The type of the attribute.
    /// @param value The value of the attribute.
    /// @param attributes The attributes array to add the new attribute to.
    void add_extern_attribute(const cstring &name, const cstring &type, const cstring &value,
                              Util::JsonArray *attributes);

    /// @brief Adds an extern instance to the JSON representation.
    /// @param name The name of the extern instance.
    /// @param type The type of the extern instance.
    /// @param attributes The attributes array of the extern instance.
    void add_extern(const cstring &name, const cstring &type, Util::JsonArray *attributes);

    /// @brief Constructs a new JsonObjects instance.
    /// Initializes the top-level JsonObject and other member arrays.
    JsonObjects();

    /// @brief Inserts a JSON array into a parent object under a specified key.
    /// @param parent The parent JsonObject to insert the array into.
    Util::JsonArray *insert_array_field(Util::JsonObject *parent, cstring name);

    /// @brief Appends a JSON array to a parent JSON array.
    /// @param parent The parent JsonArray to which the array will be appended.
    /// @return A pointer to the newly created JSON array.
    Util::JsonArray *append_array(Util::JsonArray *parent);

    /// @brief Creates a JSON array named 'parameters' in a parent JsonObject.
    /// @param object The parent JsonObject in which the 'parameters' array will be created.
    /// @return A pointer to the newly created JSON array.
    Util::JsonArray *create_parameters(Util::JsonObject *object);

    /// @brief Creates a primitive JsonObject in a parent JsonArray with the given name.
    /// @param parent The parent JsonArray in which the primitive will be created.
    /// @param name The name of the primitive.
    /// @return A pointer to the newly created primitive JsonObject.
    Util::JsonObject *create_primitive(Util::JsonArray *parent, cstring name);
    /// Given a field list id returns the array of values called "elements".
    /// @brief Retrieves the contents of a field list identified by its ID.
    /// @param id The ID of the field list.
    /// @return A pointer to the JsonArray containing the field list's elements, or nullptr if not
    /// found.
    Util::JsonArray *get_field_list_contents(unsigned id) const;

    std::map<unsigned, Util::JsonObject *> map_parser;
    std::map<unsigned, Util::JsonObject *> map_parser_state;

    Util::JsonObject *toplevel;
    Util::JsonObject *meta;
    Util::JsonArray *actions;
    Util::JsonArray *calculations;
    Util::JsonArray *checksums;
    Util::JsonArray *counters;
    Util::JsonArray *deparsers;
    Util::JsonArray *enums;
    Util::JsonArray *errors;
    Util::JsonArray *externs;
    Util::JsonArray *field_lists;
    Util::JsonArray *headers;
    Util::JsonArray *header_stacks;
    Util::JsonArray *header_types;
    Util::JsonArray *header_union_types;
    Util::JsonArray *header_unions;
    Util::JsonArray *header_union_stacks;
    ordered_map<std::string, unsigned> header_type_id;
    ordered_map<std::string, unsigned> union_type_id;
    Util::JsonArray *learn_lists;
    Util::JsonArray *meter_arrays;
    Util::JsonArray *parsers;
    Util::JsonArray *parse_vsets;
    Util::JsonArray *pipelines;
    Util::JsonArray *register_arrays;
    Util::JsonArray *force_arith;
    Util::JsonArray *field_aliases;
};

}  // namespace BMV2

#endif /* BACKENDS_BMV2_COMMON_JSONOBJECTS_H_ */
