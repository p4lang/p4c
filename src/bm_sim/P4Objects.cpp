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
#include <bm/bm_sim/phv.h>

#include <istream>
#include <ostream>
#include <string>
#include <tuple>
#include <vector>
#include <set>
#include <exception>

#include "jsoncpp/json.h"

namespace bm {

using std::unique_ptr;
using std::string;

namespace {

template <typename T,
          typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
T hexstr_to_int(const std::string &hexstr) {
  // TODO(antonin): temporary trick to avoid code duplication
  return Data(hexstr).get<T>();
}

class json_exception : public std::exception {
 public:
  json_exception(const std::string &error_string, const Json::Value &json_value)
      : error_string(error_string), json_value(json_value), with_json(true) { }

  explicit json_exception(const std::string &error_string)
      : error_string(error_string) { }

  const std::string msg(bool verbose) const {
    std::string verbose_string(error_string);
    if (verbose && with_json) {
      verbose_string += std::string("\nbad json:\n");
      verbose_string += json_value.toStyledString();
    }
    verbose_string += std::string("\n");
    return verbose_string;
  }

  const char *what() const noexcept override {
    return error_string.c_str();
  }

 private:
  std::string error_string;
  Json::Value json_value{};
  bool with_json{false};
};

// Inspired from
// http://stackoverflow.com/questions/12261915/howto-throw-stdexceptions-with-variable-messages
class ExceptionFormatter {
 public:
  ExceptionFormatter() = default;

  template <typename Type>
  ExceptionFormatter &operator <<(const Type &value) {
    stream_ << value;
    return *this;
  }

  std::string str() const { return stream_.str(); }
  // implicit conversion to string
  operator std::string() const { return stream_.str(); }

  ExceptionFormatter(const ExceptionFormatter &other) = delete;
  ExceptionFormatter &operator=(const ExceptionFormatter &other) = delete;

  ExceptionFormatter(ExceptionFormatter &&other) = delete;
  ExceptionFormatter &operator=(ExceptionFormatter &&other) = delete;

 private:
  std::stringstream stream_{};
};

using EFormat = ExceptionFormatter;

}  // namespace


enum class P4Objects::ExprType { UNKNOWN, DATA, HEADER, HEADER_STACK, BOOL };

void
P4Objects::build_expression(const Json::Value &json_expression,
                            Expression *expr) {
  ExprType expr_type(ExprType::UNKNOWN);
  build_expression(json_expression, expr, &expr_type);
}

namespace {

using ExprType = P4Objects::ExprType;

ExprOpcode get_eq_opcode(ExprType expr_type) {
  switch (expr_type) {
    case ExprType::DATA:
      return ExprOpcode::EQ_DATA;
    case ExprType::HEADER:
      return ExprOpcode::EQ_HEADER;
    case ExprType::BOOL:
      return ExprOpcode::EQ_BOOL;
    default:
      break;
  }
  assert(0);
  return ExprOpcode::EQ_DATA;
}

ExprOpcode get_neq_opcode(ExprType expr_type) {
  switch (expr_type) {
    case ExprType::DATA:
      return ExprOpcode::NEQ_DATA;
    case ExprType::HEADER:
      return ExprOpcode::NEQ_HEADER;
    case ExprType::BOOL:
      return ExprOpcode::NEQ_BOOL;
    default:
      break;
  }
  assert(0);
  return ExprOpcode::NEQ_DATA;
}

// TODO(antonin): should this information be in expressions.h?
ExprType get_opcode_type(ExprOpcode opcode) {
  switch (opcode) {
    case ExprOpcode::LOAD_FIELD:
    case ExprOpcode::LOAD_CONST:
    case ExprOpcode::LOAD_LOCAL:
    case ExprOpcode::LOAD_REGISTER_REF:
    case ExprOpcode::LOAD_REGISTER_GEN:
    case ExprOpcode::LOAD_LAST_HEADER_STACK_FIELD:
    case ExprOpcode::ADD:
    case ExprOpcode::SUB:
    case ExprOpcode::MOD:
    case ExprOpcode::DIV:
    case ExprOpcode::MUL:
    case ExprOpcode::SHIFT_LEFT:
    case ExprOpcode::SHIFT_RIGHT:
    case ExprOpcode::BIT_AND:
    case ExprOpcode::BIT_OR:
    case ExprOpcode::BIT_XOR:
    case ExprOpcode::BIT_NEG:
    case ExprOpcode::TWO_COMP_MOD:
    case ExprOpcode::BOOL_TO_DATA:
    case ExprOpcode::LAST_STACK_INDEX:
    case ExprOpcode::SIZE_STACK:
    case ExprOpcode::ACCESS_FIELD:
      return ExprType::DATA;
    case ExprOpcode::LOAD_BOOL:
    case ExprOpcode::EQ_DATA:
    case ExprOpcode::NEQ_DATA:
    case ExprOpcode::GT_DATA:
    case ExprOpcode::LT_DATA:
    case ExprOpcode::GET_DATA:
    case ExprOpcode::LET_DATA:
    case ExprOpcode::EQ_HEADER:
    case ExprOpcode::NEQ_HEADER:
    case ExprOpcode::EQ_BOOL:
    case ExprOpcode::NEQ_BOOL:
    case ExprOpcode::AND:
    case ExprOpcode::OR:
    case ExprOpcode::NOT:
    case ExprOpcode::VALID_HEADER:
    case ExprOpcode::DATA_TO_BOOL:
      return ExprType::BOOL;
    case ExprOpcode::LOAD_HEADER:
    case ExprOpcode::DEREFERENCE_STACK:
      return ExprType::HEADER;
    case ExprOpcode::LOAD_HEADER_STACK:
      return ExprType::HEADER_STACK;
    case ExprOpcode::TERNARY_OP:
      return ExprType::UNKNOWN;
    case ExprOpcode::SKIP:
      break;
  }
  assert(0);
  return ExprType::UNKNOWN;
}

}  // namespace

void
P4Objects::build_expression(const Json::Value &json_expression,
                            Expression *expr, ExprType *expr_type) {
  *expr_type = ExprType::UNKNOWN;
  if (json_expression.isNull()) return;
  const auto type = json_expression["type"].asString();
  const auto &json_value = json_expression["value"];
  if (type == "expression") {
    const auto op = json_value["op"].asString();
    const auto &json_left = json_value["left"];
    const auto &json_right = json_value["right"];
    ExprType typeL, typeR;

    if (op == "?") {
      const auto &json_cond = json_value["cond"];
      build_expression(json_cond, expr);

      Expression e1, e2;
      build_expression(json_left, &e1, &typeL);
      build_expression(json_right, &e2, &typeR);
      assert(typeL == typeR);
      expr->push_back_ternary_op(e1, e2);
      *expr_type = typeL;
    } else if (op == "==" || op == "!=") {
      build_expression(json_left, expr, &typeL);
      build_expression(json_right, expr, &typeR);
      assert(typeL == typeR);
      assert(typeL != ExprType::UNKNOWN);
      auto opcode = (op == "==") ? get_eq_opcode(typeL) : get_neq_opcode(typeL);
      expr->push_back_op(opcode);
      *expr_type = ExprType::BOOL;
    } else if (op == "access_field") {
      build_expression(json_left, expr);
      expr->push_back_access_field(json_right.asInt());
    } else {
      // special handling for unary + and -, we set the left operand to 0
      if ((op == "+" || op == "-") && json_left.isNull())
        expr->push_back_load_const(Data(0));
      else
        build_expression(json_left, expr);
      build_expression(json_right, expr);

      auto opcode = ExprOpcodesMap::get_opcode(op);
      expr->push_back_op(opcode);
      *expr_type = get_opcode_type(opcode);
    }
  } else if (type == "header") {
    auto header_id = get_header_id(json_value.asString());
    expr->push_back_load_header(header_id);
    *expr_type = ExprType::HEADER;

    enable_arith(header_id);
  } else if (type == "field") {
    const auto header_name = json_value[0].asString();
    auto header_id = get_header_id(header_name);
    const auto field_name = json_value[1].asString();
    int field_offset = get_field_offset(header_id, field_name);
    expr->push_back_load_field(header_id, field_offset);
    *expr_type = ExprType::DATA;

    enable_arith(header_id, field_offset);
  } else if (type == "bool") {
    expr->push_back_load_bool(json_value.asBool());
    *expr_type = ExprType::BOOL;
  } else if (type == "hexstr") {
    expr->push_back_load_const(Data(json_value.asString()));
    *expr_type = ExprType::DATA;
  } else if (type == "local") {  // runtime data for expressions in actions
    expr->push_back_load_local(json_value.asInt());
    *expr_type = ExprType::DATA;
  } else if (type == "register") {
    // TODO(antonin): cheap optimization
    // this may not be worth doing, and probably does not belong here
    const auto register_array_name = json_value[0].asString();
    const auto &json_index = json_value[1];
    assert(json_index.size() == 2);
    if (json_index["type"].asString() == "hexstr") {
      const auto idx = hexstr_to_int<unsigned int>(
          json_index["value"].asString());
      expr->push_back_load_register_ref(
          get_register_array(register_array_name), idx);
    } else {
      build_expression(json_index, expr);
      expr->push_back_load_register_gen(
          get_register_array(register_array_name));
    }
    *expr_type = ExprType::DATA;
  } else if (type == "header_stack") {
    auto header_stack_id = get_header_stack_id(json_value.asString());
    expr->push_back_load_header_stack(header_stack_id);
    *expr_type = ExprType::HEADER_STACK;

    phv_factory.enable_all_stack_field_arith(header_stack_id);
  } else if (type == "stack_field") {
    const auto header_stack_name = json_value[0].asString();
    auto header_stack_id = get_header_stack_id(header_stack_name);
    auto *header_type = header_stack_to_type_map[header_stack_name];
    const auto field_name = json_value[1].asString();
    int field_offset = header_type->get_field_offset(field_name);
    expr->push_back_load_last_header_stack_field(header_stack_id, field_offset);
    *expr_type = ExprType::DATA;

    phv_factory.enable_stack_field_arith(header_stack_id, field_offset);
  } else {
    throw json_exception(
        EFormat() << "Invalid 'type' in expression: '" << type << "'",
        json_expression);
  }
}

int
P4Objects::add_primitive_to_action(const Json::Value &cfg_primitive,
                                   ActionFn *action_fn) {
  const auto primitive_name = cfg_primitive["op"].asString();

  auto primitive = get_primitive(primitive_name);
  if (!primitive)
    throw json_exception(
        EFormat() << "Unknown primitive action: " << primitive_name);

  action_fn->push_back_primitive(primitive);

  const auto &cfg_primitive_parameters = cfg_primitive["parameters"];

  // check number of parameters
  const size_t num_params_expected = primitive->get_num_params();
  const size_t num_params = cfg_primitive_parameters.size();
  if (num_params != num_params_expected) {
    // should not happen with a good compiler
    throw json_exception(
        EFormat() << "Invalid number of parameters for primitive action "
                  << primitive_name << ": expected " << num_params_expected
                  << " but got " << num_params,
        cfg_primitive);
  }

  for (const auto &cfg_parameter : cfg_primitive_parameters) {
    const auto type = cfg_parameter["type"].asString();

    if (type == "hexstr") {
      const auto value_hexstr = cfg_parameter["value"].asString();
      action_fn->parameter_push_back_const(Data(value_hexstr));
    } else if (type == "runtime_data") {
      auto action_data_offset = cfg_parameter["value"].asUInt();
      auto runtime_data_size = action_fn->get_num_params();
      if (action_data_offset >= runtime_data_size) {
        throw json_exception(
            EFormat() << "Invalid 'runtime_data' parameter reference in "
                      << "action '" << action_fn->get_name() << "' when "
                      << "calling primitive '" << primitive_name << "': trying "
                      << "to use parameter at offset " << action_data_offset
                      << " but action only has " << runtime_data_size
                      << " parameter(s)",
            cfg_parameter);
      }
      action_fn->parameter_push_back_action_data(action_data_offset);
    } else if (type == "header") {
      const auto header_name = cfg_parameter["value"].asString();
      auto header_id = get_header_id(header_name);
      action_fn->parameter_push_back_header(header_id);

      enable_arith(header_id);
    } else if (type == "field") {
      const auto &cfg_value_field = cfg_parameter["value"];
      const auto header_name = cfg_value_field[0].asString();
      auto header_id = get_header_id(header_name);
      const auto field_name = cfg_value_field[1].asString();
      auto field_offset = get_field_offset(header_id, field_name);
      action_fn->parameter_push_back_field(header_id, field_offset);

      enable_arith(header_id, field_offset);
    } else if (type == "calculation") {
      const auto name = cfg_parameter["value"].asString();
      auto calculation = get_named_calculation(name);
      action_fn->parameter_push_back_calculation(calculation);
    } else if (type == "meter_array") {
      const auto name = cfg_parameter["value"].asString();
      auto meter = get_meter_array(name);
      action_fn->parameter_push_back_meter_array(meter);
    } else if (type == "counter_array") {
      const auto name = cfg_parameter["value"].asString();
      auto counter = get_counter_array(name);
      action_fn->parameter_push_back_counter_array(counter);
    } else if (type == "register_array") {
      const auto name = cfg_parameter["value"].asString();
      auto register_array = get_register_array(name);
      action_fn->parameter_push_back_register_array(register_array);
    } else if (type == "header_stack") {
      const auto header_stack_name = cfg_parameter["value"].asString();
      auto header_stack_id = get_header_stack_id(header_stack_name);
      action_fn->parameter_push_back_header_stack(header_stack_id);
    } else if (type == "expression") {
      // TODO(Antonin): should this make the field case (and other) obsolete
      // maybe if we can optimize this case
      auto expr = new ArithExpression();
      build_expression(cfg_parameter["value"], expr);
      expr->build();
      action_fn->parameter_push_back_expression(
          std::unique_ptr<ArithExpression>(expr));
    } else if (type == "register") {
      // TODO(antonin): cheap optimization
      // this may not be worth doing, and probably does not belong here
      const auto &cfg_register = cfg_parameter["value"];
      const auto register_array_name = cfg_register[0].asString();
      auto &json_index = cfg_register[1];
      assert(json_index.size() == 2);
      if (json_index["type"].asString() == "hexstr") {
        const auto idx = hexstr_to_int<unsigned int>(
            json_index["value"].asString());
        action_fn->parameter_push_back_register_ref(
            get_register_array(register_array_name), idx);
      } else {
        auto idx_expr = new ArithExpression();
        build_expression(json_index, idx_expr);
        idx_expr->build();
        action_fn->parameter_push_back_register_gen(
            get_register_array(register_array_name),
            std::unique_ptr<ArithExpression>(idx_expr));
      }
    } else if (type == "extern") {
      const auto name = cfg_parameter["value"].asString();
      auto extern_instance = get_extern_instance(name);
      action_fn->parameter_push_back_extern_instance(extern_instance);
    } else if (type == "string") {
      action_fn->parameter_push_back_string(cfg_parameter["value"].asString());
    } else if (type == "stack_field") {
      const auto &cfg_value = cfg_parameter["value"];
      const auto header_stack_name = cfg_value[0].asString();
      auto header_stack_id = get_header_stack_id(header_stack_name);
      auto *header_type = header_stack_to_type_map[header_stack_name];
      const auto field_name = cfg_value[1].asString();
      auto field_offset = header_type->get_field_offset(field_name);
      action_fn->parameter_push_back_last_header_stack_field(
          header_stack_id, field_offset);

      phv_factory.enable_stack_field_arith(header_stack_id, field_offset);
    } else {
      throw json_exception(
          EFormat() << "Parameter type '" << type << "' not supported",
          cfg_parameter);
      return 1;
    }
  }
  return 0;
}

void
P4Objects::parse_config_options(const Json::Value &root) {
  if (!root.isMember("config_options")) return;
  const auto &cfg_options = root["config_options"];
  for (auto it = cfg_options.begin(); it != cfg_options.end(); ++it) {
    auto key = it.key().asString();
    auto v = it->asString();
    config_options.emplace(key, v);
  }
}

namespace {

struct DirectMeterArray {
  MeterArray *meter;
  header_id_t header;
  int offset;
};

}  // namespace

struct P4Objects::InitState {
  std::unordered_map<std::string, DirectMeterArray> direct_meters{};
};

void
P4Objects::init_enums(const Json::Value &cfg_root) {
  const auto &cfg_enums = cfg_root["enums"];
  for (const auto &cfg_enum : cfg_enums) {
    auto enum_name = cfg_enum["name"].asString();
    auto enum_added = enums.add_enum(enum_name);
    if (!enum_added)
      throw json_exception("Invalid enums specification in json", cfg_enums);
    const auto &cfg_enum_entries = cfg_enum["entries"];
    for (const auto &cfg_enum_entry : cfg_enum_entries) {
      auto entry_name = cfg_enum_entry[0].asString();
      auto value = static_cast<EnumMap::type_t>(cfg_enum_entry[1].asInt());
      auto entry_added = enums.add_entry(enum_name, entry_name, value);
      if (!entry_added)
        throw json_exception("Invalid enums specification in json", cfg_enum);
    }
  }
}

void
P4Objects::init_header_types(const Json::Value &cfg_root) {
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
        std::unique_ptr<VLHeaderExpression> VL_expr(nullptr);
        if (!cfg_header_type.isMember("length_exp")) {
          const Json::Value &cfg_length_exp = cfg_header_type["length_exp"];
          ArithExpression raw_expr;
          build_expression(cfg_length_exp, &raw_expr);
          raw_expr.build();
          VL_expr.reset(new VLHeaderExpression(raw_expr));
        }
        int max_header_bytes = 0;
        if (!cfg_header_type.isMember("max_length"))
          max_header_bytes = cfg_header_type["max_length"].asInt();
        header_type->push_back_VL_field(
            field_name, max_header_bytes, std::move(VL_expr), is_signed);
      } else {
        int field_bit_width = cfg_field[1].asInt();
        header_type->push_back_field(field_name, field_bit_width, is_signed);
      }
    }

    add_header_type(header_type_name, unique_ptr<HeaderType>(header_type));
  }
}

void
P4Objects::init_headers(const Json::Value &cfg_root) {
  const Json::Value &cfg_headers = cfg_root["headers"];

  for (const auto &cfg_header : cfg_headers) {
    const string header_name = cfg_header["name"].asString();
    const string header_type_name = cfg_header["header_type"].asString();
    header_id_t header_id = cfg_header["id"].asInt();
    bool metadata = cfg_header["metadata"].asBool();

    HeaderType *header_type = get_header_type(header_type_name);
    header_to_type_map[header_name] = header_type;

    phv_factory.push_back_header(header_name, header_id,
                                 *header_type, metadata);
    phv_factory.disable_all_field_arith(header_id);
    add_header_id(header_name, header_id);
  }
}

void
P4Objects::init_header_stacks(const Json::Value &cfg_root) {
  const Json::Value &cfg_header_stacks = cfg_root["header_stacks"];

  for (const auto &cfg_header_stack : cfg_header_stacks) {
    const string header_stack_name = cfg_header_stack["name"].asString();
    const string header_type_name = cfg_header_stack["header_type"].asString();
    header_stack_id_t header_stack_id = cfg_header_stack["id"].asInt();

    HeaderType *header_stack_type = get_header_type(header_type_name);
    header_stack_to_type_map[header_stack_name] = header_stack_type;

    std::vector<header_id_t> header_ids;
    for (const auto &cfg_header_id : cfg_header_stack["header_ids"]) {
      header_ids.push_back(cfg_header_id.asInt());
      header_id_to_stack_id[cfg_header_id.asInt()] = header_stack_id;
    }

    phv_factory.push_back_header_stack(header_stack_name, header_stack_id,
                                       *header_stack_type, header_ids);
    add_header_stack_id(header_stack_name, header_stack_id);
  }
}

void
P4Objects::init_extern_instances(const Json::Value &cfg_root) {
  const Json::Value &cfg_extern_instances = cfg_root["extern_instances"];
  for (const auto &cfg_extern_instance : cfg_extern_instances) {
    const string extern_instance_name = cfg_extern_instance["name"].asString();
    const p4object_id_t extern_instance_id = cfg_extern_instance["id"].asInt();

    const string extern_type_name = cfg_extern_instance["type"].asString();
    auto instance = ExternFactoryMap::get_instance()->get_extern_instance(
        extern_type_name);
    if (instance == nullptr) {
      throw json_exception(
          EFormat() << "Invalid reference to extern type '"
                    << extern_type_name << "'",
          cfg_extern_instance);
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
        throw json_exception(
            EFormat() << "Extern type '" << extern_type_name
                      << "' has no attribute '" << name << "'",
            cfg_extern_attribute);
      }

      if (type == "hexstr") {
        const string value_hexstr = cfg_extern_attribute["value"].asString();
        instance->_set_attribute<Data>(name, Data(value_hexstr));
      } else if (type == "string") {
        // adding string to possible attributes for more flexible externs
        // (especially now that externs can access P4Objects), while we finalize
        // the extern design
        const string value_string = cfg_extern_attribute["value"].asString();
        instance->_set_attribute<std::string>(name, value_string);
      } else if (type == "expression") {
        Expression expr;
        build_expression(cfg_extern_attribute["value"], &expr);
        expr.build();
        // we rely on the fact that an Expression can be copied
        instance->_set_attribute<Expression>(name, expr);
      } else {
        throw json_exception("Only attributes of type 'hexstr', 'string' or"
                             " 'expression' are supported for extern instance"
                             " attribute initialization", cfg_extern_attribute);
      }
    }

    // needs to be set before the call to init!
    instance->_set_p4objects(this);

    add_extern_instance(extern_instance_name, std::move(instance));
  }
}

void
P4Objects::init_parse_vsets(const Json::Value &cfg_root) {
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
}

void
P4Objects::init_errors(const Json::Value &cfg_root) {
  const auto &cfg_errors = cfg_root["errors"];
  for (const auto &cfg_error : cfg_errors) {
    auto name = cfg_error[0].asString();
    auto value = static_cast<ErrorCode::type_t>(cfg_error[1].asInt());
    auto added = error_codes.add(name, value);
    if (!added)
      throw json_exception("Invalid errors specification in json", cfg_errors);
  }
  error_codes.add_core();
}

void
P4Objects::init_parsers(const Json::Value &cfg_root) {
  const Json::Value &cfg_parsers = cfg_root["parsers"];
  for (const auto &cfg_parser : cfg_parsers) {
    const string parser_name = cfg_parser["name"].asString();
    p4object_id_t parser_id = cfg_parser["id"].asInt();

    std::unique_ptr<Parser> parser(
        new Parser(parser_name, parser_id, &error_codes));

    std::unordered_map<string, ParseState *> current_parse_states;

    // parse states

    const Json::Value &cfg_parse_states = cfg_parser["parse_states"];
    for (const auto &cfg_parse_state : cfg_parse_states) {
      const string parse_state_name = cfg_parse_state["name"].asString();
      const p4object_id_t id = cfg_parse_state["id"].asInt();
      // p4object_id_t parse_state_id = cfg_parse_state["id"].asInt();
      std::unique_ptr<ParseState> parse_state(
          new ParseState(parse_state_name, id));

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
            const auto &header_type = phv_factory.get_header_type(header_id);
            if (header_type.is_VL_header() && !header_type.has_VL_expr()) {
              throw json_exception(
                  EFormat() << "Cannot use 'extract' parser op with a VL header"
                            << " for which no length expression was provided,"
                            << " use 'extract_VL' instead",
                  cfg_parser_op);
            }
            parse_state->add_extract(header_id);
          } else if (extract_type == "stack") {
            header_stack_id_t header_stack_id =
              get_header_stack_id(extract_header);
            parse_state->add_extract_to_stack(header_stack_id);
          } else {
            throw json_exception(
                EFormat() << "Extract type '" << extract_type
                          << "' not supported",
                cfg_parser_op);
          }
        } else if (op_type == "extract_VL") {
          assert(cfg_parameters.size() == 2);
          const Json::Value &cfg_extract = cfg_parameters[0];
          const Json::Value &cfg_VL_expr = cfg_parameters[1];
          assert(cfg_extract["type"].asString() == "regular");
          assert(cfg_VL_expr["type"].asString() == "expression");
          auto header_id = get_header_id(cfg_extract["value"].asString());
          const auto &header_type = phv_factory.get_header_type(header_id);
          ArithExpression VL_expr;
          build_expression(cfg_VL_expr["value"], &VL_expr);
          VL_expr.build();
          parse_state->add_extract_VL(header_id, VL_expr,
                                      header_type.get_VL_max_header_bytes());
        } else if (op_type == "set") {
          assert(cfg_parameters.size() == 2);
          const Json::Value &cfg_dest = cfg_parameters[0];
          const Json::Value &cfg_src = cfg_parameters[1];

          const string &dest_type = cfg_dest["type"].asString();
          assert(dest_type == "field");
          const auto dest = field_info(cfg_dest["value"][0].asString(),
                                       cfg_dest["value"][1].asString());
          enable_arith(std::get<0>(dest), std::get<1>(dest));

          const string &src_type = cfg_src["type"].asString();
          if (src_type == "field") {
            const auto src = field_info(cfg_src["value"][0].asString(),
                                        cfg_src["value"][1].asString());
            parse_state->add_set_from_field(
              std::get<0>(dest), std::get<1>(dest),
              std::get<0>(src), std::get<1>(src));
            enable_arith(std::get<0>(src), std::get<1>(src));
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
        } else if (op_type == "verify") {
          assert(cfg_parameters.size() == 2);
          BoolExpression cond_expr;
          build_expression(cfg_parameters[0], &cond_expr);
          cond_expr.build();
          ArithExpression error_expr;
          const auto &j_error_expr = cfg_parameters[1];
          if (j_error_expr.isInt()) {  // backward-compatibility
            auto error_code =  static_cast<ErrorCode::type_t>(
                j_error_expr.asInt());
            if (!error_codes.exists(error_code)) {
              throw json_exception("Invalid error code in verify statement",
                                   cfg_parser_op);
            }
            error_expr.push_back_load_const(Data(error_code));
          } else {
            build_expression(j_error_expr, &error_expr);
          }
          error_expr.build();
          parse_state->add_verify(cond_expr, error_expr);
        } else if (op_type == "primitive") {
          assert(cfg_parameters.size() == 1);
          const auto primitive_name = cfg_parameters[0]["op"].asString();
          std::unique_ptr<ActionFn> action_fn(new ActionFn(
              primitive_name, 0, 0));
          add_primitive_to_action(cfg_parameters[0], action_fn.get());
          parse_state->add_method_call(action_fn.get());
          parse_methods.push_back(std::move(action_fn));
        } else if (op_type == "shift") {
          assert(cfg_parameters.size() == 1);
          auto shift_bytes = static_cast<size_t>(cfg_parameters[0].asInt());
          parse_state->add_shift(shift_bytes);
        } else {
          throw json_exception(
              EFormat() << "Parser op '" << op_type << "'not supported",
              cfg_parser_op);
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

      current_parse_states[parse_state_name] = parse_state.get();
      parse_states.push_back(std::move(parse_state));
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

    add_parser(parser_name, std::move(parser));
  }
}

void
P4Objects::init_deparsers(const Json::Value &cfg_root) {
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
}

void
P4Objects::init_calculations(const Json::Value &cfg_root) {
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
      } else {
        throw json_exception(
            EFormat() << "Unsupported input type '" << type
                      << "' for calculation '" << name << "'", cfg_field);
      }
    }

    check_hash(algo);

    auto calculation = new NamedCalculation(name, id, builder, algo);
    add_named_calculation(name, unique_ptr<NamedCalculation>(calculation));
  }
}

void
P4Objects::init_counter_arrays(const Json::Value &cfg_root) {
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
}

void
P4Objects::init_meter_arrays(const Json::Value &cfg_root,
                             InitState *init_state) {
  auto &direct_meters = init_state->direct_meters;

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
}

void
P4Objects::init_register_arrays(const Json::Value &cfg_root) {
  const Json::Value &cfg_register_arrays = cfg_root["register_arrays"];
  for (const auto &cfg_register_array : cfg_register_arrays) {
    const string name = cfg_register_array["name"].asString();
    const p4object_id_t id = cfg_register_array["id"].asInt();
    const size_t size = cfg_register_array["size"].asUInt();
    const int bitwidth = cfg_register_array["bitwidth"].asInt();

    RegisterArray *register_array = new RegisterArray(name, id, size, bitwidth);
    add_register_array(name, unique_ptr<RegisterArray>(register_array));
  }
}

void
P4Objects::init_actions(const Json::Value &cfg_root) {
  const Json::Value &cfg_actions = cfg_root["actions"];
  for (const auto &cfg_action : cfg_actions) {
    const string action_name = cfg_action["name"].asString();
    p4object_id_t action_id = cfg_action["id"].asInt();
    std::unique_ptr<ActionFn> action_fn(new ActionFn(
        action_name, action_id, cfg_action["runtime_data"].size()));

    const auto &cfg_primitive_calls = cfg_action["primitives"];
    for (const auto &cfg_primitive_call : cfg_primitive_calls)
      add_primitive_to_action(cfg_primitive_call, action_fn.get());

    add_action(action_id, std::move(action_fn));
  }
}

namespace {

int get_table_size(const Json::Value &cfg_table) {
  if (cfg_table.isMember("max_size") && !cfg_table.isMember("entries"))
    return cfg_table["max_size"].asInt();
  if (!cfg_table.isMember("max_size") && cfg_table.isMember("entries"))
    return static_cast<int>(cfg_table["entries"].size());
  if (!cfg_table.isMember("max_size") && !cfg_table.isMember("entries")) {
    throw json_exception(
        EFormat() << "Table '" << cfg_table["name"].asString()
                  << "' has neither 'max_size' nor 'entries' attribute",
        cfg_table);
  }
  // both "max_size" and "entries"
  const auto max_size = cfg_table["max_size"].asInt();
  const auto entries_size = static_cast<int>(cfg_table["entries"].size());
  // TODO(antonin): print a warning if max_size != entries_size?
  (void) max_size;
  // we choose entries_size, because the table will be immutable anyway
  return entries_size;
}

ActionData parse_action_data(const ActionFn *action,
                             const Json::Value &cfg_action_data) {
  if (action->get_num_params() != cfg_action_data.size()) {
    throw json_exception(
        EFormat() << "Invalid number of arguments for action '"
                  << action->get_name() << "': expected "
                  << action->get_num_params() << " but got "
                  << cfg_action_data.size(),
        cfg_action_data);
  }
  ActionData adata;
  for (const auto &d : cfg_action_data)
    adata.push_back_action_data(Data(d.asString()));
  return adata;
}

MatchKeyParam::Type match_name_to_match_type(const std::string &name) {
  std::unordered_map<std::string, MatchKeyParam::Type> map_name_to_match_type =
      { {"exact", MatchKeyParam::Type::EXACT},
        {"lpm", MatchKeyParam::Type::LPM},
        {"ternary", MatchKeyParam::Type::TERNARY},
        {"valid", MatchKeyParam::Type::VALID},
        {"range", MatchKeyParam::Type::RANGE} };

  auto it = map_name_to_match_type.find(name);
  if (it == map_name_to_match_type.end())
    throw json_exception(EFormat() << "Invalid match type: '" << name << "'");
  return it->second;
}

std::vector<MatchKeyParam> parse_match_key(
    const Json::Value &cfg_match_key, const Json::Value &cfg_ref_match_key) {
  std::vector<MatchKeyParam> match_key;

  if (cfg_match_key.size() != cfg_ref_match_key.size()) {
    throw json_exception(
        EFormat() << "Invalid number of fields in match key, expected "
                  << cfg_ref_match_key.size() << " but got "
                  << cfg_match_key.size(),
        cfg_match_key);
  }

  auto convert_valid_key = [](bool key) {
    return key ? std::string("\x01", 1) : std::string("\x00", 1);
  };

  auto transform_hexstr = [](const std::string hexstr) {
    ByteContainer bc(hexstr);
    return std::string(bc.data(), bc.size());
  };

  for (size_t i = 0; i < cfg_match_key.size(); i++) {
    // cannot use [i] directly because of ambiguous operator overloading in
    // jsoncpp code
    auto index = static_cast<Json::ArrayIndex>(i);
    const auto &cfg_f = cfg_match_key[index];
    const auto match_type_str = cfg_f["match_type"].asString();
    const auto ref_match_type_str =
        cfg_ref_match_key[index]["match_type"].asString();

    if (match_type_str != ref_match_type_str) {
      throw json_exception(
          EFormat() << "Invalid match type for field #" << i
                    << " in match key, expected " << ref_match_type_str
                    << " but got " << match_type_str,
          cfg_match_key);
    }

    const auto match_type = match_name_to_match_type(match_type_str);
    switch (match_type) {
      case MatchKeyParam::Type::EXACT:
        match_key.emplace_back(match_type,
                               transform_hexstr(cfg_f["key"].asString()));
        break;
      case MatchKeyParam::Type::LPM:
        match_key.emplace_back(match_type,
                               transform_hexstr(cfg_f["key"].asString()),
                               cfg_f["prefix_length"].asInt());
        break;
      case MatchKeyParam::Type::TERNARY:
        match_key.emplace_back(match_type,
                               transform_hexstr(cfg_f["key"].asString()),
                               transform_hexstr(cfg_f["mask"].asString()));
        break;
      case MatchKeyParam::Type::VALID:
        match_key.emplace_back(match_type,
                               convert_valid_key(cfg_f["key"].asBool()));
        break;
      case MatchKeyParam::Type::RANGE:
        match_key.emplace_back(match_type,
                               transform_hexstr(cfg_f["start"].asString()),
                               transform_hexstr(cfg_f["end"].asString()));
        break;
    }
  }
  return match_key;
}

}  // namespace

void
P4Objects::init_pipelines(const Json::Value &cfg_root,
                          LookupStructureFactory *lookup_factory,
                          InitState *init_state) {
  auto &direct_meters = init_state->direct_meters;

  const Json::Value &cfg_pipelines = cfg_root["pipelines"];
  for (const auto &cfg_pipeline : cfg_pipelines) {
    const string pipeline_name = cfg_pipeline["name"].asString();
    p4object_id_t pipeline_id = cfg_pipeline["id"].asInt();

    // pipelines -> action profiles

    const auto &cfg_act_profs = cfg_pipeline["action_profiles"];
    for (const auto &cfg_act_prof : cfg_act_profs) {
      const auto act_prof_name = cfg_act_prof["name"].asString();
      const auto act_prof_id = cfg_act_prof["id"].asInt();
      const auto act_prof_size = cfg_act_prof["max_size"].asInt();
      // we ignore size for action profiles, at least for now
      (void) act_prof_size;
      const auto with_selection = cfg_act_prof.isMember("selector");

      std::unique_ptr<ActionProfile> action_profile(
          new ActionProfile(act_prof_name, act_prof_id, with_selection));
      if (with_selection) {
        auto calc = process_cfg_selector(cfg_act_prof["selector"]);
        action_profile->set_hash(std::move(calc));
      }
      add_action_profile(act_prof_name, std::move(action_profile));
    }

    // pipelines -> tables

    const Json::Value &cfg_tables = cfg_pipeline["tables"];
    for (const auto &cfg_table : cfg_tables) {
      const string table_name = cfg_table["name"].asString();
      p4object_id_t table_id = cfg_table["id"].asInt();

      MatchKeyBuilder key_builder;
      const Json::Value &cfg_match_key = cfg_table["key"];

      auto add_f = [this, &key_builder](
          const Json::Value &cfg_f) {
        const Json::Value &cfg_key_field = cfg_f["target"];
        const string header_name = cfg_key_field[0].asString();
        header_id_t header_id = get_header_id(header_name);
        const string field_name = cfg_key_field[1].asString();
        int field_offset = get_field_offset(header_id, field_name);
        const auto mtype = match_name_to_match_type(
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
            throw json_exception(
                EFormat() << "Table '" << table_name
                          << "' features 2 LPM match fields",
                cfg_match_key);
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
      const int table_size = get_table_size(cfg_table);
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
      } else {
        assert(table_type == "indirect" || table_type == "indirect_ws");
        bool with_selection = (table_type == "indirect_ws");
        if (table_type == "indirect") {
          table =
              MatchActionTable::create_match_action_table<MatchTableIndirect>(
                  match_type, table_name, table_id, table_size, key_builder,
                  with_counters, with_ageing, lookup_factory);
        } else {
          table =
              MatchActionTable::create_match_action_table<MatchTableIndirectWS>(
                  match_type, table_name, table_id, table_size, key_builder,
                  with_counters, with_ageing, lookup_factory);
        }
        // static_cast valid even when table is indirect_ws
        auto mt_indirect = static_cast<MatchTableIndirect *>(
            table->get_match_table());
        ActionProfile *action_profile = nullptr;
        if (cfg_table.isMember("action_profile")) {
          action_profile = get_action_profile(
              cfg_table["action_profile"].asString());
        } else if (cfg_table.isMember("act_prof_name")) {
          const auto name = cfg_table["act_prof_name"].asString();
          action_profile = new ActionProfile(name, table_id, with_selection);
          add_action_profile(
              name, std::unique_ptr<ActionProfile>(action_profile));
        } else {
          throw json_exception("indirect tables need to have attribute "
                               "'action_profile' (new JSON format) or "
                               "'act_prof_name' (old JSON format)\n",
                               cfg_table);
        }
        mt_indirect->set_action_profile(action_profile);

        if (table_type == "indirect_ws"
            && !cfg_table.isMember("action_profile")) {
          assert(cfg_table.isMember("selector"));
          auto calc = process_cfg_selector(cfg_table["selector"]);
          action_profile->set_hash(std::move(calc));
        }
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

      std::string act_prof_name("");
      if (cfg_table.isMember("action_profile")) {  // new JSON format
        act_prof_name = cfg_table["action_profile"].asString();
      } else if (cfg_table.isMember("act_prof_name")) {
        act_prof_name = cfg_table["act_prof_name"].asString();
      }

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
        if (act_prof_name != "")
          add_action_to_act_prof(act_prof_name, action_name, action);
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
        const auto table_type = cfg_table["type"].asString();
        if (table_type != "simple") {
          throw json_exception(
              EFormat() << "Table '" << table_name << "' does not have type "
                        << "'simple' and therefore cannot specify a "
                        << "'default_entry' attribute",
              cfg_table);
        }

        auto simple_table = dynamic_cast<MatchTable *>(table);
        assert(simple_table);

        const auto &cfg_default_entry = cfg_table["default_entry"];
        const p4object_id_t action_id = cfg_default_entry["action_id"].asInt();
        const ActionFn *action = get_action_by_id(action_id); assert(action);

        const Json::Value false_value(false);
        const bool is_action_const =
            cfg_default_entry.get("action_const", false_value).asBool();
        if (is_action_const) simple_table->set_const_default_action_fn(action);

        if (cfg_default_entry.isMember("action_data")) {
          auto adata = parse_action_data(action,
                                         cfg_default_entry["action_data"]);

          const bool is_action_entry_const =
              cfg_default_entry.get("action_entry_const", false_value).asBool();

          simple_table->set_default_entry(action, std::move(adata),
                                          is_action_entry_const);
        }
      }

      // for 'simple' tables, it is possible to specify immutable entries
      if (cfg_table.isMember("entries")) {
        const auto table_type = cfg_table["type"].asString();
        if (table_type != "simple") {
          throw json_exception(
              EFormat() << "Table '" << table_name << "' does not have type "
                        << "'simple' and therefore cannot specify a "
                        << "'entries' attribute",
              cfg_table);
        }

        auto simple_table = dynamic_cast<MatchTable *>(table);
        assert(simple_table);

        const auto &cfg_entries = cfg_table["entries"];

        for (const auto &cfg_entry : cfg_entries) {
          auto match_key = parse_match_key(cfg_entry["match_key"],
                                           cfg_table["key"]);

          const auto &cfg_action_entry = cfg_entry["action_entry"];
          auto action_id = static_cast<p4object_id_t>(
              cfg_action_entry["action_id"].asInt());
          const ActionFn *action = get_action_by_id(action_id); assert(action);
          auto adata = parse_action_data(action,
                                         cfg_action_entry["action_data"]);

          const auto priority = cfg_entry["priority"].asInt();

          entry_handle_t handle;
          auto rc = simple_table->add_entry(match_key, action, std::move(adata),
                                            &handle, priority);
          if (rc == MatchErrorCode::BAD_MATCH_KEY) {
            throw json_exception(
                "Error when adding table entry, match key is malformed",
                cfg_entry);
          } else if (rc == MatchErrorCode::DUPLICATE_ENTRY) {
            // We choose to treat this as an error to be consistent with the
            // control plane behavior
            throw json_exception("Duplicate entries in table initializer",
                                 cfg_entries);
          } else {
            assert(rc == MatchErrorCode::SUCCESS);
          }
          (void) handle;  // we have no need for the handle
        }

        // this means that the control plane won't be able to modify the entries
        // at runtime; we need to do this at the end otheriwse the add_entry
        // calls above would fail.
        simple_table->set_immutable_entries();
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
}

void
P4Objects::init_checksums(const Json::Value &cfg_root) {
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
}

void
P4Objects::init_learn_lists(const Json::Value &cfg_root) {
  const Json::Value &cfg_learn_lists = cfg_root["learn_lists"];

  if (cfg_learn_lists.size() > 0) {
    assert(notifications_transport);
  }

  for (const auto &cfg_learn_list : cfg_learn_lists) {
    LearnEngineIface::list_id_t list_id = cfg_learn_list["id"].asInt();
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
}

void
P4Objects::init_field_lists(const Json::Value &cfg_root) {
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
      if (type == "field") {
        const Json::Value &cfg_value_field = cfg_element["value"];
        const string header_name = cfg_value_field[0].asString();
        header_id_t header_id = get_header_id(header_name);
        const string field_name = cfg_value_field[1].asString();
        int field_offset = get_field_offset(header_id, field_name);
        field_list->push_back_field(header_id, field_offset);
      } else if (type == "hexstr") {
        const Json::Value &cfg_value_hexstr = cfg_element["value"];
        const Json::Value &cfg_value_bitwidth = cfg_element["bitwidth"];
        field_list->push_back_constant(
            cfg_value_hexstr.asString(), cfg_value_bitwidth.asInt());
      } else {
        assert(0);  // TODO(antonin): other types
      }
    }

    add_field_list(list_id, std::move(field_list));
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
    this->notifications_transport = std::shared_ptr<TransportIface>(
        TransportIface::make_dummy());
  } else {
    this->notifications_transport = notifications_transport;
  }

  InitState init_state;

  try {
    init_enums(cfg_root);

    init_header_types(cfg_root);

    init_headers(cfg_root);

    init_header_stacks(cfg_root);

    if (cfg_root.isMember("field_aliases")) {
      const auto &cfg_field_aliases = cfg_root["field_aliases"];

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

    init_extern_instances(cfg_root);

    init_parse_vsets(cfg_root);

    init_errors(cfg_root);  // parser errors

    init_parsers(cfg_root);

    init_deparsers(cfg_root);

    init_calculations(cfg_root);

    init_counter_arrays(cfg_root);

    init_meter_arrays(cfg_root, &init_state);

    init_register_arrays(cfg_root);

    init_actions(cfg_root);

    ageing_monitor = AgeingMonitorIface::make(
        device_id, cxt_id, notifications_transport);

    init_pipelines(cfg_root, lookup_factory, &init_state);

    init_checksums(cfg_root);

    learn_engine = LearnEngineIface::make(device_id, cxt_id);

    init_learn_lists(cfg_root);

    init_field_lists(cfg_root);

    // invoke init() for extern instances, we do this at the very end in case
    // init() looks up some object (e.g. RegisterArray) in P4Objects
    for (const auto &p : extern_instances)
      p.second->init();

    check_required_fields(required_fields);
  } catch (const json_exception &e) {
    outstream << e.msg(verbose_output);
    return 1;
  }

  // force arith fields

  if (cfg_root.isMember("force_arith")) {
    const Json::Value &cfg_force_arith = cfg_root["force_arith"];

    for (const auto &cfg_field : cfg_force_arith) {
      const auto field = field_info(cfg_field[0].asString(),
                                    cfg_field[1].asString());
      enable_arith(std::get<0>(field), std::get<1>(field));
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
      enable_arith(std::get<0>(field), std::get<1>(field));
    }
  }

  for (const auto &h : arith_objects.headers) {
    if (!header_exists(h)) continue;
    // safe to call because we just checked for the header existence
    const header_id_t header_id = get_header_id(h);
    enable_arith(header_id);
  }

  parse_config_options(cfg_root);

  return 0;
}

void
P4Objects::reset_state() {
  // TODO(antonin): is this robust?
  for (const auto &e : match_action_tables_map) {
    e.second->get_match_table()->reset_state();
  }
  for (const auto &e : action_profiles_map) {
    e.second->reset_state();
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
  for (const auto &e : action_profiles_map) {
    e.second->serialize(out);
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
  for (const auto &e : action_profiles_map) {
    e.second->deserialize(in, *this);
  }
  for (const auto &e : meter_arrays) {
    e.second->deserialize(in);
  }
}

int
P4Objects::get_field_offset(header_id_t header_id,
                            const string &field_name) const {
  const HeaderType &header_type = phv_factory.get_header_type(header_id);
  return header_type.get_field_offset(field_name);
}

size_t
P4Objects::get_field_bytes(header_id_t header_id, int field_offset) const {
  const HeaderType &header_type = phv_factory.get_header_type(header_id);
  return (header_type.get_bit_width(field_offset) + 7) / 8;
}

size_t
P4Objects::get_field_bits(header_id_t header_id, int field_offset) const {
  const HeaderType &header_type = phv_factory.get_header_type(header_id);
  return header_type.get_bit_width(field_offset);
}

size_t
P4Objects::get_header_bits(header_id_t header_id) const {
  const HeaderType &header_type = phv_factory.get_header_type(header_id);
  return header_type.get_bit_width();
}

std::tuple<header_id_t, int>
P4Objects::field_info(const string &header_name,
                      const string &field_name) const {
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
      throw json_exception(
          EFormat() << "Field " << p.first << "." << p.second
                    << " is required by switch target but is not defined");
    }
  }
  return res;
}

std::unique_ptr<CalculationsMap::MyC>
P4Objects::check_hash(const std::string &name) const {
  auto h = CalculationsMap::get_instance()->get_copy(name);
  if (!h) throw json_exception(EFormat() << "Unknown hash algorithm: " << name);
  return h;
}

std::unique_ptr<Calculation>
P4Objects::process_cfg_selector(const Json::Value &cfg_selector) const {
  const auto selector_algo = cfg_selector["algo"].asString();
  const auto &cfg_selector_input = cfg_selector["input"];

  BufBuilder builder;
  // TODO(antonin): I do this kind of thing in a bunch of places, I need to find
  // a nicer way
  for (const auto &cfg_element : cfg_selector_input) {
    const auto type = cfg_element["type"].asString();
    assert(type == "field");  // TODO(antonin): other types

    const auto &cfg_value_field = cfg_element["value"];
    const auto header_name = cfg_value_field[0].asString();
    const auto header_id = get_header_id(header_name);
    const auto field_name = cfg_value_field[1].asString();
    auto field_offset = get_field_offset(header_id, field_name);
    builder.push_back_field(header_id, field_offset);
  }

  check_hash(selector_algo);

  return std::unique_ptr<Calculation>(new Calculation(builder, selector_algo));
}

ActionPrimitive_ *
P4Objects::get_primitive(const std::string &name) {
  auto it = primitives.find(name);
  if (it == primitives.end()) {
    auto prim = ActionOpcodesMap::get_instance()->get_primitive(name);
    if (!prim) return nullptr;
    prim->_set_p4objects(this);
    primitives[name] = std::move(prim);
    return primitives[name].get();
  }
  return it->second.get();
}

void
P4Objects::enable_arith(header_id_t header_id, int field_offset) {
  auto it = header_id_to_stack_id.find(header_id);
  if (it == header_id_to_stack_id.end()) {
    phv_factory.enable_field_arith(header_id, field_offset);
  } else {
    phv_factory.enable_stack_field_arith(it->second, field_offset);
  }
}

void
P4Objects::enable_arith(header_id_t header_id) {
  auto it = header_id_to_stack_id.find(header_id);
  if (it == header_id_to_stack_id.end()) {
    phv_factory.enable_all_field_arith(header_id);
  } else {
    phv_factory.enable_all_stack_field_arith(it->second);
  }
}

ActionFn *
P4Objects::get_action_for_action_profile(
    const std::string &act_prof_name, const std::string &action_name) const {
  return aprof_actions_map.at(std::make_pair(act_prof_name, action_name));
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

Parser *
P4Objects::get_parser_rt(const std::string &name) const {
  auto it = parsers.find(name);
  return (it != parsers.end()) ? it->second.get() : nullptr;
}

Deparser *
P4Objects::get_deparser_rt(const std::string &name) const {
  auto it = deparsers.find(name);
  return (it != deparsers.end()) ? it->second.get() : nullptr;
}

Pipeline *
P4Objects::get_pipeline_rt(const std::string &name) const {
  auto it = pipelines_map.find(name);
  return (it != pipelines_map.end()) ? it->second.get() : nullptr;
}

ExternType *
P4Objects::get_extern_instance_rt(const std::string &name) const {
  auto it = extern_instances.find(name);
  return (it != extern_instances.end()) ? it->second.get() : nullptr;
}

ActionProfile *
P4Objects::get_action_profile_rt(const std::string &name) const {
  auto it = action_profiles_map.find(name);
  return (it != action_profiles_map.end()) ? it->second.get() : nullptr;
}

ConfigOptionMap
P4Objects::get_config_options() const {
  return config_options;
}

ErrorCodeMap
P4Objects::get_error_codes() const {
  return error_codes;
}

EnumMap::type_t
P4Objects::get_enum_value(const std::string &name) const {
  return enums.from_name(name);
}

const std::string &
P4Objects::get_enum_name(const std::string &enum_name,
                         EnumMap::type_t entry_value) const {
  return enums.to_name(enum_name, entry_value);
}

void
P4Objects::add_header_type(const std::string &name,
                           std::unique_ptr<HeaderType> header_type) {
  header_types_map[name] = std::move(header_type);
}

HeaderType *
P4Objects::get_header_type(const std::string &name) {
  return header_types_map.at(name).get();
}

void
P4Objects::add_header_id(const std::string &name, header_id_t header_id) {
  header_ids_map[name] = header_id;
}

void
P4Objects::add_header_stack_id(const std::string &name,
                               header_stack_id_t header_stack_id) {
  header_stack_ids_map[name] = header_stack_id;
}

header_id_t
P4Objects::get_header_id(const std::string &name) const {
  return header_ids_map.at(name);
}

header_stack_id_t
P4Objects::get_header_stack_id(const std::string &name) const {
  return header_stack_ids_map.at(name);
}

void
P4Objects::add_action(p4object_id_t id, std::unique_ptr<ActionFn> action) {
  actions_map[id] = std::move(action);
}

void
P4Objects::add_action_to_table(
    const std::string &table_name,
    const std::string &action_name, ActionFn *action) {
  t_actions_map[std::make_pair(table_name, action_name)] = action;
}

void
P4Objects::add_action_to_act_prof(const std::string &act_prof_name,
                                  const std::string &action_name,
                                  ActionFn *action) {
  aprof_actions_map[std::make_pair(act_prof_name, action_name)] = action;
}

void
P4Objects::add_parser(const std::string &name, std::unique_ptr<Parser> parser) {
  parsers[name] = std::move(parser);
}

void
P4Objects::add_parse_vset(const std::string &name,
                          std::unique_ptr<ParseVSet> parse_vset) {
  parse_vsets[name] = std::move(parse_vset);
}

void
P4Objects::add_deparser(const std::string &name,
                        std::unique_ptr<Deparser> deparser) {
  deparsers[name] = std::move(deparser);
}

void
P4Objects::add_match_action_table(const std::string &name,
                                  std::unique_ptr<MatchActionTable> table) {
  add_control_node(name, table.get());
  match_action_tables_map[name] = std::move(table);
}

void
P4Objects::add_action_profile(const std::string &name,
                              std::unique_ptr<ActionProfile> action_profile) {
  action_profiles_map[name] = std::move(action_profile);
}

void
P4Objects::add_conditional(const std::string &name,
                           std::unique_ptr<Conditional> conditional) {
  add_control_node(name, conditional.get());
  conditionals_map[name] = std::move(conditional);
}

void
P4Objects::add_control_node(const std::string &name, ControlFlowNode *node) {
  control_nodes_map[name] = node;
}

void
P4Objects::add_pipeline(const std::string &name,
                        std::unique_ptr<Pipeline> pipeline) {
  pipelines_map[name] = std::move(pipeline);
}

void
P4Objects::add_meter_array(const std::string &name,
                           std::unique_ptr<MeterArray> meter_array) {
  meter_arrays[name] = std::move(meter_array);
}

void
P4Objects::add_counter_array(const std::string &name,
                             std::unique_ptr<CounterArray> counter_array) {
  counter_arrays[name] = std::move(counter_array);
}

void
P4Objects::add_register_array(const std::string &name,
                              std::unique_ptr<RegisterArray> register_array) {
  register_arrays[name] = std::move(register_array);
}

void
P4Objects::add_named_calculation(
    const std::string &name, std::unique_ptr<NamedCalculation> calculation) {
  calculations[name] = std::move(calculation);
}

void
P4Objects::add_field_list(const p4object_id_t field_list_id,
                          std::unique_ptr<FieldList> field_list) {
  field_lists[field_list_id] = std::move(field_list);
}

void
P4Objects::add_extern_instance(const std::string &name,
                               std::unique_ptr<ExternType> extern_instance) {
  extern_instances[name] = std::move(extern_instance);
}

}  // namespace bm
