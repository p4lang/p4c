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

#include "bm_sim/actions.h"

size_t ActionFn::nb_data_tmps = 0;

void ActionFn::parameter_push_back_field(header_id_t header, int field_offset) {
  ActionParam param;
  param.tag = ActionParam::FIELD;
  param.field = {header, field_offset};
  params.push_back(param);
}

void ActionFn::parameter_push_back_header(header_id_t header) {
  ActionParam param;
  param.tag = ActionParam::HEADER;
  param.header = header;
  params.push_back(param);
}

void ActionFn::parameter_push_back_header_stack(header_stack_id_t header_stack) {
  ActionParam param;
  param.tag = ActionParam::HEADER_STACK;
  param.header_stack = header_stack;
  params.push_back(param);
}

void ActionFn::parameter_push_back_const(const Data &data) {
  const_values.push_back(data);
  ActionParam param;
  param.tag = ActionParam::CONST;
  param.const_offset = const_values.size() - 1;;
  params.push_back(param);
}

void ActionFn::parameter_push_back_action_data(int action_data_offset) {
  ActionParam param;
  param.tag = ActionParam::ACTION_DATA;
  param.action_data_offset = action_data_offset;
  params.push_back(param);
}

void ActionFn::parameter_push_back_calculation(const NamedCalculation *calculation) {
  ActionParam param;
  param.tag = ActionParam::CALCULATION;
  param.calculation = calculation;
  params.push_back(param);
}

void ActionFn::parameter_push_back_meter_array(MeterArray *meter_array) {
  ActionParam param;
  param.tag = ActionParam::METER_ARRAY;
  param.meter_array = meter_array;
  params.push_back(param);
}

void ActionFn::parameter_push_back_counter_array(CounterArray *counter_array) {
  ActionParam param;
  param.tag = ActionParam::COUNTER_ARRAY;
  param.counter_array = counter_array;
  params.push_back(param);
}

void ActionFn::parameter_push_back_register_array(RegisterArray *register_array) {
  ActionParam param;
  param.tag = ActionParam::REGISTER_ARRAY;
  param.register_array = register_array;
  params.push_back(param);
}

void ActionFn::parameter_push_back_expression(
  std::unique_ptr<ArithExpression> expr
) {
  size_t nb_expression_params = 0;
  for(const ActionParam &p : params)
    if(p.tag == ActionParam::EXPRESSION) nb_expression_params += 1;

  assert(nb_expression_params <= ActionFn::nb_data_tmps);
  if(nb_expression_params == ActionFn::nb_data_tmps)
    ActionFn::nb_data_tmps += 1;

  expressions.push_back(std::move(expr));
  ActionParam param;
  param.tag = ActionParam::EXPRESSION;
  param.expression = {static_cast<unsigned int>(nb_expression_params),
		      expressions.back().get()};
  params.push_back(param);
}

void ActionFn::push_back_primitive(ActionPrimitive_ *primitive) {
  primitives.push_back(primitive);
}


bool ActionOpcodesMap::register_primitive(
    const char *name,
    std::unique_ptr<ActionPrimitive_> primitive) {
  const std::string str_name = std::string(name);
  auto it = map_.find(str_name);
  if (it != map_.end()) return false;
  map_[str_name] = std::move(primitive);
  return true;
}

ActionPrimitive_ *ActionOpcodesMap::get_primitive(const std::string &name) {
  return map_[name].get();
}

ActionOpcodesMap *ActionOpcodesMap::get_instance() {
  static ActionOpcodesMap instance;
  return &instance;
}
