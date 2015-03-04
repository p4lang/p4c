#include "behavioral_sim/P4Objects.h"

typedef unsigned char opcode_t;

void P4Objects::build_conditional(const Json::Value &json_expression,
				  Conditional *conditional) {
  if(json_expression.isNull()) return ;
  const std::string type = json_expression["type"].asString();
  const Json::Value json_value = json_expression["value"];
  if(type == "expression") {
    const std::string op = json_value["op"].asString();
    const Json::Value json_left = json_value["left"];
    const Json::Value json_right = json_value["left"];

    build_conditional(json_left, conditional);
    build_conditional(json_right, conditional);
    
    ExprOpcode opcode = ExprOpcodesMap::get_opcode(op);
    conditional->push_back_op(opcode);
  }
  else if(type == "header") {
    header_id_t header_id = get_header_id(json_value.asString());
    conditional->push_back_load_header(header_id);
  }
  else if(type == "field") {
    const string header_name = json_value[0].asString();
    header_id_t header_id = get_header_id(header_name);
    const string field_name = json_value[1].asString();
    int field_offset = get_field_offset(header_id, field_name);
    conditional->push_back_load_field(header_id, field_offset);
  }
  else if(type == "bool") {
    conditional->push_back_load_bool(json_value.asBool());
  }
  else if(type == "hexstr") {
    conditional->push_back_load_const( Data(json_value.asString()) );
  }
  else {
    assert(0);
  }
}

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

    const Json::Value cfg_fields = cfg_header_type["fields"];
    for (auto it = cfg_fields.begin(); it != cfg_fields.end(); it++) {
      const string field_name = it.key().asString();
      int field_bit_width = (*it).asInt();
      header_type->push_back_field(field_name, field_bit_width);
    }

    add_header_type(header_type_name, unique_ptr<HeaderType>(header_type));
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
	header_id_t header_id = get_header_id(header_name);
	const string field_name = cfg_key_field[1].asString();
	int field_offset = get_field_offset(header_id, field_name);
	key_builder.push_back_field(header_id, field_offset);
      }
      
      parse_state->set_key_builder(key_builder);

      parse_states.push_back(unique_ptr<ParseState>(parse_state));
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

	if(value_hexstr == "default") {
	  parse_state->set_default_switch_case(next_state);
	}
	else {
	  parse_state->add_switch_case(ByteContainer(value_hexstr), next_state);
	}
      }
    }

    const string init_state_name = cfg_parser["init_state"].asString();
    const ParseState *init_state = current_parse_states[init_state_name];
    parser->set_init_state(init_state);

    add_parser(parser_name, unique_ptr<Parser>(parser));
  }

  // deparsers

  const Json::Value cfg_deparsers = cfg_root["deparsers"];
  for (const auto &cfg_deparser : cfg_deparsers) {

    const string deparser_name = cfg_deparser["name"].asString();
    Deparser *deparser = new Deparser();

    const Json::Value cfg_ordered_headers = cfg_deparser["order"];
    for (const auto &cfg_header : cfg_ordered_headers) {

      const string header_name = cfg_header.asString();
      deparser->push_back_header(get_header_id(header_name));
    }

    add_deparser(deparser_name, unique_ptr<Deparser>(deparser));
  }

  // action primitives' opcodes

  // I decided to get rid of opcodes...

  // unordered_map<string, opcode_t> primitive_opcodes;
  // const Json::Value cfg_primitive_opcodes = cfg_root["primitive_opcodes"];
  // for (auto it = cfg_primitive_opcodes.begin();
  //      it != cfg_primitive_opcodes.end(); it++) {
  //   const string primitive_name = it.key().asString();
  //   opcode_t opcode = (*it).asInt();
  //   primitive_opcodes[primitive_name] = opcode;
  // }

  // actions
  
  const Json::Value cfg_actions = cfg_root["actions"];
  for (const auto &cfg_action : cfg_actions) {

    const string action_name = cfg_action["name"].asString();
    ActionFn *action_fn = new ActionFn();

    // This runtime data is only useful for the PD layer

    // const Json::Value cfg_runtime_data = cfg_action["runtime_data"];
    // for (const auto &cfg_parameter : cfg_runtime_data) {
      
    //   const string parameter_name = cfg_parameter["name"].asString();
    //   int parameter_bitwidth = cfg_parameter["bitwidth"].asInt();
    // }

    const Json::Value cfg_primitive_calls = cfg_action["primitives"];
    for (const auto &cfg_primitive_call : cfg_primitive_calls) {
      const string primitive_name = cfg_primitive_call["op"].asString();

      ActionPrimitive_ *primitive =
	ActionOpcodesMap::get_instance()->get_primitive(primitive_name);

      action_fn->push_back_primitive(primitive);

      const Json::Value cfg_primitive_parameters =
	cfg_primitive_call["parameters"];
      for(const auto &cfg_parameter : cfg_primitive_parameters) {
	
	const string type = cfg_parameter["type"].asString();

	if(type == "hexstr") {
	  const string value_hexstr = cfg_parameter["value"].asString();
	  action_fn->parameter_push_back_const(Data(value_hexstr));
	}
	else if(type == "runtime_data") {
	  int action_data_offset = cfg_parameter["value"].asInt();
	  action_fn->parameter_push_back_action_data(action_data_offset);
	}
	else if(type == "header") {
	  const string header_name = cfg_parameter["value"].asString();
	  header_id_t header_id = get_header_id(header_name);
	  action_fn->parameter_push_back_header(header_id);
	}
	else if(type == "field") {
	  Json::Value cfg_value_field = cfg_parameter["value"];
	  const string header_name = cfg_value_field[0].asString();
	  header_id_t header_id = get_header_id(header_name);
	  const string field_name = cfg_value_field[1].asString();
	  int field_offset = get_field_offset(header_id, field_name);
	  action_fn->parameter_push_back_field(header_id, field_offset);
	}
      }
    }
    add_action(action_name, unique_ptr<ActionFn>(action_fn));
  }

  // pipelines

  const Json::Value cfg_pipelines = cfg_root["pipelines"];
  for (const auto &cfg_pipeline : cfg_pipelines) {

    const string pipeline_name = cfg_pipeline["name"].asString();
    const string first_node_name = cfg_pipeline["init_table"].asString();

    // pipelines -> tables
    
    const Json::Value cfg_tables = cfg_pipeline["tables"];
    for (const auto &cfg_table : cfg_tables) {

      const string table_name = cfg_table["name"].asString();

      size_t nbytes_key = 0;

      MatchKeyBuilder key_builder;
      const Json::Value cfg_match_key = cfg_table["key"];
      for (const auto &cfg_key_entry : cfg_match_key) {
	
	const Json::Value cfg_key_field = cfg_key_entry["target"];
	const string header_name = cfg_key_field[0].asString();
	header_id_t header_id = get_header_id(header_name);
	const string field_name = cfg_key_field[1].asString();
	int field_offset = get_field_offset(header_id, field_name);
	key_builder.push_back_field(header_id, field_offset);

	nbytes_key += get_field_bytes(header_id, field_offset);
      }

      const string table_type = cfg_table["type"].asString();
      const int table_size = cfg_table["max_size"].asInt();
      // for now, discard ageing and counters
      
      if(table_type == "exact") {
      	ExactMatchTable *table =
	  new ExactMatchTable(table_name, table_size,
			      nbytes_key, key_builder);
	add_exact_match_table(table_name, unique_ptr<ExactMatchTable>(table));
      }
      else if(table_type == "lpm") {
	LongestPrefixMatchTable *table =
	  new LongestPrefixMatchTable(table_name, table_size,
				      nbytes_key, key_builder);
	add_lpm_table(table_name, unique_ptr<LongestPrefixMatchTable>(table));
      }
      else if(table_type == "ternary") {
	TernaryMatchTable *table =
	  new TernaryMatchTable(table_name, table_size,
				nbytes_key, key_builder);
	add_ternary_match_table(table_name, unique_ptr<TernaryMatchTable>(table));
      }
    }

    // pipelines -> conditionals
    
    const Json::Value cfg_conditionals = cfg_pipeline["conditionals"];
    for (const auto &cfg_conditional : cfg_conditionals) {

      const string conditional_name = cfg_conditional["name"].asString();
      Conditional *conditional = new Conditional(conditional_name);

      const Json::Value cfg_expression = cfg_conditional["expression"];
      build_conditional(cfg_expression, conditional);

      add_conditional(conditional_name, unique_ptr<Conditional>(conditional));
    }

    for (const auto &cfg_conditional : cfg_conditionals) {

      const string conditional_name = cfg_conditional["name"].asString();
      Conditional *conditional = get_conditional(conditional_name);
      
      const Json::Value cfg_true_next = cfg_conditional["true_next"];
      const Json::Value cfg_false_next = cfg_conditional["false_next"];

      if(!cfg_true_next.isNull()) {
	ControlFlowNode *next_node = get_control_node(cfg_true_next.asString());
	conditional->set_next_node_if_true(next_node);
      }
      if(!cfg_false_next.isNull()) {
	ControlFlowNode *next_node = get_control_node(cfg_false_next.asString());
	conditional->set_next_node_if_false(next_node);
      }
    }

    ControlFlowNode *first_node = get_control_node(first_node_name);
    Pipeline *pipeline = new Pipeline(pipeline_name, first_node);
    add_pipeline(pipeline_name, unique_ptr<Pipeline>(pipeline));
  }
}

void P4Objects::destroy_objects() {
  // TODO: I moved to unique_ptr's so just remove this function ?
}

int P4Objects::get_field_offset(header_id_t header_id, const string &field_name) {
  const HeaderType &header_type =
    phv.get_header(header_id).get_header_type();
  return header_type.get_field_offset(field_name);  
}

size_t P4Objects::get_field_bytes(header_id_t header_id,
				  int field_offset) {
  const HeaderType &header_type =
    phv.get_header(header_id).get_header_type();
  return (header_type.get_bit_width(field_offset) + 7) / 8;
}
