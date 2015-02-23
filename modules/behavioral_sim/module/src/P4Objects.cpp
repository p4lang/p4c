#include "P4Objects.h"
#include "behavioral_utils/json.h"
#include <iostream>

typedef unsigned char opcode_t;

void P4Objects::init_objects(std::istream &is) {
  Json::Value cfg_root;
  is >> cfg_root;

  header_type_id_t header_type_id = 0;

  // header types

  const Json::Value cfg_header_types = cfg_root["header_types"];
  for (const auto &cfg_header_type : cfg_header_types) {
    const string header_type_name = cfg_header_type["name"].asString();
    HeaderType *header_type = new HeaderType(header_type_id++,
					     header_type_name);
    add_header_type(header_type_name, header_type);

    const Json::Value cfg_fields = cfg_header_type["fields"];
    for (auto it = cfg_fields.begin(); it != cfg_fields.end(); it++) {
      const string field_name = it.key().asString();
      int field_bit_width = (*it).asInt();
      header_type->push_back_field(field_name, field_bit_width);
    }
  }

  // headers

  const Json::Value cfg_headers = cfg_root["headers"];
  for (const auto &cfg_header : cfg_headers) {

    const string header_name = cfg_header["name"].asString();
    const string header_type_name = cfg_header["header_type"].asString();
    
    HeaderType *header_type = get_header_type(header_type_name);

    header_id_t header_id = phv.push_back_header(header_name, *header_type);
    add_header_id(header_name, header_id);
  }

  // parsers

  const Json::Value cfg_parsers = cfg_root["parsers"];
  for (const auto &cfg_parser : cfg_parsers) {

    const string parser_name = cfg_parser["name"].asString();

    Parser *parser = new Parser();
    add_parser(parser_name, parser);

    unordered_map<string, ParseState *> current_parse_states;

    // parse states

    const Json::Value cfg_parse_states = cfg_parser["parse_states"];
    for (const auto &cfg_parse_state : cfg_parse_states) {

      const string parse_state_name = cfg_parse_state["name"].asString();
      ParseState *parse_state = new ParseState(parse_state_name);

      const Json::Value cfg_extracts = cfg_parse_state["extracts"];
      for(const auto &cfg_extract : cfg_extracts) {
	const string header_name = cfg_extract.asString();
	header_id_t header_id = get_header_id(header_name);
	parse_state->add_extract(header_id);
      }

      // we do not support parser set ops for now

      ParseSwitchKeyBuilder key_builder;
      const Json::Value cfg_transition_key = cfg_parse_state["transition_key"];
      for (const auto &cfg_key_field : cfg_transition_key) {

	const string header_name = cfg_key_field[0].asString();
	const string field_name = cfg_key_field[1].asString();
	header_id_t header_id = get_header_id(header_name);
	const HeaderType &header_type =
	  phv.get_header(header_id).get_header_type();
	int field_offset = header_type.get_field_offset(field_name);
	key_builder.push_back_field(header_id, field_offset);
      }
      
      parse_state->set_key_builder(key_builder);

      parse_states.push_back(parse_state);
      current_parse_states[parse_state_name] = parse_state;
    }

    for (const auto &cfg_parse_state : cfg_parse_states) {

      const string parse_state_name = cfg_parse_state["name"].asString();
      ParseState *parse_state = current_parse_states[parse_state_name];
      const Json::Value cfg_transitions = cfg_parse_state["transitions"];
      for(const auto &cfg_transition : cfg_transitions) {
	
      	const string value_hexstr = cfg_transition["value"].asString();
      	// ignore mask for now
	const string next_state_name = cfg_transition["next_state"].asString();
      	const ParseState *next_state = current_parse_states[next_state_name];
	parse_state->add_switch_case(ByteContainer(value_hexstr),
				     next_state);
      }
    }

    const string init_state_name = cfg_parser["init_state"].asString();
    const ParseState *init_state = current_parse_states[init_state_name];
    parser->set_init_state(init_state);
  }

  // deparsers

  const Json::Value cfg_deparsers = cfg_root["deparsers"];
  for (const auto &cfg_deparser : cfg_deparsers) {

    const string deparser_name = cfg_deparser["name"].asString();
    Deparser *deparser = new Deparser();
    add_deparser(deparser_name, deparser);

    const Json::Value cfg_ordered_headers = cfg_deparser["order"];
    for (const auto &cfg_header : cfg_ordered_headers) {

      const string header_name = cfg_header.asString();
      deparser->push_back_header(get_header_id(header_name));
    }
  }

  // action primitives' opcodes

  unordered_map<string, opcode_t> primitive_opcodes;
  const Json::Value cfg_primitive_opcodes = cfg_root["primitive_opcodes"];
  for (auto it = cfg_primitive_opcodes.begin();
       it != cfg_primitive_opcodes.end(); it++) {
    const string primitive_name = it.key().asString();
    opcode_t opcode = (*it).asInt();
    primitive_opcodes[primitive_name] = opcode;
  }

  // actions
  
  // const Json::Value cfg_actions = cfg_root["actions"];
  // for (const auto &cfg_actions : cfg_actions) {

  //   const string action_name = cfg_action["name"].asString();
  //   ActionFn *action_fn = new ActionFn();

  //   add_action(action_name, action);    

  //   const Json::Value cfg_parameters = cfg_action["parameters"];
  //   for (const auto &cfg_parameter : cfg_parameters) {
      
  //     const string parameter_type = cfg_parameter["type"].asString();

  //     if(parameter_type == "field") {
  // 	const Json::Value cfg_value = cfg_parameter["value"];
  // 	const string header_name = cfg_value[0].asString();
  // 	header_id_t header_id = get_header_id(header_name);
  // 	const HeaderType &header_type =
  // 	  phv.get_header(header_id).get_header_type();
  // 	int field_offset = header_type.get_field_offset(field_name);	
  //     }
  //   }

  // }
}

/* use unique pointers to get rid of this code ?, it is not really important
   anyway, this is config only, and is only allocated once */

void P4Objects::destroy_objects() {
  for(auto it = header_types_map.begin(); it != header_types_map.end(); ++it)
    delete it->second;
  for(auto it = exact_tables_map.begin(); it != exact_tables_map.end(); ++it)
    delete it->second;
  for(auto it = lpm_tables_map.begin(); it != lpm_tables_map.end(); ++it)
    delete it->second;
  for(auto it = ternary_tables_map.begin(); it != ternary_tables_map.end(); ++it)
    delete it->second;
  for(auto it = parsers.begin(); it != parsers.end(); ++it)
    delete it->second;
  for(auto it = deparsers.begin(); it != deparsers.end(); ++it)
    delete it->second;
  for(auto it : parse_states)
    delete it;
}
