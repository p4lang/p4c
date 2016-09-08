/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#include <bm/bm_sim/P4Objects.h>

#include <string>
#include <tuple>
#include <vector>
#include <set>

#include "jsoncpp/json.h"

namespace bm {

using std::unique_ptr;
using std::string;

typedef unsigned char opcode_t;

namespace {

template <typename T,
          typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
T hexstr_to_int(const std::string &hexstr) {
  // TODO(antonin): temporary trick to avoid code duplication
  return Data(hexstr).get<T>();
}

}  // namespace


void
P4Objects::build_expression(const Json::Value &json_expression,
                            Expression *expr) {
  if (json_expression.isNull()) return;
  const string type = json_expression["type"].asString();
  const Json::Value json_value = json_expression["value"];
  if (type == "expression") {
    const string op = json_value["op"].asString();
    const Json::Value json_left = json_value["left"];
    const Json::Value json_right = json_value["right"];

    if (op == "?") {
      const Json::Value json_cond = json_value["cond"];
      build_expression(json_cond, expr);

      Expression e1, e2;
      build_expression(json_left, &e1);
      build_expression(json_right, &e2);
      expr->push_back_ternary_op(e1, e2);
    } else {
      // special handling for unary + and -, we set the left operand to 0
      if ((op == "+" || op == "-") && json_left.isNull())
        expr->push_back_load_const(Data(0));
      else
        build_expression(json_left, expr);
      build_expression(json_right, expr);

      ExprOpcode opcode = ExprOpcodesMap::get_opcode(op);
      expr->push_back_op(opcode);
    }
  } else if (type == "header") {
    header_id_t header_id = get_header_id(json_value.asString());
    expr->push_back_load_header(header_id);
  } else if (type == "field") {
    const string header_name = json_value[0].asString();
    header_id_t header_id = get_header_id(header_name);
    const string field_name = json_value[1].asString();
    int field_offset = get_field_offset(header_id, field_name);
    expr->push_back_load_field(header_id, field_offset);

    phv_factory.enable_field_arith(header_id, field_offset);
  } else if (type == "bool") {
    expr->push_back_load_bool(json_value.asBool());
  } else if (type == "hexstr") {
    expr->push_back_load_const(Data(json_value.asString()));
  } else if (type == "local") {  // runtime data for expressions in actions
    expr->push_back_load_local(json_value.asInt());
  } else if (type == "register") {
    // TODO(antonin): cheap optimization
    // this may not be worth doing, and probably does not belong here
    const string register_array_name = json_value[0].asString();
    auto &json_index = json_value[1];
    assert(json_index.size() == 2);
    if (json_index["type"].asString() == "hexstr") {
      const unsigned int idx = hexstr_to_int<unsigned int>(
          json_index["value"].asString());
      expr->push_back_load_register_ref(
          get_register_array(register_array_name), idx);
    } else {
      build_expression(json_index, expr);
      expr->push_back_load_register_gen(
          get_register_array(register_array_name));
    }
  } else {
    assert(0);
  }
}

int
P4Objects::init_objects(std::istream *is,
                        LookupStructureFactory *lookup_factory,
                        int device_id, size_t cxt_id,
                        std::shared_ptr<TransportIface> notifications_transport,
                        const std::set<header_field_pair> &required_fields,
                        const ForceArith &arith_objects) {
  Json::Value cfg_root;
  (*is) >> cfg_root;

  if (!notifications_transport) {
    notifications_transport = std::shared_ptr<TransportIface>(
        TransportIface::make_dummy());
  }

  // header types

  const Json::Value &cfg_header_types = cfg_root["header_types"];
  for (const auto &cfg_header_type : cfg_header_types) {
    const string header_type_name = cfg_header_type["name"].asString();
    header_type_id_t header_type_id = cfg_header_type["id"].asInt();
    HeaderType *header_type = new HeaderType(header_type_name,
                                             header_type_id);

    const Json::Value &cfg_fields = cfg_header_type["fields"];
    for (const auto cfg_field : cfg_fields) {
      const string field_name = cfg_field[0].asString();
      bool is_signed = false;
      if (cfg_field.size() > 2)
        is_signed = cfg_field[2].asBool();
      if (cfg_field[1].asString() == "*") {  // VL field
        ArithExpression raw_expr;
        assert(cfg_header_type.isMember("length_exp"));
        const Json::Value &cfg_length_exp = cfg_header_type["length_exp"];
        assert(!cfg_length_exp.isNull());
        build_expression(cfg_length_exp, &raw_expr);
        raw_expr.build();
        std::unique_ptr<VLHeaderExpression> VL_expr(
          new VLHeaderExpression(raw_expr));
        header_type->push_back_VL_field(field_name, std::move(VL_expr),
                                        is_signed);
      } else {
        int field_bit_width = cfg_field[1].asInt();
        header_type->push_back_field(field_name, field_bit_width, is_signed);
      }
    }

    add_header_type(header_type_name, unique_ptr<HeaderType>(header_type));
  }

  // headers

  const Json::Value &cfg_headers = cfg_root["headers"];
  // size_t num_headers = cfg_headers.size();

  for (const auto &cfg_header : cfg_headers) {
    const string header_name = cfg_header["name"].asString();
    const string header_type_name = cfg_header["header_type"].asString();
    header_id_t header_id = cfg_header["id"].asInt();
    bool metadata = cfg_header["metadata"].asBool();

    HeaderType *header_type = get_header_type(header_type_name);
    header_to_type_map[header_name] = header_type;

    // std::set<int> arith_offsets =
    //   build_arith_offsets(cfg_root["actions"], header_name);

    phv_factory.push_back_header(header_name, header_id,
                                 *header_type, metadata);
    phv_factory.disable_all_field_arith(header_id);
    // if (!metadata) // temporary ?
    //   phv_factory.disable_all_field_arith(header_id);
    // else
    //   phv_factory.enable_all_field_arith(header_id);
    add_header_id(header_name, header_id);
  }

  // header stacks

  const Json::Value &cfg_header_stacks = cfg_root["header_stacks"];

  for (const auto &cfg_header_stack : cfg_header_stacks) {
    const string header_stack_name = cfg_header_stack["name"].asString();
    const string header_type_name = cfg_header_stack["header_type"].asString();
    header_stack_id_t header_stack_id = cfg_header_stack["id"].asInt();

    HeaderType *header_stack_type = get_header_type(header_type_name);
    header_stack_to_type_map[header_stack_name] = header_stack_type;

    std::vector<header_id_t> header_ids;
    for (const auto &cfg_header_id : cfg_header_stack["header_ids"])
      header_ids.push_back(cfg_header_id.asInt());

    phv_factory.push_back_header_stack(header_stack_name, header_stack_id,
                                       *header_stack_type, header_ids);
    add_header_stack_id(header_stack_name, header_stack_id);
  }

  if (cfg_root.isMember("field_aliases")) {
    const Json::Value &cfg_field_aliases = cfg_root["field_aliases"];

    for (const auto &cfg_alias : cfg_field_aliases) {
      const auto from = cfg_alias[0].asString();
      const auto tgt = cfg_alias[1];
      const auto header_name = tgt[0].asString();
      const auto field_name = tgt[1].asString();
      assert(field_exists(header_name, field_name));
      const auto to = header_name + "." + field_name;
      phv_factory.add_field_alias(from, to);

      field_aliases[from] = header_field_pair(header_name, field_name);
    }
  }

  // extern instances

  const Json::Value &cfg_extern_instances = cfg_root["extern_instances"];
  for (const auto &cfg_extern_instance : cfg_extern_instances) {
    const string extern_instance_name = cfg_extern_instance["name"].asString();
    const p4object_id_t extern_instance_id = cfg_extern_instance["id"].asInt();

    const string extern_type_name = cfg_extern_instance["type"].asString();
    auto instance = ExternFactoryMap::get_instance()->get_extern_instance(
        extern_type_name);
    if (instance == nullptr) {
      outstream << "Invalid reference to extern type '"
                << extern_type_name << "'\n";
      return 1;
    }

    instance->_register_attributes();

    instance->_set_name_and_id(extern_instance_name, extern_instance_id);

    const Json::Value &cfg_extern_attributes =
        cfg_extern_instance["attribute_values"];
    for (const auto &cfg_extern_attribute : cfg_extern_attributes) {
      // only hexstring accepted
      const string name = cfg_extern_attribute["name"].asString();
      const string type = cfg_extern_attribute["type"].asString();

      if (!instance->_has_attribute(name)) {
        outstream << "Extern type '" << extern_type_name
                  << "' has no attribute '" << name << "'\n";
        return 1;
      }

      if (type == "hexstr") {
        const string value_hexstr = cfg_extern_attribute["value"].asString();
        instance->_set_attribute<Data>(name, Data(value_hexstr));
      } else {
        outstream << "Only attributes of type 'hexstr' are supported for extern"
                  << " instance attribute initialization\n";
        return 1;
      }
    }

    instance->init();

    add_extern_instance(extern_instance_name, std::move(instance));
  }

  // parse value sets

  const Json::Value &cfg_parse_vsets = cfg_root["parse_vsets"];
  for (const auto &cfg_parse_vset : cfg_parse_vsets) {
    const string parse_vset_name = cfg_parse_vset["name"].asString();
    const p4object_id_t parse_vset_id = cfg_parse_vset["id"].asInt();
    const size_t parse_vset_cbitwidth =
        cfg_parse_vset["compressed_bitwidth"].asUInt();

    std::unique_ptr<ParseVSet> vset(
        new ParseVSet(parse_vset_name, parse_vset_id, parse_vset_cbitwidth));
    add_parse_vset(parse_vset_name, std::move(vset));
  }

  // parsers

  const Json::Value &cfg_parsers = cfg_root["parsers"];
  for (const auto &cfg_parser : cfg_parsers) {
    const string parser_name = cfg_parser["name"].asString();
    p4object_id_t parser_id = cfg_parser["id"].asInt();

    Parser *parser = new Parser(parser_name, parser_id);

    std::unordered_map<string, ParseState *> current_parse_states;

    // parse states

    const Json::Value &cfg_parse_states = cfg_parser["parse_states"];
    for (const auto &cfg_parse_state : cfg_parse_states) {
      const string parse_state_name = cfg_parse_state["name"].asString();
      const p4object_id_t id = cfg_parse_state["id"].asInt();
      // p4object_id_t parse_state_id = cfg_parse_state["id"].asInt();
      ParseState *parse_state = new ParseState(parse_state_name, id);

      const Json::Value &cfg_parser_ops = cfg_parse_state["parser_ops"];
      for (const auto &cfg_parser_op : cfg_parser_ops) {
        const string op_type = cfg_parser_op["op"].asString();
        const Json::Value &cfg_parameters = cfg_parser_op["parameters"];
        if (op_type == "extract") {
          assert(cfg_parameters.size() == 1);
          const Json::Value &cfg_extract = cfg_parameters[0];
          const string extract_type = cfg_extract["type"].asString();
          const string extract_header = cfg_extract["value"].asString();
          if (extract_type == "regular") {
            header_id_t header_id = get_header_id(extract_header);
            parse_state->add_extract(header_id);
          } else if (extract_type == "stack") {
            header_stack_id_t header_stack_id =
              get_header_stack_id(extract_header);
            parse_state->add_extract_to_stack(header_stack_id);
          } else {
            assert(0 && "parser extract op not supported");
          }
        } else if (op_type == "set") {
          assert(cfg_parameters.size() == 2);
          const Json::Value &cfg_dest = cfg_parameters[0];
          const Json::Value &cfg_src = cfg_parameters[1];

          const string &dest_type = cfg_dest["type"].asString();
          assert(dest_type == "field");
          const auto dest = field_info(cfg_dest["value"][0].asString(),
                                       cfg_dest["value"][1].asString());
          phv_factory.enable_field_arith(std::get<0>(dest), std::get<1>(dest));

          const string &src_type = cfg_src["type"].asString();
          if (src_type == "field") {
            const auto src = field_info(cfg_src["value"][0].asString(),
                                        cfg_src["value"][1].asString());
            parse_state->add_set_from_field(
              std::get<0>(dest), std::get<1>(dest),
              std::get<0>(src), std::get<1>(src));
            phv_factory.enable_field_arith(std::get<0>(src), std::get<1>(src));
          } else if (src_type == "hexstr") {
            parse_state->add_set_from_data(
              std::get<0>(dest), std::get<1>(dest),
              Data(cfg_src["value"].asString()));
          } else if (src_type == "lookahead") {
            int offset = cfg_src["value"][0].asInt();
            int bitwidth = cfg_src["value"][1].asInt();
            parse_state->add_set_from_lookahead(
              std::get<0>(dest), std::get<1>(dest), offset, bitwidth);
          } else if (src_type == "expression") {
            ArithExpression expr;
            build_expression(cfg_src["value"], &expr);
            expr.build();
            parse_state->add_set_from_expression(
              std::get<0>(dest), std::get<1>(dest), expr);
          } else {
            assert(0 && "parser set op not supported");
          }
        } else {
          assert(0 && "parser op not supported");
        }
      }

      ParseSwitchKeyBuilder key_builder;
      const Json::Value &cfg_transition_key = cfg_parse_state["transition_key"];
      for (const auto &cfg_key_elem : cfg_transition_key) {
        const string type = cfg_key_elem["type"].asString();
        const auto &cfg_value = cfg_key_elem["value"];
        if (type == "field") {
          const auto field = field_info(cfg_value[0].asString(),
                                        cfg_value[1].asString());
          header_id_t header_id = std::get<0>(field);
          int field_offset = std::get<1>(field);
          key_builder.push_back_field(header_id, field_offset,
                                      get_field_bits(header_id, field_offset));
        } else if (type == "stack_field") {
          const string header_stack_name = cfg_value[0].asString();
          header_stack_id_t header_stack_id =
            get_header_stack_id(header_stack_name);
          HeaderType *header_type = header_stack_to_type_map[header_stack_name];
          const string field_name = cfg_value[1].asString();
          int field_offset = header_type->get_field_offset(field_name);
          int bitwidth = header_type->get_bit_width(field_offset);
          key_builder.push_back_stack_field(header_stack_id, field_offset,
                                            bitwidth);
        } else if (type == "lookahead") {
          int offset = cfg_value[0].asInt();
          int bitwidth = cfg_value[1].asInt();
          key_builder.push_back_lookahead(offset, bitwidth);
        } else {
          assert(0 && "invalid entry in parse state key");
        }
      }

      if (cfg_transition_key.size() > 0u)
        parse_state->set_key_builder(key_builder);

      parse_states.push_back(unique_ptr<ParseState>(parse_state));
      current_parse_states[parse_state_name] = parse_state;
    }

    for (const auto &cfg_parse_state : cfg_parse_states) {
      const string parse_state_name = cfg_parse_state["name"].asString();
      ParseState *parse_state = current_parse_states[parse_state_name];
      const Json::Value &cfg_transitions = cfg_parse_state["transitions"];
      for (const auto &cfg_transition : cfg_transitions) {
        // ensures compatibility with old JSON versions
        string type = "hexstr";
        if (cfg_transition.isMember("type"))
          type = cfg_transition["type"].asString();
        if (type == "hexstr") {
          const string value = cfg_transition["value"].asString();
          if (value == "default") type = "default";
        }

        const string next_state_name = cfg_transition["next_state"].asString();
        const ParseState *next_state = current_parse_states[next_state_name];

        if (type == "default") {
          parse_state->set_default_switch_case(next_state);
        } else if (type == "hexstr") {
          const string value_hexstr = cfg_transition["value"].asString();
          const auto &cfg_mask = cfg_transition["mask"];
          if (cfg_mask.isNull()) {
            parse_state->add_switch_case(ByteContainer(value_hexstr),
                                         next_state);
          } else {
            const string mask_hexstr = cfg_mask.asString();
            parse_state->add_switch_case_with_mask(ByteContainer(value_hexstr),
                                                   ByteContainer(mask_hexstr),
                                                   next_state);
          }
        } else if (type == "parse_vset") {
          const string vset_name = cfg_transition["value"].asString();
          ParseVSet *vset = get_parse_vset(vset_name);
          const auto &cfg_mask = cfg_transition["mask"];
          if (cfg_mask.isNull()) {
            parse_state->add_switch_case_vset(vset, next_state);
          } else {
            const string mask_hexstr = cfg_mask.asString();
            parse_state->add_switch_case_vset_with_mask(
                vset, ByteContainer(mask_hexstr), next_state);
          }
        }
      }
    }

    const string init_state_name = cfg_parser["init_state"].asString();
    const ParseState *init_state = current_parse_states[init_state_name];
    parser->set_init_state(init_state);

    add_parser(parser_name, unique_ptr<Parser>(parser));
  }

  // deparsers

  const Json::Value &cfg_deparsers = cfg_root["deparsers"];
  for (const auto &cfg_deparser : cfg_deparsers) {
    const string deparser_name = cfg_deparser["name"].asString();
    p4object_id_t deparser_id = cfg_deparser["id"].asInt();
    Deparser *deparser = new Deparser(deparser_name, deparser_id);

    const Json::Value &cfg_ordered_headers = cfg_deparser["order"];
    for (const auto &cfg_header : cfg_ordered_headers) {
      const string header_name = cfg_header.asString();
      deparser->push_back_header(get_header_id(header_name));
    }

    add_deparser(deparser_name, unique_ptr<Deparser>(deparser));
  }

  // calculations

  const Json::Value &cfg_calculations = cfg_root["calculations"];
  for (const auto &cfg_calculation : cfg_calculations) {
    const string name = cfg_calculation["name"].asString();
    const p4object_id_t id = cfg_calculation["id"].asInt();
    const string algo = cfg_calculation["algo"].asString();

    BufBuilder builder;
    for (const auto &cfg_field : cfg_calculation["input"]) {
      const string type = cfg_field["type"].asString();
      if (type == "field") {
        header_id_t header_id;
        int offset;
        std::tie(header_id, offset) = field_info(
          cfg_field["value"][0].asString(), cfg_field["value"][1].asString());
        builder.push_back_field(header_id, offset);
      } else if (type == "hexstr") {
        builder.push_back_constant(ByteContainer(cfg_field["value"].asString()),
                                   cfg_field["bitwidth"].asInt());
      } else if (type == "header") {
        header_id_t header_id = get_header_id(cfg_field["value"].asString());
        builder.push_back_header(header_id);
      } else if (type == "payload") {
        builder.append_payload();
      }
    }

    // check algo
    if (!check_hash(algo)) return 1;

    NamedCalculation *calculation = new NamedCalculation(name, id,
                                                         builder, algo);
    add_named_calculation(name, unique_ptr<NamedCalculation>(calculation));
  }

  // counter arrays

  const Json::Value &cfg_counter_arrays = cfg_root["counter_arrays"];
  for (const auto &cfg_counter_array : cfg_counter_arrays) {
    const string name = cfg_counter_array["name"].asString();
    const p4object_id_t id = cfg_counter_array["id"].asInt();
    const size_t size = cfg_counter_array["size"].asUInt();
    const Json::Value false_value(false);
    const bool is_direct =
      cfg_counter_array.get("is_direct", false_value).asBool();
    if (is_direct) continue;

    CounterArray *counter_array = new CounterArray(name, id, size);
    add_counter_array(name, unique_ptr<CounterArray>(counter_array));
  }

  // meter arrays

  // store direct meter info until the table gets created
  struct DirectMeterArray {
    MeterArray *meter;
    header_id_t header;
    int offset;
  };

  std::unordered_map<std::string, DirectMeterArray> direct_meters;

  const Json::Value &cfg_meter_arrays = cfg_root["meter_arrays"];
  for (const auto &cfg_meter_array : cfg_meter_arrays) {
    const string name = cfg_meter_array["name"].asString();
    const p4object_id_t id = cfg_meter_array["id"].asInt();
    const string type = cfg_meter_array["type"].asString();
    Meter::MeterType meter_type;
    if (type == "packets") {
      meter_type = Meter::MeterType::PACKETS;
    } else if (type == "bytes") {
      meter_type = Meter::MeterType::BYTES;
    } else {
      assert(0 && "invalid meter type");
    }
    const size_t rate_count = cfg_meter_array["rate_count"].asUInt();
    const size_t size = cfg_meter_array["size"].asUInt();

    MeterArray *meter_array = new MeterArray(name, id,
                                             meter_type, rate_count, size);
    add_meter_array(name, unique_ptr<MeterArray>(meter_array));

    const bool is_direct =
        cfg_meter_array.get("is_direct", Json::Value(false)).asBool();
    if (is_direct) {
      const Json::Value &cfg_target_field = cfg_meter_array["result_target"];
      const string header_name = cfg_target_field[0].asString();
      header_id_t header_id = get_header_id(header_name);
      const string field_name = cfg_target_field[1].asString();
      int field_offset = get_field_offset(header_id, field_name);

      DirectMeterArray direct_meter =
          {get_meter_array(name), header_id, field_offset};
      direct_meters.emplace(name, direct_meter);
    }
  }

  // register arrays

  const Json::Value &cfg_register_arrays = cfg_root["register_arrays"];
  for (const auto &cfg_register_array : cfg_register_arrays) {
    const string name = cfg_register_array["name"].asString();
    const p4object_id_t id = cfg_register_array["id"].asInt();
    const size_t size = cfg_register_array["size"].asUInt();
    const int bitwidth = cfg_register_array["bitwidth"].asInt();

    RegisterArray *register_array = new RegisterArray(name, id, size, bitwidth);
    add_register_array(name, unique_ptr<RegisterArray>(register_array));
  }

  // actions

  const Json::Value &cfg_actions = cfg_root["actions"];
  for (const auto &cfg_action : cfg_actions) {
    const string action_name = cfg_action["name"].asString();
    p4object_id_t action_id = cfg_action["id"].asInt();
    std::unique_ptr<ActionFn> action_fn(new ActionFn(action_name, action_id));

    const Json::Value &cfg_primitive_calls = cfg_action["primitives"];
    for (const auto &cfg_primitive_call : cfg_primitive_calls) {
      const string primitive_name = cfg_primitive_call["op"].asString();

      ActionPrimitive_ *primitive =
        ActionOpcodesMap::get_instance()->get_primitive(primitive_name);
      if (!primitive) {
        outstream << "Unknown primitive action: " << primitive_name
                  << std::endl;
        return 1;
      }

      action_fn->push_back_primitive(primitive);

      const Json::Value &cfg_primitive_parameters =
        cfg_primitive_call["parameters"];

      // check number of parameters
      const size_t num_params_expected = primitive->get_num_params();
      const size_t num_params = cfg_primitive_parameters.size();
      if (num_params != num_params_expected) {
        // should not happen with a good compiler
        outstream << "Invalid number of parameters for primitive action "
                  << primitive_name << ": expected " << num_params_expected
                  << " but got " << num_params << std::endl;
        return 1;
      }

      for (const auto &cfg_parameter : cfg_primitive_parameters) {
        const string type = cfg_parameter["type"].asString();

        if (type == "hexstr") {
          const string value_hexstr = cfg_parameter["value"].asString();
          action_fn->parameter_push_back_const(Data(value_hexstr));
        } else if (type == "runtime_data") {
          int action_data_offset = cfg_parameter["value"].asInt();
          action_fn->parameter_push_back_action_data(action_data_offset);
        } else if (type == "header") {
          const string header_name = cfg_parameter["value"].asString();
          header_id_t header_id = get_header_id(header_name);
          action_fn->parameter_push_back_header(header_id);

          // TODO(antonin):
          // overkill, needs something more efficient, but looks hard
          phv_factory.enable_all_field_arith(header_id);
        } else if (type == "field") {
          const Json::Value &cfg_value_field = cfg_parameter["value"];
          const string header_name = cfg_value_field[0].asString();
          header_id_t header_id = get_header_id(header_name);
          const string field_name = cfg_value_field[1].asString();
          int field_offset = get_field_offset(header_id, field_name);
          action_fn->parameter_push_back_field(header_id, field_offset);

          phv_factory.enable_field_arith(header_id, field_offset);
        } else if (type == "calculation") {
          const string name = cfg_parameter["value"].asString();
          NamedCalculation *calculation = get_named_calculation(name);
          action_fn->parameter_push_back_calculation(calculation);
        } else if (type == "meter_array") {
          const string name = cfg_parameter["value"].asString();
          MeterArray *meter = get_meter_array(name);
          action_fn->parameter_push_back_meter_array(meter);
        } else if (type == "counter_array") {
          const string name = cfg_parameter["value"].asString();
          CounterArray *counter = get_counter_array(name);
          action_fn->parameter_push_back_counter_array(counter);
        } else if (type == "register_array") {
          const string name = cfg_parameter["value"].asString();
          RegisterArray *register_array = get_register_array(name);
          action_fn->parameter_push_back_register_array(register_array);
        } else if (type == "header_stack") {
          const string header_stack_name = cfg_parameter["value"].asString();
          header_id_t header_stack_id = get_header_stack_id(header_stack_name);
          action_fn->parameter_push_back_header_stack(header_stack_id);
        } else if (type == "expression") {
          // TODO(Antonin): should this make the field case (and other) obsolete
          // maybe if we can optimize this case
          ArithExpression *expr = new ArithExpression();
          build_expression(cfg_parameter["value"], expr);
          expr->build();
          action_fn->parameter_push_back_expression(
            std::unique_ptr<ArithExpression>(expr));
        } else if (type == "register") {
          // TODO(antonin): cheap optimization
          // this may not be worth doing, and probably does not belong here
          const Json::Value &cfg_register = cfg_parameter["value"];
          const string register_array_name = cfg_register[0].asString();
          auto &json_index = cfg_register[1];
          assert(json_index.size() == 2);
          if (json_index["type"].asString() == "hexstr") {
            const unsigned int idx = hexstr_to_int<unsigned int>(
                json_index["value"].asString());
            action_fn->parameter_push_back_register_ref(
                get_register_array(register_array_name), idx);
          } else {
            ArithExpression *idx_expr = new ArithExpression();
            build_expression(json_index, idx_expr);
            idx_expr->build();
            action_fn->parameter_push_back_register_gen(
                get_register_array(register_array_name),
                std::unique_ptr<ArithExpression>(idx_expr));
          }
        } else if (type == "extern") {
          const string name = cfg_parameter["value"].asString();
          ExternType *extern_instance = get_extern_instance(name);
          action_fn->parameter_push_back_extern_instance(extern_instance);
        } else {
          assert(0 && "parameter not supported");
        }
      }
    }
    add_action(action_id, std::move(action_fn));
  }

  // pipelines

  ageing_monitor = std::unique_ptr<AgeingMonitor>(
      new AgeingMonitor(device_id, cxt_id, notifications_transport));

  std::unordered_map<std::string, MatchKeyParam::Type> map_name_to_match_type =
      { {"exact", MatchKeyParam::Type::EXACT},
        {"lpm", MatchKeyParam::Type::LPM},
        {"ternary", MatchKeyParam::Type::TERNARY},
        {"valid", MatchKeyParam::Type::VALID},
        {"range", MatchKeyParam::Type::RANGE} };

  const Json::Value &cfg_pipelines = cfg_root["pipelines"];
  for (const auto &cfg_pipeline : cfg_pipelines) {
    const string pipeline_name = cfg_pipeline["name"].asString();
    p4object_id_t pipeline_id = cfg_pipeline["id"].asInt();

    // pipelines -> tables

    const Json::Value &cfg_tables = cfg_pipeline["tables"];
    for (const auto &cfg_table : cfg_tables) {
      const string table_name = cfg_table["name"].asString();
      p4object_id_t table_id = cfg_table["id"].asInt();

      MatchKeyBuilder key_builder;
      const Json::Value &cfg_match_key = cfg_table["key"];

      auto add_f = [this, &key_builder, &map_name_to_match_type](
          const Json::Value &cfg_f) {
        const Json::Value &cfg_key_field = cfg_f["target"];
        const string header_name = cfg_key_field[0].asString();
        header_id_t header_id = get_header_id(header_name);
        const string field_name = cfg_key_field[1].asString();
        int field_offset = get_field_offset(header_id, field_name);
        const auto mtype = map_name_to_match_type.at(
            cfg_f["match_type"].asString());
        const std::string name = header_name + "." + field_name;
        if ((!cfg_f.isMember("mask")) || cfg_f["mask"].isNull()) {
          key_builder.push_back_field(header_id, field_offset,
                                      get_field_bits(header_id, field_offset),
                                      mtype, name);
        } else {
          const Json::Value &cfg_key_mask = cfg_f["mask"];
          key_builder.push_back_field(header_id, field_offset,
                                      get_field_bits(header_id, field_offset),
                                      ByteContainer(cfg_key_mask.asString()),
                                      mtype, name);
        }
      };

      // sanity checking, really necessary?
      bool has_lpm = false;
      for (const auto &cfg_key_entry : cfg_match_key) {
        const string match_type = cfg_key_entry["match_type"].asString();
        if (match_type == "lpm") {
          if (has_lpm) {
            outstream << "Table " << table_name << "features 2 LPM match fields"
                      << std::endl;
            return 1;
          }
          has_lpm = true;
        }
      }

      for (const auto &cfg_key_entry : cfg_match_key) {
        const string match_type = cfg_key_entry["match_type"].asString();
        if (match_type == "valid") {
          const Json::Value &cfg_key_field = cfg_key_entry["target"];
          const string header_name = cfg_key_field.asString();
          header_id_t header_id = get_header_id(header_name);
          key_builder.push_back_valid_header(header_id, header_name);
        } else {
          add_f(cfg_key_entry);
        }
      }

      const string match_type = cfg_table["match_type"].asString();
      const string table_type = cfg_table["type"].asString();
      const int table_size = cfg_table["max_size"].asInt();
      const Json::Value false_value(false);
      // if attribute is missing, default is false
      const bool with_counters =
        cfg_table.get("with_counters", false_value).asBool();
      const bool with_ageing =
        cfg_table.get("support_timeout", false_value).asBool();

      // TODO(antonin): improve this to make it easier to create new kind of
      // tables e.g. like the register mechanism for primitives :)
      std::unique_ptr<MatchActionTable> table;
      if (table_type == "simple") {
        table = MatchActionTable::create_match_action_table<MatchTable>(
          match_type, table_name, table_id, table_size, key_builder,
          with_counters, with_ageing, lookup_factory);
      } else if (table_type == "indirect") {
        table = MatchActionTable::create_match_action_table<MatchTableIndirect>(
          match_type, table_name, table_id, table_size, key_builder,
          with_counters, with_ageing, lookup_factory);
      } else if (table_type == "indirect_ws") {
        table =
          MatchActionTable::create_match_action_table<MatchTableIndirectWS>(
            match_type, table_name, table_id, table_size, key_builder,
            with_counters, with_ageing, lookup_factory);

        if (!cfg_table.isMember("selector")) {
          assert(0 && "indirect_ws tables need to specify a selector");
        }
        const Json::Value &cfg_table_selector = cfg_table["selector"];
        const string selector_algo = cfg_table_selector["algo"].asString();
        const Json::Value &cfg_table_selector_input =
          cfg_table_selector["input"];

        BufBuilder builder;
        // TODO(antonin): I do this kind of thing in a bunch of places, I need
        // to find a nicer way
        for (const auto &cfg_element : cfg_table_selector_input) {
          const string type = cfg_element["type"].asString();
          assert(type == "field");  // TODO(antonin): other types

          const Json::Value &cfg_value_field = cfg_element["value"];
          const string header_name = cfg_value_field[0].asString();
          header_id_t header_id = get_header_id(header_name);
          const string field_name = cfg_value_field[1].asString();
          int field_offset = get_field_offset(header_id, field_name);
          builder.push_back_field(header_id, field_offset);
        }
        // typedef MatchTableIndirectWS::hash_t hash_t;
        // check algo
        if (!check_hash(selector_algo)) return 1;

        std::unique_ptr<Calculation> calc(
          new Calculation(builder, selector_algo));
        MatchTableIndirectWS *mt_indirect_ws =
          static_cast<MatchTableIndirectWS *>(table->get_match_table());
        mt_indirect_ws->set_hash(std::move(calc));
      } else {
        assert(0 && "invalid table type");
      }

      // maintains backwards compatibility
      if (cfg_table.isMember("direct_meters") &&
          !cfg_table["direct_meters"].isNull()) {
        const std::string meter_name = cfg_table["direct_meters"].asString();
        const DirectMeterArray &direct_meter = direct_meters[meter_name];
        table->get_match_table()->set_direct_meters(
            direct_meter.meter, direct_meter.header, direct_meter.offset);
      }

      if (with_ageing) ageing_monitor->add_table(table->get_match_table());

      add_match_action_table(table_name, std::move(table));
    }

    // pipelines -> conditionals

    const Json::Value &cfg_conditionals = cfg_pipeline["conditionals"];
    for (const auto &cfg_conditional : cfg_conditionals) {
      const string conditional_name = cfg_conditional["name"].asString();
      p4object_id_t conditional_id = cfg_conditional["id"].asInt();
      Conditional *conditional = new Conditional(conditional_name,
                                                 conditional_id);

      const Json::Value &cfg_expression = cfg_conditional["expression"];
      build_expression(cfg_expression, conditional);
      conditional->build();

      add_conditional(conditional_name, unique_ptr<Conditional>(conditional));
    }

    // next node resolution for tables

    for (const auto &cfg_table : cfg_tables) {
      const string table_name = cfg_table["name"].asString();
      MatchTableAbstract *table = get_abstract_match_table(table_name);

      const Json::Value &cfg_next_nodes = cfg_table["next_tables"];

      auto get_next_node = [this](const Json::Value &cfg_next_node)
          -> const ControlFlowNode *{
        if (cfg_next_node.isNull())
          return nullptr;
        return get_control_node(cfg_next_node.asString());
      };

      std::string actions_key = cfg_table.isMember("action_ids") ? "action_ids"
          : "actions";
      const Json::Value &cfg_actions = cfg_table[actions_key];
      for (const auto &cfg_action : cfg_actions) {
        p4object_id_t action_id = 0;
        string action_name = "";
        ActionFn *action = nullptr;
        if (actions_key == "action_ids") {
          action_id = cfg_action.asInt();
          action = get_action_by_id(action_id); assert(action);
          action_name = action->get_name();
        } else {
          action_name = cfg_action.asString();
          action = get_one_action_with_name(action_name); assert(action);
          action_id = action->get_id();
        }

        const Json::Value &cfg_next_node = cfg_next_nodes[action_name];
        const ControlFlowNode *next_node = get_next_node(cfg_next_node);
        table->set_next_node(action_id, next_node);
        add_action_to_table(table_name, action_name, action);
      }

      if (cfg_next_nodes.isMember("__HIT__"))
        table->set_next_node_hit(get_next_node(cfg_next_nodes["__HIT__"]));
      if (cfg_next_nodes.isMember("__MISS__"))
        table->set_next_node_miss(get_next_node(cfg_next_nodes["__MISS__"]));

      if (cfg_table.isMember("base_default_next")) {
        table->set_next_node_miss_default(
            get_next_node(cfg_table["base_default_next"]));
      }

      // for 'simple' tables, it is possible to specify a default action or a
      // default action with some default action data. It is also specify to
      // choose if the default action is "const", meaning it cannot be changed
      // by the control plane; or if the default entry (action + action data) is
      // "const".
      // note that this code has to be placed after setting the next nodes, as
      // it may call MatchTable::set_default_action.
      if (cfg_table.isMember("default_entry")) {
        const string table_type = cfg_table["type"].asString();
        if (table_type != "simple") {
          outstream << "Table '" << table_name << "' does not have type "
                    << "'simple' and therefore cannot specify a "
                    << "'default_entry' attribute\n";
          return 1;
        }

        auto simple_table = dynamic_cast<MatchTable *>(table);
        assert(simple_table);

        const Json::Value &cfg_default_entry = cfg_table["default_entry"];
        const p4object_id_t action_id = cfg_default_entry["action_id"].asInt();
        const ActionFn *action = get_action_by_id(action_id); assert(action);

        const Json::Value false_value(false);
        const bool is_action_const =
            cfg_default_entry.get("action_const", false_value).asBool();
        if (is_action_const) simple_table->set_const_default_action_fn(action);

        if (cfg_default_entry.isMember("action_data")) {
          const Json::Value &cfg_action_data = cfg_default_entry["action_data"];

          // TODO(antonin)
          // if (action->num_params() != cfg_action_data.size()) {
          //   outstream << "Default entry specification for table '"
          //             << table_name << "' incorrect: expected "
          //             << action->num_params() << " args for action data "
          //             << "but got " << cfg_action_data.size() << "\n";
          //   return 1;
          // }

          ActionData adata;
          for (const auto &d : cfg_action_data)
            adata.push_back_action_data(Data(d.asString()));

          const bool is_action_entry_const =
              cfg_default_entry.get("action_entry_const", false_value).asBool();

          simple_table->set_default_entry(action, std::move(adata),
                                          is_action_entry_const);
        }
      }
    }

    // next node resolution for conditionals

    for (const auto &cfg_conditional : cfg_conditionals) {
      const string conditional_name = cfg_conditional["name"].asString();
      Conditional *conditional = get_conditional(conditional_name);

      const Json::Value &cfg_true_next = cfg_conditional["true_next"];
      const Json::Value &cfg_false_next = cfg_conditional["false_next"];

      if (!cfg_true_next.isNull()) {
        ControlFlowNode *next_node = get_control_node(cfg_true_next.asString());
        conditional->set_next_node_if_true(next_node);
      }
      if (!cfg_false_next.isNull()) {
        ControlFlowNode *next_node = get_control_node(
          cfg_false_next.asString());
        conditional->set_next_node_if_false(next_node);
      }
    }

    ControlFlowNode *first_node = nullptr;
    if (!cfg_pipeline["init_table"].isNull()) {
      const string first_node_name = cfg_pipeline["init_table"].asString();
      first_node = get_control_node(first_node_name);
    }

    Pipeline *pipeline = new Pipeline(pipeline_name, pipeline_id, first_node);
    add_pipeline(pipeline_name, unique_ptr<Pipeline>(pipeline));
  }

  // checksums

  const Json::Value &cfg_checksums = cfg_root["checksums"];
  for (const auto &cfg_checksum : cfg_checksums) {
    const string checksum_name = cfg_checksum["name"].asString();
    p4object_id_t checksum_id = cfg_checksum["id"].asInt();
    const string checksum_type = cfg_checksum["type"].asString();

    const Json::Value &cfg_cksum_field = cfg_checksum["target"];
    const string header_name = cfg_cksum_field[0].asString();
    header_id_t header_id = get_header_id(header_name);
    const string field_name = cfg_cksum_field[1].asString();
    int field_offset = get_field_offset(header_id, field_name);

    Checksum *checksum;
    if (checksum_type == "ipv4") {
      checksum = new IPv4Checksum(checksum_name, checksum_id,
                                  header_id, field_offset);
    } else {
      assert(checksum_type == "generic");
      const string calculation_name = cfg_checksum["calculation"].asString();
      NamedCalculation *calculation = get_named_calculation(calculation_name);
      checksum = new CalcBasedChecksum(checksum_name, checksum_id,
                                       header_id, field_offset, calculation);
    }

    if (cfg_checksum.isMember("if_cond") && !cfg_checksum["if_cond"].isNull()) {
      auto cksum_condition = std::unique_ptr<Expression>(new Expression());
      build_expression(cfg_checksum["if_cond"], cksum_condition.get());
      cksum_condition->build();
      checksum->set_checksum_condition(std::move(cksum_condition));
    }

    checksums.push_back(unique_ptr<Checksum>(checksum));

    for (auto it = deparsers.begin(); it != deparsers.end(); ++it) {
      it->second->add_checksum(checksum);
    }
  }

  // learn lists

  learn_engine = std::unique_ptr<LearnEngine>(
      new LearnEngine(device_id, cxt_id));

  const Json::Value &cfg_learn_lists = cfg_root["learn_lists"];

  if (cfg_learn_lists.size() > 0) {
    assert(notifications_transport);
  }

  for (const auto &cfg_learn_list : cfg_learn_lists) {
    LearnEngine::list_id_t list_id = cfg_learn_list["id"].asInt();
    learn_engine->list_create(list_id, 16);  // 16 is max nb of samples
    learn_engine->list_set_learn_writer(list_id, notifications_transport);

    const Json::Value &cfg_learn_elements = cfg_learn_list["elements"];
    for (const auto &cfg_learn_element : cfg_learn_elements) {
      const string type = cfg_learn_element["type"].asString();
      assert(type == "field");  // TODO(antonin): other types

      const Json::Value &cfg_value_field = cfg_learn_element["value"];
      const string header_name = cfg_value_field[0].asString();
      header_id_t header_id = get_header_id(header_name);
      const string field_name = cfg_value_field[1].asString();
      int field_offset = get_field_offset(header_id, field_name);
      learn_engine->list_push_back_field(list_id, header_id, field_offset);
    }

    learn_engine->list_init(list_id);
  }

  const Json::Value &cfg_field_lists = cfg_root["field_lists"];
  // used only for cloning

  // TODO(antonin): some cleanup for learn lists / clone lists / calculation
  // lists
  for (const auto &cfg_field_list : cfg_field_lists) {
    p4object_id_t list_id = cfg_field_list["id"].asInt();
    std::unique_ptr<FieldList> field_list(new FieldList());
    const Json::Value &cfg_elements = cfg_field_list["elements"];

    for (const auto &cfg_element : cfg_elements) {
      const string type = cfg_element["type"].asString();
      assert(type == "field");  // TODO(antonin): other types

      const Json::Value &cfg_value_field = cfg_element["value"];
      const string header_name = cfg_value_field[0].asString();
      header_id_t header_id = get_header_id(header_name);
      const string field_name = cfg_value_field[1].asString();
      int field_offset = get_field_offset(header_id, field_name);
      field_list->push_back_field(header_id, field_offset);
    }

    add_field_list(list_id, std::move(field_list));
  }

  if (!check_required_fields(required_fields)) {
    return 1;
  }

  // force arith fields

  if (cfg_root.isMember("force_arith")) {
    const Json::Value &cfg_force_arith = cfg_root["force_arith"];

    for (const auto &cfg_field : cfg_force_arith) {
      const auto field = field_info(cfg_field[0].asString(),
                                    cfg_field[1].asString());
      phv_factory.enable_field_arith(std::get<0>(field), std::get<1>(field));
    }
  }

  for (const auto &p : arith_objects.fields) {
    if (!field_exists(p.first, p.second)) {
      continue;
      // TODO(antonin): skipping warning for now, but maybe it would be nice to
      // leave the choice to the person calling force_arith_field
      // outstream << "field " << p.first << "." << p.second
      //           << " does not exist but required for arith, ignoring\n";
    } else {
      const auto field = field_info(p.first, p.second);
      phv_factory.enable_field_arith(std::get<0>(field), std::get<1>(field));
    }
  }

  for (const auto &h : arith_objects.headers) {
    if (!header_exists(h)) continue;
    // safe to call because we just checked for the header existence
    const header_id_t header_id = get_header_id(h);
    phv_factory.enable_all_field_arith(header_id);
  }

  return 0;
  // obviously this function is very long, but it is doing a very dumb job...
  // NOLINTNEXTLINE(readability/fn_size)
}

void
P4Objects::reset_state() {
  // TODO(antonin): is this robust?
  for (const auto &e : match_action_tables_map) {
    e.second->get_match_table()->reset_state();
  }
  for (const auto &e : meter_arrays) {
    e.second->reset_state();
  }
  for (const auto &e : counter_arrays) {
    e.second->reset_state();
  }
  for (const auto &e : register_arrays) {
    e.second->reset_state();
  }
  learn_engine->reset_state();
  ageing_monitor->reset_state();
}

void
P4Objects::serialize(std::ostream *out) const {
  for (const auto &e : match_action_tables_map) {
    e.second->get_match_table()->serialize(out);
  }
  for (const auto &e : meter_arrays) {
    e.second->serialize(out);
  }
}

void
P4Objects::deserialize(std::istream *in) {
  for (const auto &e : match_action_tables_map) {
    e.second->get_match_table()->deserialize(in, *this);
  }
  for (const auto &e : meter_arrays) {
    e.second->deserialize(in);
  }
}

int
P4Objects::get_field_offset(header_id_t header_id, const string &field_name) {
  const HeaderType &header_type = phv_factory.get_header_type(header_id);
  return header_type.get_field_offset(field_name);
}

size_t
P4Objects::get_field_bytes(header_id_t header_id, int field_offset) {
  const HeaderType &header_type = phv_factory.get_header_type(header_id);
  return (header_type.get_bit_width(field_offset) + 7) / 8;
}

size_t
P4Objects::get_field_bits(header_id_t header_id, int field_offset) {
  const HeaderType &header_type = phv_factory.get_header_type(header_id);
  return header_type.get_bit_width(field_offset);
}

size_t
P4Objects::get_header_bits(header_id_t header_id) {
  const HeaderType &header_type = phv_factory.get_header_type(header_id);
  return header_type.get_bit_width();
}

std::tuple<header_id_t, int>
P4Objects::field_info(const string &header_name, const string &field_name) {
  auto it = field_aliases.find(header_name + "." + field_name);
  if (it != field_aliases.end())
    return field_info(std::get<0>(it->second), std::get<1>(it->second));
  header_id_t header_id = get_header_id(header_name);
  return std::make_tuple(header_id, get_field_offset(header_id, field_name));
}

bool
P4Objects::field_exists(const string &header_name,
                        const string &field_name) const {
  // TODO(antonin): the alias feature, while correcty-handled during packet
  // processing in phv.cpp is kind of hacky here and needs to be improved.
  if (field_aliases.find(header_name + "." + field_name) != field_aliases.end())
    return true;
  const auto it = header_to_type_map.find(header_name);
  if (it == header_to_type_map.end()) return false;
  const HeaderType *header_type = it->second;
  return (header_type->get_field_offset(field_name) != -1);
}

bool
P4Objects::header_exists(const string &header_name) const {
  return header_to_type_map.find(header_name) != header_to_type_map.end();
}

bool
P4Objects::check_required_fields(
    const std::set<header_field_pair> &required_fields) {
  bool res = true;
  for (const auto &p : required_fields) {
    if (!field_exists(p.first, p.second)) {
      res = false;
      outstream << "Field " << p.first << "." << p.second
                << " is required by switch target but is not defined\n";
    }
  }
  return res;
}

std::unique_ptr<CalculationsMap::MyC>
P4Objects::check_hash(const std::string &name) const {
  auto h = CalculationsMap::get_instance()->get_copy(name);
  if (!h) outstream << "Unknown hash algorithm: " << name  << std::endl;
  return h;
}

MeterArray *
P4Objects::get_meter_array_rt(const std::string &name) const {
  auto it = meter_arrays.find(name);
  return (it != meter_arrays.end()) ? it->second.get() : nullptr;
}

CounterArray *
P4Objects::get_counter_array_rt(const std::string &name) const {
  auto it = counter_arrays.find(name);
  return (it != counter_arrays.end()) ? it->second.get() : nullptr;
}

RegisterArray *
P4Objects::get_register_array_rt(const std::string &name) const {
  auto it = register_arrays.find(name);
  return (it != register_arrays.end()) ? it->second.get() : nullptr;
}

ParseVSet *
P4Objects::get_parse_vset_rt(const std::string &name) const {
  auto it = parse_vsets.find(name);
  return (it != parse_vsets.end()) ? it->second.get() : nullptr;
}

}  // namespace bm
